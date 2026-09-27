// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 reinf.cpp Do_Reinforcements/Create_Group/Place_Reinforcements.
// YR 0x65D8E0/0x65DD30/0x65E010: explicit map waypoint reinforcement path.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_loading.hpp"
#include "yrpp/TeamClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/HouseClass.h"
#include <memory>
#include <vector>
namespace game {
bool reinforce_team(TeamTypeClass& type,int waypoint) noexcept {try{
    if(!type.Owner||!type.TaskForce||!type.ScriptType||type.Loadable)return false;
    const auto cell=ScenarioClass::Instance->GetWaypointCoords(waypoint==-1?type.Waypoint:waypoint);
    auto* spawn=MapClass::Instance.TryGetCellAt(cell);if(!spawn)return false;
    std::unique_ptr<TeamClass> team(GameCreate<TeamClass>(&type,type.Owner,0));
    if(!team||!team->CurrentScript)return false;
    team->IsForcedActive=true;team->IsUnderStrength=false;
    std::vector<std::unique_ptr<FootClass>> members;
    const int previous=Unsorted::ScenarioInit;
    struct Restore{int depth;~Restore(){Unsorted::ScenarioInit=depth;}}restore{previous};
    ++Unsorted::ScenarioInit;
    for(int i=0;i<type.TaskForce->CountEntries;++i){
        const auto& entry=type.TaskForce->Entries[i];if(!entry.Type)return false;
        for(int n=0;n<entry.Amount;++n){
            auto* object=entry.Type->CreateObject(type.Owner);if(!object)return false;
            if(!(static_cast<unsigned>(object->AbstractFlags)&static_cast<unsigned>(AbstractFlags::Foot))){delete object;return false;}
            std::unique_ptr<FootClass> foot(static_cast<FootClass*>(object));
            if(foot->WhatAmI()==AbstractType::Unit&&!static_cast<UnitClass*>(foot.get())->InitializeLocomotor())return false;
            if(foot->WhatAmI()==AbstractType::Infantry&&!static_cast<InfantryClass*>(foot.get())->InitializeLocomotor())return false;
            if(foot->WhatAmI()==AbstractType::Aircraft&&!static_cast<AircraftClass*>(foot.get())->InitializeLocomotor())return false;
            auto at=spawn->GetCoords();
            const auto facing=foot->WhatAmI()==AbstractType::Aircraft?DirType::South:DirType::North;
            if(!type.Droppod&&!foot->Unlimbo(at,facing))return false;
            if(type.VeteranLevel==2)foot->Veterancy.SetVeteran();
            else if(type.VeteranLevel==3)foot->Veterancy.SetElite();
            members.push_back(std::move(foot));
        }
    }
    for(auto& foot:members){if(!team->AddMember(foot.get(),false)){while(team->FirstUnit)team->LiberateMember(team->FirstUnit);return false;}foot->IsInitiated=true;}
    if(type.Droppod){
        // YR 0x65D8E0 replaces TS drop pods with a PDPLANE carrying the
        // existing team. Cargo remains in limbo until ParadropCargo places it.
        auto* planeType=AircraftTypeClass::Find("PDPLANE");if(!planeType)return false;
        std::unique_ptr<AircraftClass> plane(GameCreate<AircraftClass>(planeType,type.Owner));
        if(!plane)return false;plane->IsALoaner=true;
        CellStruct origin;
        if(type.UseTransportOrigin)origin=ScenarioClass::Instance->GetWaypointCoords(type.TransportWaypoint);
        else{
            auto edge=type.Owner->StartingEdge;
            if(unsigned(edge)>=4)edge=unsigned(type.Owner->Edge)<4?type.Owner->Edge:Edge::North;
            origin=MapClass::Instance.PickCellOnEdge(edge,CellStruct::Empty,CellStruct::Empty,SpeedType::Winged,true,MovementZone::Normal);
        }
        auto* entry=MapClass::Instance.TryGetCellAt(origin);if(!entry)return false;
        plane->QueueMission(Mission::ParadropApproach,false);plane->SetDestination(nullptr,true);plane->SetTarget(spawn);
        if(!plane->Unlimbo(entry->GetCoords(),DirType::North))return false;
        plane->HasPassengers=true;
        for(auto& member:members)plane->Passengers.AddPassenger(member.get());
        plane->NextMission();plane.release();
    }
    for(auto& foot:members)foot.release();
    team.release();return true;
}catch(...){return false;}
}
}
