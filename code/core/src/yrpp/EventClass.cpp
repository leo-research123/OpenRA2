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
#include "runtime_diagnostics.hpp"
#include "debug/debug_state.hpp"

namespace {
QueueClass<EventClass, EventClass::OUT_CAPACITY> outgoing;
QueueClass<EventClass, EventClass::DO_CAPACITY> scheduled;
QueueClass<EventClass, EventClass::MEGA_CAPACITY> mega_missions;
int event_execution_delay = 0;
bool invalid_object(const ObjectClass* object) {
    return object && (!object->IsAlive || !object->Health || object->InLimbo);
}
}
QueueClass<EventClass, EventClass::OUT_CAPACITY>& EventClass::OutList = outgoing;
QueueClass<EventClass, EventClass::DO_CAPACITY>& EventClass::DoList = scheduled;
QueueClass<EventClass, EventClass::MEGA_CAPACITY>& EventClass::MegaMissionList = mega_missions;
int& EventClass::LastExecutionDelay = event_execution_delay;

void EventClass::Execute() {
    game::runtime_log_event("command", "type=%d house=%d", int(Type), int(HouseIndex));
    if (Type == EventType::MegaMission)
        game::runtime_log_event("command_target",
                                "actor=%u mission=%u target=%u target_kind=%u destination=%u destination_kind=%u",
                                unsigned(MegaMission.Whom.m_ID), unsigned(MegaMission.Mission),
                                unsigned(MegaMission.Target.m_ID), unsigned(MegaMission.Target.m_RTTI),
                                unsigned(MegaMission.Destination.m_ID), unsigned(MegaMission.Destination.m_RTTI));
    if (Type == EventType::SpecialPlace) {
        auto* house = HouseClass::Array.GetItemOrDefault(static_cast<unsigned char>(HouseIndex));
        if (house) house->Fire_SW(SpecialPlace.ID, SpecialPlace.Location);
        return;
    }
    if (Type == EventType::AllCheer) {
        if (auto* house = HouseClass::Array.GetItemOrDefault(static_cast<unsigned char>(HouseIndex))) house->Cheer();
        return;
    }
    if (Type == EventType::Options) {
        if ((static_cast<unsigned>(Game::RecordingFlag) & 0x02u) == 0) Game::SpecialDialog = 1;
        return;
    }
    if (Type == EventType::Scatter) {
        auto* actor = Scatter.Whom.As_Techno();
        Game::PlanningManager_ClearToken(actor, nullptr);
        if (actor && (actor->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None && actor->IsAlive &&
            !actor->InLimbo && !actor->IsTether) {
            static_cast<FootClass*>(actor)->unknown_bool_6B2 = true;
            actor->Scatter(CoordStruct::Empty, true, false);
        }
        return;
    }
    if (Type == EventType::Archive) {
        auto* object = Archive.Whom1.As_Techno();
        Game::PlanningManager_ClearToken(object, nullptr);
        if (object && object->IsAlive) object->SetArchiveTarget(Archive.Whom2.As_Abstract());
        return;
    }

    if (Type == EventType::Produce || Type == EventType::Suspend || Type == EventType::Abandon ||
        Type == EventType::Place) {
        auto* house = HouseClass::Array.GetItemOrDefault(static_cast<unsigned char>(HouseIndex));
        if (!house) return;
        switch (Type) {
            case EventType::Produce:
                house->BeginProduction(Produce.RTTIType, Produce.HeapID, Produce.IsNaval != 0);
                break;
            case EventType::Suspend:
                house->SuspendProduction(Suspend.RTTIType, Suspend.HeapID, Suspend.IsNaval != 0);
                break;
            case EventType::Abandon:
                house->AbandonProduction(Abandon.RTTIType, Abandon.HeapID, Abandon.IsNaval != 0, false);
                break;
            case EventType::Place:
                house->PlaceObject(Place.RTTIType, Place.HeapID, Place.IsNaval != 0, Place.Location);
                break;
            default:
                break;
        }
        return;
    }
    if (Type == EventType::Repair || Type == EventType::Sell) {
        auto* object = Repair.Whom.As_Techno();
        Game::PlanningManager_ClearToken(object, nullptr);
        if (!object || !object->IsAlive) return;
        if (Type == EventType::Repair)
            object->SetRepairState(-1);
        else if ((object->Owner->ArrayIndex == static_cast<signed char>(HouseIndex) ||
                  (HouseClass::CurrentPlayer && HouseIndex == HouseClass::CurrentPlayer->ArrayIndex &&
                   game::debug_can_control_house(object->Owner))) &&
                 object->WhatAmI() == AbstractType::Building)
            object->Sell(-1);
        return;
    }
    if (Type == EventType::Idle) {
        // OpenTS IDLE, calibrated to YR 0x4C6CB0 case 0x6. This is not a
        // Mission::Stop order or an immediate locomotor position reset.
        auto* techno = Idle.Whom.As_Techno();
        if (techno && techno->PlanningToken) techno->ClearPlanningTokens(nullptr);
        if (!techno || !techno->IsAlive || techno->InLimbo || techno->IsTether) return;
        if (techno->CurrentMission == Mission::Construction || techno->CurrentMission == Mission::Selling) return;
        if (!techno->OnBridge && !techno->GetCell()->SlopeIndex && techno->vt_entry_2B0()) return;
        if ((techno->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None) {
            auto* foot = static_cast<FootClass*>(techno);
            foot->NavQueue.Clear();
            foot->ClearNavigationList();
            foot->PlanningPathIdx = -1;
            foot->WaypointIndex = 0;
            foot->WaypointNearbyAccessibleCellDelta = {0, 0};
            foot->WaypointCell = {0, 0};
        }
        techno->SendToEachLink(RadioCommand::NotifyUnlink);
        techno->SetDestination(nullptr, true);
        techno->SetTarget(nullptr);
        if (techno->GetTechnoType()->BalloonHover) {
            techno->SetDestination(nullptr, true);
            techno->SetTarget(nullptr);
        }
        if (techno->SpawnManager) techno->SpawnManager->ResetTarget();
        if (techno->GetTechnoType()->OpenTopped) techno->SetTargetForPassengers(nullptr);
        if (techno->WhatAmI() == AbstractType::Unit) {
            auto* unit = static_cast<UnitClass*>(techno);
            if (unit->Type->Harvester &&
                (unit->CurrentMission == Mission::Harvest || unit->CurrentMission == Mission::Enter)) {
                unit->QueueMission(Mission::Guard, false);
                unit->NextMission();
            }
            if (unit->SlaveManager) unit->SlaveManager->AllGuard();
        }
        return;
    }
    if (Type == EventType::Deploy) {
        auto* techno = Deploy.Whom.As_Techno();
        if (techno && techno->PlanningToken) techno->ClearPlanningTokens(nullptr);
        if (!techno || !techno->IsAlive || techno->InLimbo || techno->IsTether || techno->EMPLockRemaining) return;
        if (!techno->OnBridge && !techno->GetCell()->SlopeIndex && techno->vt_entry_2B0()) return;
        if (techno->CurrentMission == Mission::Construction || techno->CurrentMission == Mission::Selling ||
            techno->WhatAmI() == AbstractType::Aircraft)
            return;
        const auto at = techno->GetMapCoords();
        auto* building = at == CellStruct::Empty ? nullptr : MapClass::Instance.GetCellAt(at)->GetBuilding();
        if (building && building->Type->WeaponsFactory) return;
        techno->SendToFirstLink(RadioCommand::NotifyUnlink);
        techno->SetDestination(nullptr, true);
        techno->SetTarget(nullptr);
        techno->QueueMission(Mission::Unload, false);
        return;
    }
    if (Type == EventType::PlanConnect || Type == EventType::PlanCommit || Type == EventType::PlanNodeDelete) {
        Game::PlanningManager_Execute(this);
        return;
    }
    if (Type != EventType::MegaMission && Type != EventType::MegaMissionF) std::abort();
    auto* techno = MegaMission.Whom.As_Techno();
    // 0x004C71D7 compares the raw byte with 1. Planned-node execution writes
    // 2 at 0x0063865E and follows the ordinary mission path without clearing
    // its token graph; this field is not a C++ bool.
    if (MegaMission.IsPlanningEvent == 1) {
        Game::PlanningManager_Execute(this);
        return;
    }
    if (techno && techno->PlanningToken) techno->ClearPlanningTokens(this);
    if (!techno || !techno->IsAlive || techno->Health <= 0 || techno->InLimbo) return;
    ObjectClass* clicked = nullptr;
    if (MegaMission.Target.m_RTTI) {
        clicked = MegaMission.Target.As_Object();
        if (invalid_object(clicked)) return;
    }
    if (MegaMission.Destination.m_RTTI) {
        clicked = MegaMission.Destination.As_Object();
        if (invalid_object(clicked)) return;
    }
    auto* foot = (techno->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None ? static_cast<FootClass*>(techno)
                                                                                      : nullptr;
    auto* link = techno->RadioLinks.Capacity ? techno->GetNthLink() : nullptr;
    const auto requested = static_cast<Mission>(static_cast<signed char>(MegaMission.Mission));
    if (requested == Mission::Enter && techno->CurrentMission == Mission::Enter && foot) {
        auto* destination = MegaMission.Destination.As_Abstract();
        if (destination && destination->WhatAmI() == AbstractType::Building && destination == link) return;
    }
    if (!techno->IsTether)
        techno->SendToFirstLink(RadioCommand::NotifyUnlink);
    else if (link && link->IsAlive && link->WhatAmI() == AbstractType::Building &&
             static_cast<BuildingClass*>(link)->Type->Refinery) {
        techno->SendToFirstLink(RadioCommand::NotifyUnlink);
        techno->IsTether = false;
    }
    techno->QueueUpToEnter = nullptr;
    if (foot && foot->Team && requested != Mission::Unload) foot->Team->LiberateMember(foot, -1, 1);
    if (clicked && HouseClass::CurrentPlayer->IsAlliedWith(techno)) clicked->Flash(7);
    const auto mission = techno->RespondMegaEventMission(this);
    techno->QueueMission(mission, false);
    if (foot) foot->LastDestination = nullptr;
    techno->LastTarget = nullptr;
    if (techno->SlaveManager && mission != Mission::Attack) techno->SlaveManager->AllGuard();
    if (requested == Mission::Area_Guard && foot) {
        techno->SetTarget(nullptr);
        techno->SetDestination(MegaMission.Target.As_Abstract(), true);
        techno->SetArchiveTarget(MegaMission.Target.As_Abstract());
    } else {
        if (foot) {
            foot->SetArchiveTarget(nullptr);
            foot->ClearNavigationList();
        }
        auto* target = MegaMission.Target.As_Abstract();
        techno->SetTarget(target);
        techno->SetDestination(MegaMission.Destination.As_Abstract(), true);
        if (techno->GetTechnoType()->OpenTopped && target) techno->SetTargetForPassengers(target);
        if (MegaMission.Follow.m_RTTI) {
            techno->vt_entry_47C(MegaMission.Follow.As_Abstract());
        }
    }
}
