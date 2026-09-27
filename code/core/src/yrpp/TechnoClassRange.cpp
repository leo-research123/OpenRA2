// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp / techtype.cpp::In_Range; YR moves the
// coordinate/weapon overload to TechnoClass (0x6F7220), not TechnoTypeClass.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/YRMath.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>

namespace {
int elevation_bonus(const TechnoClass& actor,AbstractClass& target,bool horizontal){
    // YR 0x6F6F60 / 0x6F70E0. The former decompilation loses sqrt's
    // return value: assembly 0x6F70B2..0x6F70BA returns its truncation.
    if(!actor.IsOnFloor()||!target.IsOnFloor())return 0;
    const int levels=std::max(0,MapClass::Instance.GetCellAt(actor.GetCoords())->GetLevel()
        -MapClass::Instance.GetCellAt(target.GetCoords())->GetLevel());
    const auto& rules=*RulesClass::Instance;
    if(!rules.ElevationIncrement)std::abort(); // incomplete original Rules load
    const int bonus=rule_integer(std::min(double(levels/rules.ElevationIncrement)*rules.ElevationIncrementBonus,rules.ElevationBonusCap));
    const unsigned x=unsigned(bonus)*256u;
    if(horizontal)return std::bit_cast<int>(x);
    const unsigned z=unsigned(levels)*unsigned(Unsorted::LevelHeight);
    return rule_integer(Math::sqrt(double(std::bit_cast<int>(x*x+z*z))));
}
int distance(const CoordStruct& a,const CoordStruct& b,bool horizontal=false){
    const double x=double(a.X)-b.X,y=double(a.Y)-b.Y,z=horizontal?0.0:double(a.Z)-b.Z;
    return rule_integer(Math::sqrt(x*x+z*z+y*y));
}
bool trajectory_valid(int speed,int horizontal,int height,double gravity){
    // 0x48ABC0: unlike OpenTS, the speed used here is calculated from the
    // effective range (0x48AB90), not the weapon's stored projectile speed.
    const double v=double(speed),x=horizontal?double(horizontal):0.001,y=double(height);
    const double vv=v*v,xx=x*x;
    const double discriminant=v*(vv*v)-(vv*y*gravity+vv*y*gravity)-gravity*gravity*xx;
    if(discriminant<0.0)return false;
    const double divisor=y*y/xx+1.0+y*y/xx+1.0;
    if(divisor==0.0)return false;
    return (Math::sqrt(discriminant)+vv-y*gravity)/divisor>=0.0
        ||(vv-y*gravity-Math::sqrt(discriminant))/divisor>=0.0;
}
}
bool TechnoClass::IsCloseEnough(AbstractClass* target,int index) const {
    if(!target)return true;
    auto source=GetCoords();
    const auto* slot=GetWeapon(index);const auto* weapon=slot?slot->WeaponType:nullptr;
    if(weapon&&weapon->CellRangefinding){
        source=MapClass::Instance.GetCellAt(GetCoords())->GetCoords();
        if(OnBridge)source.Z+=CellClass::BridgeHeight;
    }
    if(IsInAir())source.Z=target->GetCoords().Z;
    return IsCloseEnough(source,target,weapon);
}
bool TechnoClass::IsCloseEnough(const CoordStruct& source,AbstractClass* target,const WeaponTypeClass* weapon) const {
    if(!target||!weapon)return false;
    int range=weapon->Range;if(range==-512)return true;
    if(target->IsInAir())range+=GetTechnoType()->AirRangeBonus;
    if(CanOccupyFire())range=(RulesClass::Instance->OccupyWeaponRange+GetOccupyRangeBonus())*256;
    if(BunkerLinkedItem&&WhatAmI()!=AbstractType::Building)range+=RulesClass::Instance->BunkerWeaponRangeBonus*256;
    if(InOpenToppedTransport)range+=RulesClass::Instance->OpenToppedRangeBonus*256;
    const auto* projectile=weapon->Projectile;
    int effective=range;
    if(projectile->SubjectToElevation)effective+=elevation_bonus(*this,*target,false);
    auto destination=target->GetCoords();
    if(target->IsOnFloor()){
        destination.Z=MapClass::Instance.GetCellFloorHeight(destination);
        if(MapClass::Instance.GetCellAt(destination)->ContainsBridgeEx())destination.Z+=CellClass::BridgeHeight;
    }
    if(weapon->MinimumRange&&distance(source,destination)<weapon->MinimumRange)return false;
    if(projectile->Arcing){
        int horizontalRange=range;
        if(projectile->SubjectToElevation)horizontalRange+=elevation_bonus(*this,*target,true);
        const int horizontal=distance(source,destination,true),height=destination.Z-source.Z;
        if(horizontal>horizontalRange)return false;
        const double gravity=double(RulesClass::Instance->Gravity)*(projectile->Floater?0.5:1.0);
        const int speed=rule_integer(Math::sqrt(double(horizontalRange)*gravity*1.2));
        if(!trajectory_valid(speed,horizontal,height,gravity))return false;
        if(MapClass::Instance.GetCellAt(destination)->ContainsBridgeEx()&&height>=3*Unsorted::LevelHeight)return false;
    }else{
        if(target->WhatAmI()==AbstractType::Building){
            const auto* type=static_cast<BuildingClass*>(target)->Type;
            effective+=(type->GetFoundationWidth()+type->GetFoundationHeight(false))*64;
        }
        // The target checks RTTI 3 (AircraftType), not RTTI 2 (Aircraft).
        if(distance(source,destination,WhatAmI()==AbstractType::AircraftType)>effective)return false;
        if(MapClass::Instance.GetCellAt(source)->ContainsBridgeEx()){
            const int bridge=CellClass::BridgeHeight+MapClass::Instance.GetCellFloorHeight(source);
            if(source.Z<bridge&&destination.Z>=bridge)return false;
        }
    }
    return !TrajectoryHelper::FindFirstImpenetrableObstacle(source,destination,weapon,Owner);
}
