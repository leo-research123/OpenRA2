// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 aircraft.cpp waypoint Move and AI; YR 0x4166C0/0x414BB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AircraftClass.h"
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_world.hpp"
bool AircraftClass::ReadyToNextMission() const {
    return CurrentMission!=Mission::Sticky&&CurrentMission!=Mission::Rescue&&
        (!IsLocked||CurrentMission==Mission::AttackMove)&&IsCarryallNotLanding;
}
void AircraftClass::Update(){
    if(!IsAlive||InLimbo)return;
    if(ReadyToNextMission())NextMission();
    FootClass::Update();
    if(!IsAlive||InLimbo)return;
    // YR 0x414BB0/0x41B890: a flyover team is removed after entering and
    // leaving the playable rectangle, not upon touching its final waypoint.
    if(((!Type->FlyBy&&!Type->FlyBack&&!MapClass::Instance.IsWithinUsableArea(GetMapCoords(),true))
        ||!MapClass::Instance.CoordinatesLegal(GetMapCoords()))&&IsLeavingMap()){
        UnInit();return;
    }
    if(ReadyToNextMission())NextMission();
}
bool AircraftClass::IsLeavingMap() const {
    if((Target&&CurrentMission!=Mission::AttackMove)||!IsInPlayfield||
        GetCurrentMission()==Mission::ParadropApproach||GetCurrentMission()==Mission::ParadropOverfly)return false;
    return Team?Team->IsLeavingMapNow():IsALoaner;
}
void AircraftClass::SetDestination(AbstractClass* target,bool immediate){
    // Ordinary map waypoint destinations use the Foot navigation state;
    // landing-pad radio negotiation is a separate aircraft mission branch.
    FootClass::SetDestination(target,immediate);
}
int AircraftClass::Mission_Move(){
    switch(MissionStatus){
    case 0:if(Destination){SetDestination(Destination,true);MissionStatus=1;}else EnterIdleMode(false,true);break;
    case 1:
        if(Destination){CoordStruct at;Destination->GetDestination(&at,this);Locomotor->Move_To(at);MissionStatus=2;}
        else EnterIdleMode(false,true);
        break;
    case 2:case 4:
        if(!Locomotor->Is_Moving()||!Destination||GetMapCoords()==CellClass::Coord2Cell(Destination->GetDestination(this)))MissionStatus=3;
        break;
    case 3:if(!Locomotor->Is_Moving())EnterIdleMode(false,true);break;
    }
    if(MissionStatus==1)return int(CurrentMissionControl()->Rate*900.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
    return 1;
}
bool AircraftClass::EnterIdleMode(bool initial,bool resume){
    // 0x4176F0 preserves a queued paradrop before Foot::Unlimbo's idle pass.
    if((CurrentMission==Mission::Retreat||CurrentMission==Mission::ParadropApproach||CurrentMission==Mission::ParadropOverfly)&&!TemporalTargetingMe)return false;
    if(QueuedMission==Mission::ParadropApproach){NextMission();return false;}
    const bool result=FootClass::EnterIdleMode(initial,resume);
    if(Destination){QueueMission(Mission::Move,false);return result;}
    QueueMission(Mission::Guard,false);
    return result;
}
