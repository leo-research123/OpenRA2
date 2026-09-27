// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Can_Fire, calibrated to YR 0x6FC0B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"
#include <cstdlib>

bool TechnoClass::IsUnderBridge() const {
    // 0x703B10 tests the centre and four edge extensions in this order.
    const auto at=GetMapCoords();
    const auto* centre=MapClass::Instance.TryGetCellAt(at);
    if(!centre || OnBridge)return false;
    if(centre->ContainsBridge())return true;
    for(const int direction:{4,6,2,0}) {
        const auto offset=Unsorted::AdjacentCell[direction];
        const CellStruct neighbor{short(at.X+offset.X),short(at.Y+offset.Y)};
        const auto* cell=MapClass::Instance.TryGetCellAt(neighbor);
        const bool northSouth=cell && (static_cast<unsigned>(cell->Flags)&0x800u);
        if(cell && cell->ContainsBridge() && northSouth==(direction==4 || direction==0))return true;
    }
    return false;
}

int TechnoClass::EstimateDamage(TechnoClass* target,WeaponTypeClass* weapon) const {
    if(!target)return 0;
    int damage=weapon->Damage;
    if(weapon->IsSonic || weapon->UseFireParticles)return 0;
    if(damage<=0)return damage;
    damage=rule_integer(Owner->FirepowerMultiplier*FirepowerMultiplier*damage);
    const auto* type=GetTechnoType();
    if((Veterancy.IsVeteran() && type->VeteranAbilities.FIREPOWER)
        || (Veterancy.IsElite() && (type->VeteranAbilities.FIREPOWER || type->EliteAbilities.FIREPOWER)))
        damage=rule_integer(double(damage)*RulesClass::Instance->VeteranCombat);
    // Original asks the victim house about the SHOOTER type and divides by
    // the SHOOTER's object armor multiplier. Preserve the original behavior.
    damage=rule_integer(double(damage)/(target->Owner->GetArmorMultiplier(GetTechnoType())*ArmorMultiplier));
    const auto* targetType=target->GetTechnoType();
    if((target->Veterancy.IsVeteran() && targetType->VeteranAbilities.STRONGER)
        || (target->Veterancy.IsElite() && (targetType->VeteranAbilities.STRONGER || targetType->EliteAbilities.STRONGER)))
        damage=rule_integer(double(damage)/RulesClass::Instance->VeteranArmor);
    return MapClass::GetTotalDamage(std::max(damage,1),weapon->Warhead,target->GetType()->Armor,0);
}

// OpenTS Rearm_Delay; YR 0x6FCFA0. Burst delays belong to UnitType in
// this binary, and veteran ROF multiplies the already-truncated house delay.
int TechnoClass::GetROF(int index) const {
    if(WhatAmI()==AbstractType::Building && Ammo>1)return 1;
    auto* weapon=GetWeapon(index)->WeaponType;
    if(!weapon)return 1;
    if(weapon->IsSonic || (weapon->UseSparkParticles && SparkParticleSystem)
        || (weapon->UseFireParticles && FireParticleSystem) || (weapon->IsRailgun && RailgunParticleSystem))return weapon->ROF;
    auto& random=ScenarioClass::Instance->Random;
    if(CurrentBurstIndex<weapon->Burst) {
        if(CurrentBurstIndex>0 && CurrentBurstIndex<=4 && WhatAmI()==AbstractType::Unit) {
            auto* unit=static_cast<const UnitClass*>(this)->Type;
            const int delays[]{unit->BurstDelay0,unit->BurstDelay1,unit->BurstDelay2,unit->BurstDelay3};
            if(delays[CurrentBurstIndex-1]!=-1)return delays[CurrentBurstIndex-1];
        }
        return random.RandomRanged(3,5);
    }
    const int jitter=random.RandomRanged(0,2);
    int delay=rule_integer(double(weapon->ROF)*Owner->ROFMultiplier+double(jitter));
    auto* type=GetTechnoType();
    if((Veterancy.IsVeteran() && type->VeteranAbilities.ROF)
        || (Veterancy.IsElite() && (type->VeteranAbilities.ROF || type->EliteAbilities.ROF)))
        delay=rule_integer(double(delay)*RulesClass::Instance->VeteranROF);
    if(CanOccupyFire()) {
        if(GetOccupantCount()>0)delay/=GetOccupantCount();
        if(RulesClass::Instance->OccupyROFMultiplier>0.0f)delay=rule_integer(double(delay)/RulesClass::Instance->OccupyROFMultiplier);
    }
    if(BunkerLinkedItem && WhatAmI()!=AbstractType::Building && RulesClass::Instance->BunkerROFMultiplier!=0.0f)
        delay=rule_integer(double(delay)/RulesClass::Instance->BunkerROFMultiplier);
    return delay;
}

FireError TechnoClass::GetFireError(AbstractClass* target,int index,bool checkRange) const {
    if(!target || SlaveOwner)return FireError::ILLEGAL;
    if(IsWarpingIn() || target==LocomotorTarget)return FireError::REARM;
    if(IsBeingWarpedOut() || Deactivated || IsSinking || target==DrainTarget || target==Transporter)return FireError::ILLEGAL;
    if(IsWarpingSomethingOut() && TemporalImUsing && TemporalImUsing->Target==target)return FireError::REARM;
    const auto isFoot=[](const AbstractClass* object){return (object->AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None;};
    auto* victim=(target->AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None?static_cast<TechnoClass*>(target):nullptr;
    const auto* targetCell=MapClass::Instance.GetCellAt(target->GetCoords());
    auto* type=GetTechnoType();
    if(isFoot(this) && static_cast<const FootClass*>(this)->IsAttackedByLocomotor)return FireError::ILLEGAL;
    if(victim) {
        auto* victimType=victim->GetTechnoType();
        if(victim->InLimbo || (Berzerk && victimType->BerserkFriendly) || (type->Natural && victimType->Unnatural)
            || victim->IsImmobilized || (!Owner->IsHumanPlayer && victim->IsIronCurtained()))return FireError::ILLEGAL;
        if(victim->VisualCharacter(true,Owner)==VisualType::Hidden && !targetCell->Sensors_InclHouse(Owner->ArrayIndex)
            && (CombatDamage(-1)>0 || !victim->Owner->IsAlliedWith(Owner)))return FireError::CANT;
        if(victim->IsTether && victimType->BalloonHover && !victim->IsInAir())return FireError::ILLEGAL;
    }
    if(IsFallingDown)return FireError::ILLEGAL;
    if(IsUnderEMP()) {
        if(WhatAmI()!=AbstractType::Unit)return FireError::CANT;
        auto* unitType=static_cast<const UnitClass*>(this)->Type;
        if(!unitType->LargeVisceroid && !unitType->SmallVisceroid)return FireError::CANT;
    }
    auto* weapon=GetWeapon(index)->WeaponType;
    if(!weapon || (weapon->IonSensitive && Owner->IonSensitivesShouldBeOffline()))return FireError::CANT;
    if(weapon->DrainWeapon && victim && (victim->DrainingMe || !victim->GetTechnoType()->Drainable))return FireError::ILLEGAL;
    if(victim && victim->BunkerLinkedItem && victim->WhatAmI()==AbstractType::Unit && weapon->Range<384)return FireError::ILLEGAL;
    if(WhatAmI()==AbstractType::Unit && static_cast<const UnitClass*>(this)->IsDeployingOrUndeploying())return FireError::ILLEGAL;
    auto* warhead=weapon->Warhead;
    if(warhead && warhead->Psychedelic && victim && (victim->GetTechnoType()->ImmuneToPsionics || victim->BunkerLinkedItem))return FireError::ILLEGAL;
    if(warhead && warhead->IsLocomotor) {
        if(target->WhatAmI()==AbstractType::Unit && static_cast<UnitClass*>(target)->IsDeployingOrUndeploying())return FireError::ILLEGAL;
        if(isFoot(target) && static_cast<FootClass*>(target)->GetTechnoType()->JumpJet) {
            const auto& locomotor=static_cast<FootClass*>(target)->Locomotor;
            if(!locomotor)std::abort();
            if(locomotor->Is_Moving_Now())return FireError::ILLEGAL;
        }
        if(target->WhatAmI()==AbstractType::Unit) {
            auto* unit=static_cast<UnitClass*>(target);
            if(unit->Type->IsSimpleDeployer && unit->CurrentMission==Mission::Unload)return FireError::ILLEGAL;
        }
        if(victim && victim->GetTechnoType()->Organic)return FireError::ILLEGAL;
    }
    if(InOpenToppedTransport && (!weapon->FireInTransport || (Transporter && (Transporter->IsBeingWarpedOut() || Transporter->Transporter))))return FireError::ILLEGAL;
    if(victim && warhead && victim->IsBeingWarpedOut() && !warhead->Temporal)return FireError::ILLEGAL;
    if(weapon->Spawner) {
        if(IsUnderBridge() || IsParalyzed())return FireError::CANT;
        if(!SpawnManager->CountAliveSpawns())return FireError::REARM;
    }
    if(warhead->Temporal && victim && victim->WhatAmI()==AbstractType::Aircraft && victim->GetTechnoType()->Spawned)return FireError::ILLEGAL;
    const auto effectActive=[&](const WeaponTypeClass* w){return w && ((w->UseFireParticles && FireParticleSystem)
        || (w->IsRailgun && RailgunParticleSystem) || (w->UseSparkParticles && SparkParticleSystem) || (w->IsSonic && Wave));};
    if(effectActive(GetWeapon(index==0?1:0)->WeaponType))return FireError::CANT;
    if(target->IsInAir() && !weapon->Projectile->AA)return target==LocomotorTarget?FireError::REARM:FireError::ILLEGAL;
    if(isFoot(target) && static_cast<FootClass*>(target)->InWhichLayer()!=Layer::Ground && !weapon->Projectile->AA)return FireError::ILLEGAL;
    bool testLand=false;
    if(victim) {
        auto* cell=victim->GetCell();
        bool water=(cell->LandType==LandType::Water || cell->LandType==LandType::Beach) && !victim->IsInAir();
        if(victim->OnBridge)water=false;
        else if(water && SelectNavalTargeting(target)==-1)return FireError::ILLEGAL;
        testLand=target->IsOnFloor() && !water;
    } else {
        if(!target->IsInAir() && !weapon->Projectile->AG)return FireError::ILLEGAL;
        testLand=target->WhatAmI()==AbstractType::Cell && targetCell->LandType!=LandType::Water && targetCell->LandType!=LandType::Beach;
    }
    if(testLand && static_cast<int>(type->LandTargeting)==1)return FireError::ILLEGAL;
    if(weapon->IsMagBeam && LocomotorTarget && target->WhatAmI()!=AbstractType::Building
        && (LocomotorTarget==target || Wave))return FireError::REARM;
    bool useTimer=true;
    if(index==0 && WhatAmI()==AbstractType::Unit) {
        auto* unit=static_cast<const UnitClass*>(this);
        const int burst=CurrentBurstIndex%weapon->Burst;
        const int sync=burst==0?unit->Type->FiringSyncFrame0:unit->Type->FiringSyncFrame1;
        if(burst<2 && sync!=-1 && unit->CurrentFiringFrame!=-1) {
            useTimer=false;
            if(unit->CurrentFiringFrame!=sync)return FireError::REARM;
        }
    }
    if(useTimer && RearmTimer.GetTimeLeft()!=0)return FireError::REARM;
    if(effectActive(weapon))return FireError::REARM;
    if(!Ammo)return FireError::AMMO;
    if(weapon->DecloakToFire && CloakState!=::CloakState::Uncloaked
        && (WhatAmI()!=AbstractType::Aircraft || CloakState==::CloakState::Cloaked))return FireError::CLOAKED;
    if(type->HunterSeeker)return FireError::RANGE;
    if(warhead->Parasite && isFoot(this)
        && !static_cast<const FootClass*>(this)->ParasiteImUsing->CanInfect(isFoot(target)?static_cast<FootClass*>(target):nullptr))return FireError::ILLEGAL;
    if(warhead->Parasite && isFoot(target) && Unsorted::CurrentFrame<static_cast<FootClass*>(target)->LastBeParasitedStartFrame)return FireError::ILLEGAL;
    if(victim) {
        if(warhead->Parasite && victim->IsIronCurtained())return FireError::ILLEGAL;
        if(warhead->MindControl && !CaptureManager->CanCapture(victim))return FireError::ILLEGAL;
        if(warhead->Verses[static_cast<int>(victim->GetTechnoType()->Armor)]==0.0)return FireError::ILLEGAL;
        if((warhead->BombDisarm && !victim->AttachedBomb) || (warhead->IvanBomb && victim->AttachedBomb) || victim->IsSinking)return FireError::ILLEGAL;
        if(OnBridge!=victim->OnBridge) {
            if(GetCell() && GetCell()->ContainsBridgeEx() && !IsInAir() && victim->GetCell() && victim->GetCell()->ContainsBridgeEx())return FireError::ILLEGAL;
            if(warhead->Parasite && std::abs(Location.Z-victim->Location.Z)>2*Unsorted::LevelHeight)return FireError::ILLEGAL;
        }
    }
    if(type->Organic && IsParalyzed())return FireError::ILLEGAL;
    return !checkRange || IsCloseEnough(target,index)?FireError::OK:FireError::RANGE;
}
