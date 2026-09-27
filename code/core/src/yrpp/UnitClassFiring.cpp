// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Firing_AI; YR 0x736DF0 supplies the branches.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/WeaponTypeClass.h"
#include <bit>
#include <cstdlib>

// OpenTS UnitClass::Turret_Facing; YR 0x746E30 tests Type->Turret (0xCA1).
// All vehicle FLH, shot direction and directional muzzle animation queries
// must follow the same facing as the turret drawn by Unit.DrawAsVXL.
DirStruct* UnitClass::TurretFacing(DirStruct* output) const {
    *output=Type->Turret?SecondaryFacing.Current():PrimaryFacing.Current();
    return output;
}

// YR Unit.Fire 0x741444..0x741478: firing temporarily exposes the appearance,
// but deliberately retains the disguise used by retaliation and acquisition.
// Other vehicle firing effects remain in the existing Techno.Fire path.
BulletClass* UnitClass::Fire(AbstractClass* target,int index) {
    auto* weapon=GetWeapon(index)->WeaponType;
    auto* bullet=TechnoClass::Fire(target,index);
    if(bullet && weapon && Type->DisguiseWhenStill) {
        DisguiseBlinkTimer.Start(weapon->DisguiseFakeBlinkTime);
        Mark(MarkType::Change);
    }
    return bullet;
}

// OpenTS Can_Fire, calibrated against the complete YR 0x740FD0 branch order.
FireError UnitClass::GetFireError(AbstractClass* target,int index,bool checkRange) const {
    if(DeathFrameCounter>=0)return FireError::CANT;
    if(SpawnManager&&SpawnManager->CountLaunchingSpawns())return FireError::BUSY;
    if(LocomotorTarget)return FireError::ILLEGAL;
    const auto result=TechnoClass::GetFireError(target,index,checkRange);
    if((result==FireError::OK||result==FireError::FACING)&&Type->DeployToFire&&CanDeployNow()&&!GetCell()->CanBuildHere())return FireError::MUST_DEPLOY;
    if(result!=FireError::OK)return result;
    if(IsTether&&GetNthLink()->WhatAmI()==AbstractType::Building)return FireError::CANT;
    const auto* weapon=GetWeapon(index)->WeaponType;
    if(CombatDamage(-1)<0){
        auto* object=target&&(target->AbstractFlags&::AbstractFlags::Object)!=::AbstractFlags::None?static_cast<ObjectClass*>(target):nullptr;
        if(!object||!object->IsStrange()||object->GetHealthPercentage()>=RulesClass::Instance->ConditionGreen)return FireError::ILLEGAL;
    }
    if(!Type->MobileFire&&Destination)return FireError::MOVING;
    if(!Locomotor)std::abort(); // Original COM dependency, never a guessed state.
    if(!weapon->FireWhileMoving&&(Type->JumpJet?Locomotor->Is_Moving_Now():bool(Destination)))return FireError::MOVING;
    if((weapon->UseSparkParticles||weapon->UseFireParticles||TemporalImUsing)&&Destination)return FireError::MOVING;
    if(!IsFiring&&unknown_bool_6AF&&!weapon->Projectile->ROT)return FireError::ROTATING;
    if(!weapon->OmniFire&&!Type->LargeVisceroid&&!Type->SmallVisceroid){
        const auto facing=Type->Voxel?SecondaryFacing.Current():PrimaryFacing.Current();
        DirStruct direction;GetDirectionTo(&direction,target);
        const auto delta=std::bit_cast<short>(static_cast<unsigned short>(facing.Raw-direction.Raw));
        if(std::abs(int(delta))>(weapon->Projectile->ROT?0x1000:0x800))return FireError::FACING;
    }
    return Locomotor->Can_Fire();
}

void UnitClass::UpdateFiring(){
    const auto advance=[&]{TurretAnimFrame=std::bit_cast<int>(unsigned(TurretAnimFrame)+1u);};
    if(!Target||!GetWeapon(0)->WeaponType){
        if(Type->IsGattling){GattlingRateDown(1);if(CurrentGattlingStage>0)advance();}
        return;
    }
    const int weapon=SelectWeapon(Target);const auto error=GetFireError(Target,weapon,true);
    if((error==FireError::OK||error==FireError::FACING)&&CanDeployNow()){QueueMission(Mission::Unload,false);return;}
    switch(error){
    case FireError::OK:
        if(!Type->unknown_E10)IsFiring=false;
        if(Type->LargeVisceroid||Type->SmallVisceroid){
            constexpr int frames[]={100,105,110,115,120,125,90,95};DirStruct direction;GetDirectionTo(&direction,Target);
            Animation.Value=frames[direction.GetValue<3>()];Animation.Rate=5;Animation.Timer.Start(5);
        }
        Fire(Target,weapon);break;
    case FireError::FACING:{
        DirStruct direction;GetDirectionTo(&direction,Target);
        // 0x736F7E / 0x736F88: +0xE11 is OpenTS IsLockTurret (legacy
        // HasTurret name), +0xCA1 is Turret. A voxel body need not have a turret.
        if(!Type->HasTurret&&Type->Turret)SecondaryFacing.SetDesired(direction);
        else if(!Destination&&!Locomotor->Is_Moving()){PrimaryFacing.SetDesired(direction);SecondaryFacing.SetDesired(PrimaryFacing.Desired());}
        break;
    }
    case FireError::ILLEGAL:
        if(CombatDamage(weapon)<0){
            auto* object=Target&&(Target->AbstractFlags&::AbstractFlags::Object)!=::AbstractFlags::None?static_cast<ObjectClass*>(Target):nullptr;
            if(!object||object->WhatAmI()!=AbstractType::Unit||object->GetHealthPercentage()>=RulesClass::Instance->ConditionGreen)SetTarget(nullptr);
        }
        break;
    case FireError::CANT:if(SpawnManager)SpawnManager->ResetTarget();break;
    case FireError::RANGE:case FireError::MUST_DEPLOY:IsFiring=false;break;
    case FireError::CLOAKED:IsFiring=false;if(IsCloseEnough(Target,weapon))Uncloak(false);break;
    default:break;
    }
    if(Type->IsGattling){
        if(error==FireError::OK||error==FireError::REARM||error==FireError::FACING||error==FireError::ROTATING)GattlingRateUp(1);
        else GattlingRateDown(1);
        if(CurrentGattlingStage>0)advance();
    }else if(error==FireError::OK||error==FireError::REARM)advance();
}
