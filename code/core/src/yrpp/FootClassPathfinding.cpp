// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 findpath.cpp Find_Path / foot.cpp Basic_Path;
// YR 0x4CBBA0 wrapper and 0x4D3920 int direction cache.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/UnitClass.h"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
PathFinderData* FootClass::FindPath(CellStruct* end,int* directions,int,int,int offset,int mode) {
    CoordStruct location;GetDestination(&location,nullptr);
    CellStruct start{short(location.X/256),short(location.Y/256)};
    if(!directions)return nullptr;
    AStarClass::FollowPath(&start,&start,offset,PathDirections);
    return AStarClass::Instance.FindPath(&start,end,this,directions,-1,MovementZone::None,mode);
}

bool FootClass::UpdatePathfinding(CellStruct destination,int offset,int mode) {
    // Basic_Path's original 24-entry cache, not a second pathfinding algorithm.
    if(offset<0 || offset>=24){PathDirections[0]=-1;return false;}
    if(!offset)PathDirections[0]=-1;
    const CoordStruct destination_coord{destination.X*256+128,destination.Y*256+128,0};
    if(!IsInSameZoneAsCoords(destination_coord)){PathDirections[0]=-1;return false;}
    auto& map=MapClass::Instance;auto* type=GetTechnoType();auto& rules=*RulesClass::Instance;
    const auto here=GetCoords();
    const auto distance=[](const CoordStruct& a,const CoordStruct& b){const double x=double(a.X)-b.X,y=double(a.Y)-b.Y,z=double(a.Z)-b.Z;return int(Math::sqrt(x*x+y*y+z*z));};
    const int dist=distance(here,destination_coord);
    const int check_distance=Team?Team->GetStrayDistance():rules.CloseEnough;
    auto* cell=map.GetCellAt(destination);
    const auto move=IsCellOccupied(cell,FacingType::None,-1,nullptr,true);
    if(!type->IsTrain && ((move==Move::Temp && dist>check_distance)
        || (move==Move::No && cell->GetBuilding()))) {
        constexpr int simplified[13]{0,0,0,5,5,5,6,7,7,9,10,11,0};
        const auto movement=static_cast<MovementZone>(simplified[static_cast<int>(type->MovementZone)]);
        const bool burrow=type->IsSubterranean && cell->CanBurrowHere();
        const auto at=GetMapCoords();
        auto nearby=map.NearByLocation(destination,type->SpeedType,map.GetMovementZoneType(at,movement,OnBridge),movement,
            OnBridge,1,1,false,true,burrow,true,at,false,false);
        const auto under=[](const CellClass* c){return (static_cast<unsigned>(c->Flags)&0x100u)!=0;};
        if(move==Move::No || (nearby!=CellStruct::Empty
            && distance(destination_coord,{nearby.X*256+128,nearby.Y*256+128,0})<dist
            && AStarClass::Instance.AttemptPath(&nearby,&destination,this,under(map.GetCellAt(nearby)),under(cell),
                burrow?MovementZone::Normal:MovementZone::None)<=std::max(std::abs(nearby.X-destination.X),std::abs(nearby.Y-destination.Y))+6)) {
            SetDestination(map.GetCellAt(nearby),true);destination=nearby;
        }
    }
    Mark(MarkType::Up);
    int directions[2002]{};
    auto* path=FindPath(&destination,directions,2000,0,offset,mode);
    if(path && path->TotalDistance){
        auto fixed=*path;vt_entry_540(&fixed);
        std::memcpy(PathDirections+offset,directions,sizeof(int)*std::clamp(path->PathLength,0,24-offset));
    }
    Mark(MarkType::Down);PathDelayTimer.Start(0);
    if(WhatAmI()==AbstractType::Unit && PathDirections[0]!=-1){
        auto* unit=static_cast<UnitClass*>(this);auto* follower=unit->FollowerCar;
        if(follower && !unit->IsFollowerCar){
            const int length=path?path->PathLength-2:0;
            auto next=destination;
            while(follower){
                next=map.GetCellAt(next)->GetNeighbourCell(static_cast<FacingType>((directions[length]+4)%8))->MapCoords;
                const auto dest=follower->Destination?follower->Destination->GetDestination():CoordStruct::Empty;
                if(!follower->Destination || CellStruct{short(dest.X/256),short(dest.Y/256)}!=next){
                    follower->QueueMission(Mission::Move,false);follower->SetDestination(map.GetCellAt(next),true);
                    follower->UpdatePathfinding(next,0,0);
                }
                follower=follower->FollowerCar;
            }
        }
    }
    if(path){CurrentMapCoords=GetMapCoords();return true;}
    PathDelayTimer.Start(int(rules.PathDelay*900.0));StopMoving();
    const auto at=GetMapCoords();
    if(std::max(std::abs(at.X-destination.X),std::abs(at.Y-destination.Y))>1
        || (!OnBridge && (static_cast<unsigned>(map.GetCellAt(destination)->Flags)&0x100u))) {
        if(Team){Locomotor->Lock();Team->LiberateMember(this,-1,0);Locomotor->Unlock();}
        SetDestination(nullptr,true);SetTarget(nullptr);
        if(Owner->IsControlledByHuman())QueueMission(Mission::Guard,false);
        else {
            QueueMission(Mission::Area_Guard,false);
            const auto& runtime=game::scenario_runtime();
            if(runtime.session_mode(runtime.context)!=static_cast<int>(GameMode::Campaign)){
                CellStruct where;Owner->WhereToGo(&where,this);
                if(where!=CellStruct::Empty)SetDestination(map.GetCellAt(where),true);
            }
        }
    }
    return false;
}
