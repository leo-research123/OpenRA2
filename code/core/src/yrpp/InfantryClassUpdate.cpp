// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::AI, calibrated to YR 0x51BAB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Main flow only: tunnel and combat/theft effects remain
// explicit original-entry dependencies. This is not native Logic admission.
#include "yrpp/InfantryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/TagClass.h"
#include <bit>
#include <cmath>
#include <cstdlib>

bool InfantryClass::Theft_AI() {
    // OpenTS Theft_AI, YR 0x5202F0. Thief is distinct from VehicleThief.
    if(!Type->Thief || !Destination || Destination->WhatAmI()!=AbstractType::Unit)return false;
    auto* unit=static_cast<UnitClass*>(Destination);
    if(Owner->IsAlliedWith(unit))return false;
    const auto delta=[](int a,int b){return std::bit_cast<int>(static_cast<unsigned>(a)-static_cast<unsigned>(b));};
    const auto distance=[&](const CoordStruct& a,const CoordStruct& b) {
        const double x=delta(a.X,b.X),y=delta(a.Y,b.Y),z=delta(a.Z,b.Z);
        // Coord::Length returns int64, then this caller compares its low int32.
        return std::bit_cast<int>(static_cast<unsigned>(static_cast<std::int64_t>(std::sqrt(x*x+y*y+z*z))));
    };
    const auto destination=unit->GetDestination(this);
    const int height=delta(destination.Z,GetCoords().Z);
    const int magnitude=height<0?std::bit_cast<int>(0u-static_cast<unsigned>(height)):height;
    const auto distance_to_unit=[&] {
        const auto target=unit->GetCoords();const auto here=GetCoords();
        return distance(here,target);
    };
    if(magnitude<Unsorted::LevelHeight && distance_to_unit()<128) {
        if(unit->AttachedTag)unit->AttachedTag->RaiseEvent(TriggerEvent::EnteredBy,this,CellStruct::Empty);
        unit->SendToFirstLink(RadioCommand::NotifyUnlink);
        unit->Disappear(false);
        if(AttachedTag && AttachedTag->ShouldReplace())unit->AttachTrigger(AttachedTag);
        unit->SetOwningHouse(Owner,true);
        unit->HijackerInfantryType=Type->ArrayIndex;
        UnInit();return true;
    }
    if(!Locomotor)std::abort();
    if(distance_to_unit()<512) {
        if(Locomotor->Destination()!=CoordStruct::Empty) {
            const auto towards=Locomotor->Destination();const auto target=unit->GetCoords();
            if(distance(target,towards)<=128)return false;
        }
        // The original makes BOTH calls in this close-range branch.
        SetDestination(Destination,true);
    }else if(Locomotor->Destination()!=CoordStruct::Empty) {
        const auto towards=Locomotor->Destination(),target=unit->GetDestination(this);
        if(short(towards.X/256)==short(target.X/256) && short(towards.Y/256)==short(target.Y/256))return false;
    }
    SetDestination(Destination,true);return false;
}

void InfantryClass::Update() {
    if(TubeIndex>=0){Tunnel_AI();RadarTrackingUpdate(false);return;}
    if((IsBeingWarpedOut() || IsWarpingIn()) && Unsorted::CurrentFrame%24==0) {
        if(RulesClass::Instance->ChronoSparkle1)if(void* storage=YRMemory::Allocate(sizeof(AnimClass))) {
            const CoordStruct at{std::bit_cast<int>(static_cast<unsigned>(Location.X)+120u),
                std::bit_cast<int>(static_cast<unsigned>(Location.Y)+120u),Location.Z};
            ::new(storage) AnimClass(RulesClass::Instance->ChronoSparkle1,at,0,1,0x600,0,false);
        }
    }
    if(TemporalTargetingMe)TemporalTargetingMe->Update();
    if(IsWarpingIn() || (IsBeingWarpedOut() && IsImmobilized)) {
        if(!Locomotor)std::abort();Locomotor->Process();if(!IsAlive)return;
    }
    if(IsBeingWarpedOut()) {
        if(Target)SetTarget(nullptr);
        if(Destination)SetDestination(nullptr,true);
        return;
    }
    if(ReadyToNextMission()) {
        if(GetCurrentMission()==Mission::None && QueuedMission==Mission::None)EnterIdleMode(false,true);
        NextMission();
    }
    if(Health<=0)switch(SequenceAnim) {
        case Sequence::Die1:case Sequence::Die2:case Sequence::Die3:case Sequence::Die4:case Sequence::Die5:
        case Sequence::AirDeathStart:case Sequence::AirDeathFalling:case Sequence::AirDeathFinish:
        case Sequence::WetDie1:case Sequence::WetDie2:break;
        default:Health=1;break;
    }
    FootClass::Update();
    if(!IsAlive)return;
    if(!InLimbo && (GetCurrentMission()==Mission::Guard || GetCurrentMission()==Mission::Area_Guard)) {
        auto* building=MapClass::Instance.GetCellAt(Location)->GetBuilding();
        if(building) {
            bool scatter=!building->Type->InvisibleInGame;
            if(scatter) {
                if(building->Type->LaserFence)scatter=building->LaserFenceFrame!=12 && building->LaserFenceFrame!=8;
                else if(building->Type->FirestormWall)scatter=building->Owner->FirestormActive;
                else if(building->Type->Gate && building->IsTraversable())scatter=false;
            }
            if(!scatter)building=nullptr;
            if((!SlaveOwner || !SlaveOwner->SlaveManager || building!=SlaveOwner
                || !SlaveOwner->SlaveManager->IsSlaveAtCell(this,GetCell())) && building)
                Scatter(CoordStruct::Empty,true,true);
        }
    }
    if(InWhichLayer()!=Layer::Ground)Mark(MarkType::Change);
    if(IsFiring && !Animation.Rate) {
        IsFiring=false;
        PlayAnim(SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle?Sequence::Deployed:Sequence::Ready);
    }
    if(Team && IsInPlayfield)Team->IsLeavingMap=true; // Original Team + 0x82.
    if(!Team && GetCurrentMission()==Mission::Guard && !MapClass::Instance.CoordinatesLegal(Location)) {
        Stun();UnInit();return;
    }
    if(Theft_AI())return;
    if(ReadyToNextMission()) {
        if(GetCurrentMission()==Mission::None && QueuedMission==Mission::None)EnterIdleMode(false,true);
        NextMission();
    }
    Fear_AI();
    if(!Destination && !Crawling && unknown_bool_6DA) {
        int left=unknown_Timer_6C8.TimeLeft;
        if(unknown_Timer_6C8.StartTime!=-1) {
            const int elapsed=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)-static_cast<unsigned>(unknown_Timer_6C8.StartTime));
            left=elapsed>=left?0:std::bit_cast<int>(static_cast<unsigned>(left)-static_cast<unsigned>(elapsed));
        }
        if(!left)unknown_bool_6DA=false;
    }
    Firing_AI();
    if(!IsAlive)return;
    Doing_AI();
    if(IsAlive)Movement_AI();
}
