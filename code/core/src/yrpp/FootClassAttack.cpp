// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::Approach_Target; YR 0x4D5690.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
AbstractClass* FootClass::GreatestThreat(ThreatType threat,CoordStruct* origin,bool onlyEnemy) {
    unsigned flags=unsigned(threat);
    if(IsScanLimited)flags=(flags&~2u)|1;
    return TechnoClass::GreatestThreat(ThreatType(flags),origin,onlyEnemy);
}
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/YRMath.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>

namespace {
int add(int a,int b){return std::bit_cast<int>(unsigned(a)+unsigned(b));}
int distance(const CoordStruct& from,const CoordStruct& to,bool spatial) {
    const double x=double(from.X)-to.X,y=double(from.Y)-to.Y,z=spatial?double(from.Z)-to.Z:0.0;
    return rule_integer(Math::sqrt(x*x+y*y+z*z));
}
int cell_distance(const CellStruct& a,const CellStruct& b){return std::max(std::abs(int(a.X)-b.X),std::abs(int(a.Y)-b.Y));}
struct PersistRef {
    IPersist* pointer=nullptr;
    explicit PersistRef(ILocomotion* driver) {
        constexpr GUID iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
        if(!driver)std::abort();
        const auto result=driver->QueryInterface(iid,reinterpret_cast<void**>(&pointer));
        if((result<0 && result!=static_cast<HRESULT>(0x80004002u)) || !pointer)std::abort();
    }
    ~PersistRef(){pointer->Release();}
};
// Exact YR 0x8224DC table; OpenTS has only the coarser final 13 entries.
constexpr int angles[]{0,1,-1,2,-2,3,-3,4,-4,5,-5,6,-6,8,-8,16,-16,24,-24,32,-32,48,-48,64,-64};
}

AbstractClass* FootClass::ApproachTarget(DWORD query) {
    const bool queryOnly=static_cast<unsigned char>(query)!=0;
    if(!Target || (LocomotorTarget && LocomotorTarget==Target))return nullptr;
    const int weapon=SelectWeapon(Target);
    int range=GetWeaponRange(weapon);
    if(range>=512)range-=128;
    const bool inRange=IsCloseEnough(Target,weapon);
    auto* type=GetTechnoType();
    const bool mayApproach=type->CanApproachTarget && !DrainTarget && !BunkerLinkedItem && !InOpenToppedTransport;
    const bool explicitAttack=CurrentMission==Mission::Attack || (CurrentMission==Mission::Area_Guard && Owner->IsControlledByHuman());
    if(!inRange && (CurrentMission==Mission::Sticky
        || (!mayApproach && (CurrentMission!=Mission::Hunt || Owner->IsControlledByHuman()) && !explicitAttack))) {
        if(!queryOnly){SetTarget(nullptr);SetDestination(nullptr,true);}
        return nullptr;
    }
    bool requireBuildable=false;
    if(WhatAmI()==AbstractType::Unit && static_cast<UnitClass*>(this)->Type->DeployToFire
        && !MapClass::Instance.GetCellAt(Location)->CanBuildHere()) {
        requireBuildable=true;range=std::min(range,add(DistanceFrom3D(Target),512));
    }
    PersistRef persist(Locomotor);CLSID clsid{};persist.pointer->GetClassID(&clsid);
    const bool flyer=WhatAmI()==AbstractType::Aircraft || !std::memcmp(&clsid,&LocomotionClass::CLSIDs::Jumpjet,sizeof(clsid));
    if(Destination) {
        if(Target && type->CanRecalcApproachTarget && !inRange
            && distance(Destination->GetCoords(),Target->GetCoords(),true)
                >std::bit_cast<int>(unsigned(range)*unsigned(RulesClass::Instance->ApproachTargetResetMultiplier)) && !queryOnly) {
            unknown_5A0=nullptr;Destination=nullptr; // YR deliberately does not call SetDestination here.
        }
        if(Destination && !IsInAir())return nullptr;
    }
    if(inRange && IsInPlayfield && !requireBuildable)return nullptr;
    if(unknown_abstract_array_588.Count>0) {
        auto* next=unknown_abstract_array_588[0];
        if(!queryOnly){SetDestination(next,false);unknown_abstract_array_588.RemoveItem(0);}
        return next;
    }
    if(Target->WhatAmI()==AbstractType::Cell && static_cast<CellClass*>(Target)->Tile_Is_DestroyableCliff()) {
        CellStruct next;
        MapClass::Instance.ClosestPassableCell(&next,static_cast<CellClass*>(Target)->MapCoords,CellClass::Coord2Cell(GetDestination()));
        if(!queryOnly)SetTarget(MapClass::Instance.GetCellAt(next));
    }
    if(Target->WhatAmI()==AbstractType::Building) {
        const auto* building=static_cast<BuildingClass*>(Target)->Type;
        range=add(range,(building->GetFoundationWidth()+building->GetFoundationHeight(false))*64);
    }
    range=std::max(0,rule_integer(double(range)-(WhatAmI()==AbstractType::Infantry?281.6:179.2)));
    if(range>0 && range<=256)range=256;
    auto targetCoords=Target->GetCoords();
    const auto targetCell=CellClass::Coord2Cell(targetCoords);
    auto candidateCell=targetCell;
    const auto own=GetCoords();
    const int direction16=rule_integer((Math::atan2(double(targetCoords.Y)-own.Y,double(own.X)-targetCoords.X)
        -1.570796326794897)*-10430.06004058427);
    const auto direction=static_cast<unsigned char>(((static_cast<unsigned short>(direction16)>>7)+1)>>1);
    bool targetBridge=false;
    if(Target->WhatAmI()==AbstractType::Cell)targetBridge=static_cast<CellClass*>(Target)->ContainsBridgeEx();
    else if((Target->AbstractFlags & ::AbstractFlags::Object)!=::AbstractFlags::None)targetBridge=static_cast<ObjectClass*>(Target)->OnBridge;
    if(WhatAmI()==AbstractType::Infantry && type->CloseRange && double(range)<332.8)range=332;
    bool found=false;
    if(type->CloseRange) {
        auto here=GetCell()->GetCoords();
        auto there=(Target->AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None
            ?static_cast<TechnoClass*>(Target)->GetCell()->GetCoords():Target->GetCoords();
        const int oldDistance=distance(MapClass::Instance.GetCellAt(here)->GetCoords(),MapClass::Instance.GetCellAt(there)->GetCoords(),false);
        if(double(oldDistance)<=384.0) {
            const auto spot=GetCell()->FindInfantrySubposition(Target->GetCoords(),false,false,true);
            if(distance(spot,there,false)<oldDistance) {
                if(!queryOnly)Locomotor->Force_Immediate_Destination(spot);
                found=true;
            }
        }
    }
    auto& map=MapClass::Instance;
    if(Target->IsOnFloor()) {
        targetCoords.Z=map.GetCellFloorHeight(targetCoords);
        if(map.GetCellAt(targetCoords)->ContainsBridgeEx())targetCoords.Z+=CellClass::BridgeHeight;
    }
    const auto movement=type->MovementZone;
    const auto clear=[&](const CellStruct& cell) {
        // The decompiler loses Mark's stack pop and aliases these locals.
        // 0x4D62CA / 0x4D6375 remove/reapply occupation around the SAME
        // candidate query; neither the cell's Y nor the direction is overwritten.
        if(type->CloseRange)Mark(MarkType::Up);
        const int zone=map.GetMovementZoneType(CellClass::Coord2Cell(GetDestination()),movement,OnBridge);
        const bool pass=map.GetCellAt(cell)->IsClearToMove(type->SpeedType,false,false,zone,movement,-1,true);
        if(type->CloseRange)Mark(MarkType::Down);
        return pass;
    };
    const auto canFire=[&](const CoordStruct& spot) {
        return IsCloseEnough(spot,Target,GetWeapon(weapon)->WeaponType)
            || (type->CloseRange && double(distance(spot,Target->GetCoords(),false))<307.2);
    };
    for(int radius=range;!found && double(radius)>204.8;radius-=256) {
        for(const int offset:angles) {
            const auto facing=std::bit_cast<short>(static_cast<unsigned short>((int(direction)+offset)*256));
            const double radians=(int(facing)-0x3FFF)*-0.00009587672516830327;
            CoordStruct spot{rule_integer(Math::cos(radians)*radius+double(targetCoords.X)),
                rule_integer(double(targetCoords.Y)-Math::sin(radians)*radius),targetCoords.Z};
            if(WhatAmI()==AbstractType::Infantry && type->CloseRange)
                spot=map.GetCellAt(spot)->FindInfantrySubposition(Target->GetCoords(),false,false,true);
            else spot=CellClass::Cell2Coord(CellClass::Coord2Cell(spot));
            if(requireBuildable && !map.GetCellAt(spot)->CanBuildHere())continue;
            spot.Z=map.GetCellFloorHeight(spot);
            if(map.GetCellAt(spot)->ContainsBridgeEx())spot.Z+=CellClass::BridgeHeight;
            if(!canFire(spot))continue;
            candidateCell=CellClass::Coord2Cell(spot);
            if(!map.IsWithinUsableArea(candidateCell,true))continue;
            bool pass=clear(candidateCell);
            if(!pass) {
                if(WhatAmI()==AbstractType::Infantry)continue;
                const auto savedCell=candidateCell;
                // Original walks cumulatively through adjacent offsets; it
                // does not sample eight neighbors of a fixed centre.
                for(int i=0;i<8;++i) {
                    candidateCell.X=short(candidateCell.X+Unsorted::AdjacentCell[i].X);
                    candidateCell.Y=short(candidateCell.Y+Unsorted::AdjacentCell[i].Y);
                    const auto neighbor=CellClass::Cell2Coord(candidateCell);
                    if(canFire(neighbor) && clear(candidateCell)){pass=true;break;}
                }
                if(!pass){candidateCell=savedCell;continue;}
            }
            auto* cell=map.GetCellAt(candidateCell);
            if(flyer){found=true;break;}
            auto target=targetCell;
            if(AStarClass::Instance.AttemptPath(&candidateCell,&target,this,cell->ContainsBridgeEx(),targetBridge)
                <=cell_distance(candidateCell,target)+8){found=true;break;}
            auto from=CellClass::Coord2Cell(GetDestination());
            if(AStarClass::Instance.AttemptPath(&from,&candidateCell,this,IsOnBridge(nullptr),cell->ContainsBridgeEx())
                <=cell_distance(from,candidateCell)+8){found=true;break;}
        }
    }
    if(found) {
        if(!queryOnly)SetDestination(map.GetCellAt(candidateCell),true);
        return map.GetCellAt(candidateCell);
    }
    if(type->HunterSeeker || (WhatAmI()==AbstractType::Infantry && static_cast<InfantryClass*>(this)->Type->VehicleThief)) {
        if(!queryOnly)SetDestination(Target,true);
        return Target;
    }
    const int zone=map.GetMovementZoneType(CellClass::Coord2Cell(GetDestination()),movement,IsOnBridge(nullptr));
    candidateCell=map.NearByLocation(candidateCell,type->SpeedType,zone,movement,map.GetCellAt(candidateCell)->ContainsBridgeEx(),
        1,1,false,true,false,false,CellStruct::Empty,false,false);
    if(!queryOnly){SetTarget(nullptr);SetDestination(candidateCell==CellStruct::Empty?nullptr:map.GetCellAt(candidateCell),true);}
    return nullptr;
}
