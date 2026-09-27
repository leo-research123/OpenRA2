// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 event.cpp::Execute, calibrated to YR 0x4C6CB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/EventClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/UnitClass.h"
#include <cstdlib>

namespace {
QueueClass<EventClass,EventClass::MAX_EVENTS> outgoing;
QueueClass<EventClass,EventClass::MAX_EVENTS*128> scheduled;
bool invalid_object(const ObjectClass* object) {
    return object && (!object->IsAlive || !object->Health || object->InLimbo);
}
}
QueueClass<EventClass,EventClass::MAX_EVENTS>& EventClass::OutList=outgoing;
QueueClass<EventClass,EventClass::MAX_EVENTS*128>& EventClass::DoList=scheduled;

void EventClass::Execute() {
    if(Type==EventType::Produce||Type==EventType::Suspend||Type==EventType::Abandon||Type==EventType::Place){
        auto* house=HouseClass::Array.GetItemOrDefault(static_cast<unsigned char>(HouseIndex));if(!house)return;
        switch(Type){
        case EventType::Produce:house->BeginProduction(Produce.RTTIType,Produce.HeapID,Produce.IsNaval!=0);break;
        case EventType::Suspend:house->SuspendProduction(Suspend.RTTIType,Suspend.HeapID,Suspend.IsNaval!=0);break;
        case EventType::Abandon:house->AbandonProduction(Abandon.RTTIType,Abandon.HeapID,Abandon.IsNaval!=0,false);break;
        case EventType::Place:house->PlaceObject(Place.RTTIType,Place.HeapID,Place.IsNaval!=0,Place.Location);break;
        default:break;
        }
        return;
    }
    if(Type==EventType::Repair || Type==EventType::Sell) {
        auto* object=Repair.Whom.As_Techno();
        Game::PlanningManager_ClearToken(object,nullptr);
        if(!object || !object->IsAlive)return;
        if(Type==EventType::Repair)object->SetRepairState(-1);
        else if(object->Owner->ArrayIndex==static_cast<signed char>(HouseIndex)
            && object->WhatAmI()==AbstractType::Building)object->Sell(-1);
        return;
    }
    if(Type==EventType::Idle){
        // OpenTS IDLE, calibrated to YR 0x4C6CB0 case 0x6. This is not a
        // Mission::Stop order or an immediate locomotor position reset.
        auto* techno=Idle.Whom.As_Techno();
        if(techno&&techno->PlanningToken)techno->ClearPlanningTokens(nullptr);
        if(!techno||!techno->IsAlive||techno->InLimbo||techno->IsTether)return;
        if(techno->CurrentMission==Mission::Construction||techno->CurrentMission==Mission::Selling)return;
        if(!techno->OnBridge&&!techno->GetCell()->SlopeIndex&&techno->vt_entry_2B0())return;
        if((techno->AbstractFlags&AbstractFlags::Foot)!=AbstractFlags::None){
            auto* foot=static_cast<FootClass*>(techno);
            foot->NavQueue.Clear();foot->ClearNavigationList();
            foot->PlanningPathIdx=-1;foot->WaypointIndex=0;
            foot->WaypointNearbyAccessibleCellDelta={0,0};foot->WaypointCell={0,0};
        }
        techno->SendToEachLink(RadioCommand::NotifyUnlink);
        techno->SetDestination(nullptr,true);techno->SetTarget(nullptr);
        if(techno->GetTechnoType()->BalloonHover){
            techno->SetDestination(nullptr,true);techno->SetTarget(nullptr);
        }
        if(techno->SpawnManager)techno->SpawnManager->ResetTarget();
        if(techno->GetTechnoType()->OpenTopped)techno->SetTargetForPassengers(nullptr);
        if(techno->WhatAmI()==AbstractType::Unit){
            auto* unit=static_cast<UnitClass*>(techno);
            if(unit->Type->Harvester&&(unit->CurrentMission==Mission::Harvest||unit->CurrentMission==Mission::Enter)){
                unit->QueueMission(Mission::Guard,false);unit->NextMission();
            }
            if(unit->SlaveManager)unit->SlaveManager->AllGuard();
        }
        return;
    }
    if(Type==EventType::Deploy){
        auto* techno=Deploy.Whom.As_Techno();
        if(techno&&techno->PlanningToken)techno->ClearPlanningTokens(nullptr);
        if(!techno||!techno->IsAlive||techno->InLimbo||techno->IsTether||techno->EMPLockRemaining)return;
        if(!techno->OnBridge&&!techno->GetCell()->SlopeIndex&&techno->vt_entry_2B0())return;
        if(techno->CurrentMission==Mission::Construction||techno->CurrentMission==Mission::Selling||techno->WhatAmI()==AbstractType::Aircraft)return;
        const auto at=techno->GetMapCoords();
        auto* building=at==CellStruct::Empty?nullptr:MapClass::Instance.GetCellAt(at)->GetBuilding();
        if(building&&building->Type->WeaponsFactory)return;
        techno->SendToFirstLink(RadioCommand::NotifyUnlink);
        techno->SetDestination(nullptr,true);techno->SetTarget(nullptr);techno->QueueMission(Mission::Unload,false);
        return;
    }
    // The currently migrated dispatcher arm is the YR mission wire format.
    // Other event kinds remain unsupported, rather than silently succeeding.
    // TODO(RADAR-PLAN-EXEC): user deferred the 0x00637E00 planning dispatcher.
    // PlanConnect/PlanCommit/PlanNodeDelete (0x2A/0x2B/0x2C) still reach this
    // unsupported-event guard; raw planning state 1 reaches the guard below.
    if(Type!=EventType::MegaMission && Type!=EventType::MegaMissionF)std::abort();
    auto* techno=MegaMission.Whom.As_Techno();
    // 0x004C71D7 compares the raw byte with 1. Planned-node execution writes
    // 2 at 0x0063865E and follows the ordinary mission path without clearing
    // its token graph; this field is not a C++ bool.
    if(MegaMission.IsPlanningEvent==1)std::abort(); // TODO(RADAR-PLAN-EXEC): deferred dispatcher.
    if(techno && techno->PlanningToken)techno->ClearPlanningTokens(this);
    if(!techno || !techno->IsAlive || techno->Health<=0 || techno->InLimbo)return;
    ObjectClass* clicked=nullptr;
    if(MegaMission.Target.m_RTTI) {
        clicked=MegaMission.Target.As_Object();if(invalid_object(clicked))return;
    }
    if(MegaMission.Destination.m_RTTI) {
        clicked=MegaMission.Destination.As_Object();if(invalid_object(clicked))return;
    }
    auto* foot=(techno->AbstractFlags & AbstractFlags::Foot)!=AbstractFlags::None?static_cast<FootClass*>(techno):nullptr;
    auto* link=techno->RadioLinks.Capacity?techno->GetNthLink():nullptr;
    const auto requested=static_cast<Mission>(static_cast<signed char>(MegaMission.Mission));
    if(requested==Mission::Enter && techno->CurrentMission==Mission::Enter && foot) {
        auto* destination=MegaMission.Destination.As_Abstract();
        if(destination && destination->WhatAmI()==AbstractType::Building && destination==link)return;
    }
    if(!techno->IsTether)techno->SendToFirstLink(RadioCommand::NotifyUnlink);
    else if(link && link->IsAlive && link->WhatAmI()==AbstractType::Building
        && static_cast<BuildingClass*>(link)->Type->Refinery) {
        techno->SendToFirstLink(RadioCommand::NotifyUnlink);techno->IsTether=false;
    }
    techno->QueueUpToEnter=nullptr;
    if(foot && foot->Team && requested!=Mission::Unload)foot->Team->LiberateMember(foot,-1,1);
    if(clicked && HouseClass::CurrentPlayer->IsAlliedWith(techno))clicked->Flash(7);
    const auto mission=techno->RespondMegaEventMission(this);
    techno->QueueMission(mission,false);
    if(foot)foot->LastDestination=nullptr;
    techno->LastTarget=nullptr;
    if(techno->SlaveManager && mission!=Mission::Attack)techno->SlaveManager->AllGuard();
    if(requested==Mission::Area_Guard && foot) {
        techno->SetTarget(nullptr);
        techno->SetDestination(MegaMission.Target.As_Abstract(),true);
        techno->SetArchiveTarget(MegaMission.Target.As_Abstract());
    }else {
        if(foot){foot->SetArchiveTarget(nullptr);foot->ClearNavigationList();}
        auto* target=MegaMission.Target.As_Abstract();
        techno->SetTarget(target);
        techno->SetDestination(MegaMission.Destination.As_Abstract(),true);
        if(techno->GetTechnoType()->OpenTopped && target)techno->SetTargetForPassengers(target);
        if(MegaMission.Follow.m_RTTI) {
            techno->vt_entry_47C(MegaMission.Follow.As_Abstract());
        }
    }
}
