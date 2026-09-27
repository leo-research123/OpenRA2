// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Greatest_Threat/Target_Something_Nearby.
// YR 0x6F8DF0/0x709820/0x709550/0x707E60. EA Section 7: third_party/opents/LICENSE.md.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
#include "yrpp/TechnoClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>

int TechnoClass::GetGuardRange(int control) const {
    if(control==-1)return -1;
    auto* type=GetTechnoType();
    if(!control)return !IsEngineer()?type->GuardRange:0;
    const int range=2*(type->GuardRange?type->GuardRange:std::max(GetWeaponRange(0),GetWeaponRange(1)));
    return std::clamp(range,control==2?1792:0,4096);
}
AbstractClass* TechnoClass::GreatestThreat(ThreatType threat,CoordStruct* origin,bool onlyEnemy) {
    auto* type=GetTechnoType();const bool human=Owner->IsControlledByHuman();
    if(type->NoAutoFire&&human)return nullptr;
    unsigned flags=unsigned(threat);auto& map=MapClass::Instance;int zone=-1;
    if(!(flags&1)&&WhatAmI()!=AbstractType::Building&&WhatAmI()!=AbstractType::Aircraft)
        zone=map.GetMovementZoneType(GetMapCoords(),type->MovementZone,IsOnBridge(nullptr));
    const bool infantry=WhatAmI()==AbstractType::Infantry;
    if(infantry) {
        if(CombatDamage(-1)<0)flags=(flags&3)|0x4008;
        else if(static_cast<InfantryClass*>(this)->Type->Engineer)flags&=~0x18u;
    }else if(WhatAmI()==AbstractType::Unit&&CombatDamage(-1)<0)flags=(flags&3)|0x4010;
    int mask=0;
    if(flags&0x100)mask=0x8042;
    if(flags&4)mask|=4;
    if(flags&0x1BA60)mask|=0x40;
    if(flags&8)mask|=0x8000;
    if(flags&0x50)mask|=2;
    if(type->DistributedFire){CurrentTargets.Clear();CurrentTargetThreatValues.Clear();}
    const bool controlledTransport=Transporter&&Transporter->GetTechnoType()->OpenTopped&&Transporter->MindControlledBy;
    const bool friendly=type->AttackFriendlies||Berzerk||controlledTransport;
    const bool engineer=infantry&&static_cast<InfantryClass*>(this)->Type->Engineer;
    const auto eligible=[&](TechnoClass* target) {
        return (!onlyEnemy||target->Owner->ArrayIndex==Owner->EnemyHouseIndex)&&(!(flags&0x4000)||Owner->IsAlliedWith(target->Owner));
    };
    AbstractClass* best=nullptr;int highest=-1;
    const auto record=[&](TechnoClass* target,int value,bool collect) {
        if(collect&&type->DistributedFire){CurrentTargets.AddItem(target);CurrentTargetThreatValues.AddItem(value);}
        if(value>highest){highest=value;best=target;}
    };
    if(!(flags&3)) {
        if(mask&4)for(auto* aircraft:AircraftClass::Array) {
            int value=0;
            if((!Owner->IsAlliedWith(aircraft)||friendly)&&eligible(aircraft)
                &&CanAutoTargetObject(ThreatType(flags),mask,-1,aircraft,&value,DWORD(-1),&CoordStruct::Empty))record(aircraft,value,false);
        }
        if(flags&0x10)mask|=4;
        for(auto* target:Array) {
            int value=0;
            if((!Owner->IsAlliedWith(target)||CombatDamage(-1)<0||(!human&&engineer)||friendly)
                &&target->LastLayer==Layer::Ground&&eligible(target)
                &&CanAutoTargetObject(ThreatType(flags),mask,-1,target,&value,DWORD(zone),origin))record(target,value,false);
        }
        return best;
    }
    int range=GetGuardRange(flags&1?0:CurrentMission==Mission::Patrol?2:1);
    if(CombatDamage(-1)<0&&CurrentMission==Mission::Guard)range=512;
    int cells=range/256;
    if(!range) {
        int reach;
        if(type->TurretCount>0&&!type->IsGattling)reach=GetWeaponRange(CurrentWeaponNumber);
        else if(type->Underwater&&type->Organic&&type->SelfHealing)reach=type->GuardRange;
        else reach=std::max(GetWeaponRange(0),GetWeaponRange(1));
        cells=reach/256+type->AirRangeBonus/256+1;
    }
    const CellStruct center{short(origin->X/256),short(origin->Y/256)};
    if(CanOccupyFire())cells=GetOccupyRangeBonus()+RulesClass::Instance->OccupyWeaponRange+1;
    if(flags&4) {
        AircraftTrackerClass::Instance.FillCurrentVector(map.GetCellAt(center),cells);
        while(auto* target=AircraftTrackerClass::Instance.Get()) {
            int value=0;
            if((!Owner->IsAlliedWith(target)||friendly)&&eligible(target)&&target->IsOnMap&&target->InWhichLayer()!=Layer::Ground
                &&CanAutoTargetObject(ThreatType(flags),mask|0x8002,range,target,&value,DWORD(-1),&CoordStruct::Empty))record(target,value,true);
        }
    }
    if(flags&0x10)mask|=4;
    if(flags==5)return best;
    int wallValue=0;CellStruct wall=CellStruct::Empty;
    const auto scan=[&](int x,int y) {
        CellStruct at{short(center.X+x),short(center.Y+y)};if(!map.IsWithinUsableArea2D(at))return;
        TechnoClass* target=nullptr;int value=0;
        if(TryAutoTargetObject(ThreatType(flags),mask,&at,range,&target,&value,zone)&&target&&eligible(target))record(target,value,true);
        if(!best){const int score=GetWallThreat(&at);if(score>wallValue){wallValue=score;wall=at;}}
    };
    for(int radius=0;radius<cells;++radius) {
        // Preserve duplicated center row and the original north/south then
        // west/east ring order: tie-breaking depends on this exact ordering.
        for(int x=-radius;x<=radius;++x){scan(x,-radius);scan(x,radius);}
        for(int y=1-radius;y<radius;++y){scan(-radius,y);scan(radius,y);}
        if(best&&(radius==cells/4||radius==cells/2))return best;
        if(wall!=CellStruct::Empty)return map.GetCellAt(wall);
    }
    return best;
}
void TechnoClass::SelectDistributedTarget() {
    if(!CurrentTargets.Count){AttackedTargets.Clear();return;}
    for(int i=AttackedTargets.Count-1;i>=0;--i)if(CurrentTargets.FindItemIndex(AttackedTargets[i])<0)AttackedTargets.RemoveItem(i);
    AbstractClass* best=nullptr;int value=0;bool unseen=false;
    if(AttackedTargets.Count)for(int i=0;i<CurrentTargets.Count;++i)if(AttackedTargets.FindItemIndex(CurrentTargets[i])<0) {
        unseen=true;if(CurrentTargetThreatValues[i]>value){value=CurrentTargetThreatValues[i];best=CurrentTargets[i];}
    }
    if(!unseen) {
        AttackedTargets.Clear();
        for(int i=0;i<CurrentTargets.Count;++i)if(CurrentTargetThreatValues[i]>value){value=CurrentTargetThreatValues[i];best=CurrentTargets[i];}
    }
    SetTarget(best);
}
bool TechnoClass::TargetAndEstimateDamage(CoordStruct& origin,ThreatType threat) {
    unknown_4FC=Unsorted::CurrentFrame;
    TargetingTimer.Start(ScenarioClass::Instance->Random.RandomRanged(0,2)+(CurrentMission==Mission::Area_Guard
        ?RulesClass::Instance->GuardAreaTargetingDelay:RulesClass::Instance->NormalTargetingDelay));
    if(Target&&(ShouldLoseTargetNow&0xFF)) {
        const auto error=GetFireError(Target,SelectWeapon(Target),true);
        if(error==FireError::CANT){if(SpawnManager)SpawnManager->ResetTarget();else SetTarget(nullptr);}
        else if(error==FireError::ILLEGAL||error==FireError::RANGE)SetTarget(nullptr);
    }
    if(!Target) {
        auto* candidate=GreatestThreat(ThreatType(unsigned(threat)&3),&origin,false);
        if(GetTechnoType()->DistributedFire)SelectDistributedTarget();
        else {
            if(candidate)SetTarget(candidate);
            auto* weapon=GetWeapon(SelectWeapon(Target))->WeaponType;
            if(candidate&&(candidate->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None&&weapon&&!weapon->Projectile->Inaccurate)
                static_cast<TechnoClass*>(candidate)->EstimatedHealth-=EstimateDamage(static_cast<TechnoClass*>(candidate),weapon);
        }
    }
    return Target!=nullptr;
}
