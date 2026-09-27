// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Evaluate_Object/Evaluate_Cell/Threat_Value.
// YR 0x6F7CA0/0x6F8960/0x70CD10; EA Section 7: third_party/opents/LICENSE.md.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
#include "yrpp/TechnoClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <cmath>

namespace {
int distance(const CoordStruct& a,const CoordStruct& b,bool flat=false) {
    const double x=double(a.X)-b.X,y=double(a.Y)-b.Y,z=flat?0.0:double(a.Z)-b.Z;
    return rule_integer(std::sqrt(x*x+y*y+z*z));
}
bool campaign(){const auto& r=game::scenario_runtime();return r.session_mode(r.context)==int(GameMode::Campaign);}
HouseClass* transport_controller(const TechnoClass& actor) {
    auto* transport=actor.Transporter;
    return transport&&transport->GetTechnoType()->OpenTopped&&transport->MindControlledBy?transport->MindControlledBy->Owner:nullptr;
}
}
double TechnoClass::ThreatCoeffients(const ObjectClass* target,const CoordStruct* origin) const {
    if(!target->GetType())return 0.0;
    const auto* type=GetTechnoType();const auto& rules=*RulesClass::Instance;
    const bool smart=Owner->HasThreatNode;
    const double mine=smart?type->MyEffectivenessCoefficient:rules.MyEffectivenessCoefficientDefault;
    const double theirs=smart?type->TargetEffectivenessCoefficient:rules.TargetEffectivenessCoefficientDefault;
    const double special=smart?type->TargetSpecialThreatCoefficient:rules.TargetSpecialThreatCoefficientDefault;
    const double health=smart?type->TargetStrengthCoefficient:rules.TargetStrengthCoefficientDefault;
    const double range=smart?type->TargetDistanceCoefficient:rules.TargetDistanceCoefficientDefault;
    auto* weapon=GetWeapon(SelectWeapon(const_cast<ObjectClass*>(target)))->WeaponType;
    double value=0.0;
    if((target->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None) {
        const auto* enemy=static_cast<const TechnoClass*>(target);
        auto* enemyWeapon=enemy->GetWeapon(enemy->SelectWeapon(const_cast<TechnoClass*>(this)))->WeaponType;
        if(enemyWeapon&&enemyWeapon->Warhead)value=theirs*enemyWeapon->Warhead->Verses[int(type->Armor)]*(enemy->Target==this?-1.0:1.0);
        value+=special*enemy->GetTechnoType()->SpecialThreatValue;
        if(Owner->EnemyHouseIndex!=-1&&Owner->EnemyHouseIndex==enemy->Owner->ArrayIndex)value+=rules.EnemyHouseThreatBonus;
    }
    if(weapon&&weapon->Warhead)value+=mine*weapon->Warhead->Verses[int(target->GetType()->Armor)];
    value+=target->GetHealthPercentage()*health;
    const int reach=(weapon?weapon->Range:type->GuardRange)/256;
    // YR divides the actor-based distance, but not the explicit-origin one.
    const int separation=*origin==CoordStruct::Empty?distance(GetCoords(),target->GetCoords())/256:distance(*origin,target->GetCoords());
    return value+std::max(separation-reach,0)*range+100000.0;
}
double TechnoClass::ShouldSuppress(CellStruct* at) const {
    auto* slot=GetTurretWeapon();if(!slot||!slot->WeaponType||!slot->WeaponType->Supress)return 1.0;
    auto& map=MapClass::Instance;double factor=1.0;const auto bounds=map.MapRect;
    const auto check=[&](int x,int y) {
        if(x<bounds.X||x>=bounds.X+bounds.Width||y<bounds.Y||y>=bounds.Y+bounds.Height)return;
        auto* building=map.GetCellAt(CellStruct{short(x),short(y)})->GetBuilding();
        if(building&&Owner->IsAlliedWith(building))factor*=0.5;
    };
    for(int radius=1;radius<RulesClass::Instance->FireSupress/256;++radius) {
        for(int x=-radius;x<=radius;++x){check(at->X+x,at->Y-radius);check(at->X+x,at->Y+radius);}
        for(int y=1-radius;y<radius;++y){check(at->X-radius,at->Y+y);check(at->X+radius,at->Y+y);}
    }
    return factor;
}
bool TechnoClass::CanAutoTargetObject(ThreatType threat,int mask,int range,TechnoClass* target,int* value,DWORD zone,const CoordStruct* origin) const {
    const unsigned flags=unsigned(threat);const bool engineer=IsEngineer();
    const auto* type=GetTechnoType();const auto* enemyType=target->GetTechnoType();
    const int index=SelectWeapon(target);auto* weapon=GetWeapon(index)->WeaponType;
    if(!(flags&0x18200)&&GetFireErrorWithoutRange(target,index)==FireError::ILLEGAL)return false;
    if(target->EstimatedHealth<=0&&type->VHPScan==2)return false;
    if(weapon&&weapon->Warhead&&weapon->Warhead->Verses[int(enemyType->Armor)]<=0.02)return false;
    if(type->SpeedType==SpeedType::Float&&target->IsOnFloor()&&target->GetCell()->LandType!=LandType::Water)return false;
    if(target->InLimbo||!target->Health)return false;
    if(target->CloakState==CloakState::Cloaked&&!target->GetCell()->Sensors_InclHouse(Owner->ArrayIndex)&&Owner!=target->Owner)return false;
    if(!target->IsInPlayfield||target->CurrentMissionControl()->NoThreat||target->GetHeight()<-20)return false;
    if(int(zone)!=-1&&MapClass::Instance.GetMovementZoneType(target->GetMapCoords(),type->MovementZone,target->OnBridge)!=int(zone))return false;
    auto* controller=transport_controller(*this);
    const bool permanent=WhatAmI()==AbstractType::Infantry&&static_cast<const InfantryClass*>(this)->PermanentBerzerk;
    if(controller){if(controller->IsAlliedWith(target))return false;}
    else if(!permanent&&!type->AttackFriendlies&&!Berzerk&&Owner->IsAlliedWith(target)) {
        if((CombatDamage(-1)>=0&&!engineer)||target->GetHealthPercentage()==RulesClass::Instance->ConditionGreen)return false;
        if(WhatAmI()==AbstractType::Unit) {
            if(target->WhatAmI()==AbstractType::Aircraft){if(target->GetHeight()>0||target->GetCell()->GetBuilding())return false;}
            else if(!target->IsStrange())return false;
        }
    }
    if(target==this)return false;
    if(ScenarioClass::Instance->SpecialFlags.HarvesterImmune)
        for(auto* harvester:RulesClass::Instance->HarvesterUnit)if(static_cast<const void*>(harvester)==target->GetType())return false;
    const int separation=distance(GetCoords(),target->GetCoords(),target->IsInAir());
    if(range>0&&separation>range)return false;
    if(!range&&(IsArmed()?!IsCloseEnough(target,index):separation>type->GuardRange))return false;
    if(Owner->IsControlledByCurrentPlayer()&&!target->DiscoveredByCurrentPlayer&&!target->DiscoveredByComputer&&campaign()&&target->WhatAmI()!=AbstractType::Aircraft)return false;
    auto* building=target->WhatAmI()==AbstractType::Building?static_cast<BuildingClass*>(target):nullptr;
    if(building&&building->Type->InvisibleInGame)return false;
    if(!(unsigned(mask)&(1u<<(unsigned(target->WhatAmI())&31)))&&(!(mask&2)||!target->IsStrange()))return false;
    if(!enemyType->LegalTarget)return false;
    const auto neutralException=[&] {
        if(Owner->IsControlledByHuman()||!building||WhatAmI()!=AbstractType::Infantry)return false;
        auto* infantry=static_cast<const InfantryClass*>(this);
        if(infantry->Team&&infantry->Team->Type->IonImmune)return false;
        return !campaign()&&((building->Type->NeedsEngineer&&infantry->Type->Engineer&&target->Owner!=Owner)
            ||(infantry->Type->Occupier&&building->CanBeOccupiedBy(const_cast<InfantryClass*>(infantry))));
    };
    if(!campaign()&&target->Owner->Type->MultiplayPassive&&!neutralException())return false;
    if(enemyType->Insignificant&&!target->MindControlledBy&&!target->MindControlledByAUnit
        &&(!building||target->Owner->Type->MultiplayPassive)&&!neutralException())return false;
    if(enemyType->IsTrain&&WhatAmI()==AbstractType::Infantry&&static_cast<const InfantryClass*>(this)->Type->VehicleThief)return false;
    if(target->IsDisguisedAs(Owner)&&!type->DetectDisguise) {
        if(!target->DisguiseBlinkTimer.GetTimeLeft()||Owner->IsControlledByHuman())return false;
        if(ScenarioClass::Instance->Random.RandomRanged(0,99)>RulesClass::Instance->DisabledDisguiseDetectionPercent[int(Owner->AIDifficulty)])return false;
    }
    if(flags&0x100)return false;
    if((flags&0x200)&&(!building||!building->Type->Capturable))return false;
    const bool team=(AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None&&static_cast<const FootClass*>(this)->Team;
    auto* turret=target->GetTurretWeapon();
    if(!team&&Owner->IsControlledByHuman()&&!target->IsStrange()&&building&&(!turret||!turret->WeaponType||!target->GetThreatValue())&&!engineer)return false;
    if(engineer&&(!building||(Owner->IsAlliedWith(target->Owner)
        &&(target->GetHealthPercentage()>RulesClass::Instance->ConditionRed||!building->Type->GetActualCost(Owner)))))return false;
    if((flags&0x40)&&!enemyType->Storage)return false;
    if(GetCell()->ContainsBridge()&&target->GetCell()->ContainsBridge()&&OnBridge!=target->OnBridge)return false;
    *value=rule_integer(ThreatCoeffients(target,origin));
    if(type->VHPScan==1){if(target->EstimatedHealth<=0)*value/=2;else if(target->EstimatedHealth<=enemyType->Strength/2)*value*=2;}
    if(Owner->AllToHunt&&Owner->EnemyHouseIndex!=-1&&target->Owner!=HouseClass::Array[Owner->EnemyHouseIndex])*value=1;
    if((flags&0x800)&&building){if(building->Type->PowerBonus<=0)*value=0;else *value+=1000*building->Type->PowerBonus;}
    if(flags&0x8000){if(!building||building->Type->MaxNumberOccupants<=0)return false;*value+=1000*building->Type->MaxNumberOccupants;}
    if(flags&0x10000){if(!building||!building->Type->NeedsEngineer)return false;*value+=1000;}
    if((flags&0x1000)&&building&&building->Type->Factory==AbstractType::None)*value=0;
    if(flags&0x2000){if(building&&building->Type->CanBeOccupied){if(!building->GetOccupantCount())return false;}else if(!building||!turret||!turret->WeaponType)return false;}
    auto at=target->GetMapCoords();const double suppression=ShouldSuppress(&at);
    if(suppression!=1.0)*value=rule_integer(suppression);
    if(!*value)return false;*value=std::max(*value,1);return true;
}
bool TechnoClass::TryAutoTargetObject(ThreatType threat,int mask,CellStruct* at,int range,TechnoClass** output,int* value,int zone) {
    *output=nullptr;*value=0;auto& map=MapClass::Instance;auto* cell=map.GetCellAt(*at);
    if(zone!=-1&&map.GetMovementZoneType(*at,GetTechnoType()->MovementZone,true)!=zone)return false;
    auto* controller=transport_controller(*this);TechnoClass* candidate=nullptr;
    for(auto* object=cell->AltObject?cell->AltObject:cell->FirstObject;object;object=object->NextObject) {
        if(object==this)continue;
        if((object->AbstractFlags&::AbstractFlags::Techno)==::AbstractFlags::None){candidate=nullptr;continue;}
        candidate=static_cast<TechnoClass*>(object);
        if(CombatDamage(-1)<0){if(candidate->GetHealthPercentage()<RulesClass::Instance->ConditionGreen&&Owner->IsAlliedWith(candidate))break;continue;}
        bool infantryException=false;
        if(WhatAmI()==AbstractType::Infantry) {
            const auto* infantry=static_cast<const InfantryClass*>(this);
            infantryException=infantry->PermanentBerzerk || (infantry->Type->Engineer&&CurrentMission==Mission::Area_Guard
                &&candidate->GetHealthPercentage()<=RulesClass::Instance->ConditionRed&&candidate->WhatAmI()==AbstractType::Building
                &&candidate->GetTechnoType()->GetActualCost(candidate->Owner)>0);
        }
        if((Owner->IsAlliedWith(candidate)||controller)&&!GetTechnoType()->AttackFriendlies&&!Berzerk
            &&(!controller||!controller->IsAlliedWith(candidate))&&!infantryException)continue;
        break;
    }
    if(!candidate)return false;*output=candidate;
    const bool engineer=WhatAmI()==AbstractType::Infantry&&static_cast<InfantryClass*>(this)->Type->Engineer;
    if(!Owner->IsAlliedWith(candidate)||CombatDamage(-1)<0||(!Owner->IsControlledByHuman()&&engineer)||GetTechnoType()->AttackFriendlies||Berzerk||controller)
        return CanAutoTargetObject(threat,mask,range,candidate,value,DWORD(-1),&CoordStruct::Empty);
    return false;
}
int TechnoClass::GetWallThreat(const CellStruct* at) const {
    if(Owner->IsControlledByHuman())return 0;
    const DifficultyStruct* difficulty[]{&RulesClass::Instance->Easy,&RulesClass::Instance->Normal,&RulesClass::Instance->Difficult};
    if(!difficulty[int(Owner->AIDifficulty)]->DestroyWalls)return 0;
    auto* cell=MapClass::Instance.GetCellAt(*at);if(cell->OverlayTypeIndex==-1)return 0;
    if(!OverlayTypeClass::Array[cell->OverlayTypeIndex]->Wall)return 0;
    const CoordStruct where{at->X*256+128,at->Y*256+128,0};
    if(!IsCloseEnough3D(where,SelectWeapon(cell)))return 0;
    auto* slot=GetTurretWeapon();if(!slot||!slot->WeaponType||!slot->WeaponType->Warhead)return 0;
    auto* weapon=slot->WeaponType;
    if((weapon->Projectile&&!weapon->Projectile->AG)||!weapon->Warhead->Wall)return 0;
    if(cell->WallOwnerIndex==-1||Owner->IsAlliedWith(HouseClass::Array[cell->WallOwnerIndex]))return 0;
    return GetWeaponRange(0)-distance(GetCoords(),where);
}
