// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Per_Cell_Process / Try_To_Cloak / Revealed.
// YR 0x6F5090 / 0x6F4EB0 / 0x6F4960, including Temporal and observer-house changes.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/TacticalClass.h"
#include <bit>

bool TechnoClass::vt_entry_2B0() const {
    const auto at=Location;CellStruct projected;
    TacticalClass::AdjustCellForHeight(&projected,&at);
    return projected!=GetMapCoords();
}

// OpenTS Is_Ready_To_Random_Animate; YR tests the raw countdown for zero,
// including a stopped timer. Timer::Expired also accepts negative values.
bool TechnoClass::IsItTimeForIdleActionYet() const {
    int left=IdleActionTimer.TimeLeft;
    if(IdleActionTimer.StartTime!=-1) {
        const int elapsed=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)
            -static_cast<unsigned>(IdleActionTimer.StartTime));
        left=elapsed>=left?0:std::bit_cast<int>(static_cast<unsigned>(left)-static_cast<unsigned>(elapsed));
    }
    return left==0;
}

bool TechnoClass::EnterIdleMode(bool, bool) {
    if(TemporalImUsing && TemporalImUsing->Target)TemporalImUsing->LetGo();
    if(!RefreshMegaMission() && IsNotWarpingIn()) {
        // Inline only 0x6385C0's null-token guard. Non-null planning tokens
        // still require their original event scheduler; never report them done.
        // TODO(RADAR-PLAN-EXEC): next-node execution is deferred by the user;
        // this reachable native unsupported entry remains an explicit gap.
        if(PlanningToken)TryNextPlanningTokenNode();
    }
    return false;
}

int TechnoClass::GetThreatValue() const {
    const auto* type=GetTechnoType();
    if(!type)return 0;
    if(WhatAmI()!=AbstractType::Building)return type->ThreatPosed;
    if(GetOccupantCount()>0)
        return std::bit_cast<int>(static_cast<unsigned>(RulesClass::Instance->ThreatPerOccupant)
            *static_cast<unsigned>(GetOccupantCount()));
    return BunkerLinkedItem?BunkerLinkedItem->GetTechnoType()->ThreatPosed:type->ThreatPosed;
}
void TechnoClass::AddThreatToCell(CellClass* cell) {
    if(cell){const int threat=GetThreatValue();ThreatPosed=static_cast<unsigned>(threat);cell->UpdateThreat(GetOwningHouseIndex(),threat);}
}
void TechnoClass::RemoveThreatFromCell(CellClass* cell) {
    if(cell){cell->UpdateThreat(GetOwningHouseIndex(),std::bit_cast<int>(0u-ThreatPosed));ThreatPosed=0;}
}
void TechnoClass::UpdateThreatInCell(CellClass* cell) {
    if(cell){RemoveThreatFromCell(cell);AddThreatToCell(cell);}
}

void TechnoClass::UpdatePosition(PCPType reason) {
    if(reason!=PCPType::End)return;
    if(TemporalImUsing && TemporalImUsing->Target)TemporalImUsing->LetGo();
    const auto coords=GetCoords();
    const CellStruct at{short(coords.X/256),short(coords.Y/256)};
    Sensed();
    if(AttachedTag)AttachedTag->RaiseEvent(TriggerEvent::ComesNearWaypoint,this,CellStruct::Empty);
    if(!IsInPlayfield && MapClass::Instance.IsWithinUsableArea(at,true))IsInPlayfield=true;
    if(!DiscoveredByCurrentPlayer
        && (MapClass::Instance.GetCellAt(at)->AltFlags & AltCellFlags::NoFog)!=AltCellFlags{})
        DiscoveredBy(HouseClass::CurrentPlayer);
    MapClass::Instance.GetCellAt(at)->ActivateVeins();
}

void TechnoClass::Sensed() {
    const auto coords=GetCoords();
    auto* cell=MapClass::Instance.GetCellAt(CellStruct{short(coords.X/256),short(coords.Y/256)});
    auto* player=HouseClass::CurrentPlayer;
    if(CloakState==::CloakState::Cloaked && player && Owner!=player
        && !cell->Sensors_InclHouse(player->ArrayIndex))Deselect();
    if(!cell->CloakGen_InclHouse(Owner->ArrayIndex) || !IsReadyToCloak())return;
    DynamicVectorClass<TechnoClass*> reacquire;
    for(int i=Array.Count-1;i>=0;--i) {
        auto* attacker=Array[i];
        if(attacker->Target!=this)continue;
        // The target re-queries virtual coordinates for each attacker.
        auto* here=MapClass::Instance.GetCellAt(GetCoords());
        if(here->Sensors_InclHouse(attacker->Owner->ArrayIndex) || attacker->Owner==Owner)
            reacquire.AddItem(attacker);
    }
    Cloak(false);
    for(int i=reacquire.Count-1;i>=0;--i)reacquire[i]->SetTarget(this);
}

bool TechnoClass::DiscoveredBy(HouseClass* house) {
    if(house==HouseClass::CurrentPlayer) {
        if(DiscoveredByCurrentPlayer)return false;
    }else {
        if(DiscoveredByComputer)return false;
        DiscoveredByComputer=true;
    }
    if(!ObjectClass::DiscoveredBy(house))return false;
    // YR asks the OBSERVER house here, unlike OpenTS's owner-house predicate.
    if(!house->IsControlledByHuman() && GetCurrentMission()==Mission::Ambush)
        QueueMission(Mission::Hunt,false);
    if(house!=HouseClass::CurrentPlayer){DiscoveredByComputer=true;return true;}
    DiscoveredByCurrentPlayer=true;
    Owner->RecheckPower=true;Owner->RecheckRadar=true;
    if(IsOwnedByCurrentPlayer)return true;
    if(!Unsorted::ScenarioInit && AttachedTag)
        AttachedTag->RaiseEvent(TriggerEvent::DiscoveredByPlayer,this,CellStruct::Empty);
    Owner->DiscoveredByPlayer=true;
    return true;
}
