// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Assign_Target, YR 0x006FCDB0 / 0x0070E140.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include <cmath>
#include <climits>
#include <bit>

bool TechnoClass::IsCloseEnoughToAttackCoords(const CoordStruct& coords) const {
    auto* target=MapClass::Instance.GetCellAt(coords);
    return IsCloseEnough(target,SelectWeapon(target));
}
bool TechnoClass::InAuxiliarySearchRange(AbstractClass* target) const {
    const int index=GetSecondaryWeaponIndex();
    auto* weapon=GetWeapon(index);
    return weapon && weapon->WeaponType && weapon->WeaponType->NeverUse
        ?IsCloseEnough(target,index):IsCloseEnoughToAttack(target);
}

// OpenTS What_Weapon_Should_I_Use provides the original-class entry; YR's
// complete decision tree differs substantially (0x6F3330 / 0x6F3820).
int TechnoClass::SelectWeapon(AbstractClass* target) const {
    if(GetTechnoType()->HasMultipleTurrets() && !GetTechnoType()->IsGattling)
        return CurrentWeaponNumber!=-1?CurrentWeaponNumber:0;
    if(CanOccupyFire())return 0;
    auto* secondary=GetWeapon(1)->WeaponType;
    if(!secondary)return 0;
    auto* primary=GetWeapon(0)->WeaponType;
    if(!primary || secondary->NeverUse || !target)return 0;
    if(InOpenToppedTransport && GetTechnoType()->OpenTransportWeapon!=-1)return GetTechnoType()->OpenTransportWeapon;
    auto* techno=(target->AbstractFlags & ::AbstractFlags::Techno)!=::AbstractFlags::None?static_cast<TechnoClass*>(target):nullptr;
    if(GetTechnoType()->IsGattling)
        return 2*CurrentGattlingStage+(secondary->Projectile->AA && techno && techno->IsInAir()?1:0);
    if(secondary->Warhead->Airstrike) {
        if(target->WhatAmI()==AbstractType::Building && static_cast<BuildingClass*>(target)->Type->CanC4) {
            if(static_cast<TechnoClass*>(target)->GetTechnoType()->ResourceDestination)
                return !static_cast<TechnoClass*>(target)->GetTechnoType()->ResourceGatherer;
            return 1;
        }
        return 0;
    }
    if(primary->Warhead->IsLocomotor && techno && techno->WhatAmI()==AbstractType::Building)return 1;
    if(secondary->DrainWeapon && techno && techno->GetTechnoType()->Drainable && !Deactivated && !Owner->IsAlliedWith(techno))return 1;
    if(secondary->AreaFire && GetCurrentMission()==Mission::Unload)return 1;
    if(WhatAmI()==AbstractType::Building && static_cast<const BuildingClass*>(this)->IsOverpowered)return 1;
    if(target->WhatAmI()==AbstractType::Building && Owner->IsAlliedWith(target)
        && secondary->Warhead->ElectricAssault && static_cast<BuildingClass*>(target)->Type->Overpowerable)return 1;
    if(WhatAmI()==AbstractType::Aircraft && static_cast<const AircraftClass*>(this)->IsKamikaze)return 1;
    if(target->WhatAmI()==AbstractType::Cell) {
        auto* cell=static_cast<CellClass*>(target);
        if(((cell->LandType!=LandType::Water && cell->IsOnFloor())
            || ((static_cast<unsigned>(cell->Flags)&0x100u) && GetTechnoType()->Naval))
            && !cell->IsInAir() && GetTechnoType()->LandTargeting==LandTargetingType::Land_Secondary)return 1;
    }
    if(techno && secondary->Warhead->Verses[static_cast<int>(techno->GetTechnoType()->Armor)]!=0.0) {
        if(primary->Warhead->Verses[static_cast<int>(techno->GetTechnoType()->Armor)]==0.0)return 1;
        bool water=techno->GetCell()->LandType==LandType::Water || techno->GetCell()->LandType==LandType::Beach;
        if(techno->IsInAir())water=false;
        if(!techno->OnBridge && water){const int choice=SelectNavalTargeting(target);return choice!=-1?choice:0;}
        if(!techno->IsInAir() && GetTechnoType()->LandTargeting==LandTargetingType::Land_Secondary)return 1;
        if(secondary->Projectile->AA && techno->IsInAir())return 1;
    }
    return 0;
}
int TechnoClass::SelectNavalTargeting(AbstractClass* target) const {
    if(!target || (target->AbstractFlags & ::AbstractFlags::Techno)==::AbstractFlags::None)return -1;
    auto* techno=static_cast<TechnoClass*>(target);
    const bool underwater=techno->GetTechnoType()->Underwater;
    const bool organic=techno->GetTechnoType()->Organic;
    const bool hovering=techno->GetTechnoType()->SpeedType==SpeedType::Hover;
    const bool unnatural=techno->GetTechnoType()->Unnatural;
    switch(GetTechnoType()->NavalTargeting) {
        case NavalTargetingType::Underwater_Never:return underwater && techno->CloakState!=::CloakState::Uncloaked?-1:0;
        case NavalTargetingType::Underwater_Secondary:return underwater?1:0;
        case NavalTargetingType::Underwater_Only:return underwater?0:-1;
        case NavalTargetingType::Organic_Secondary:return organic || unnatural?1:0;
        case NavalTargetingType::SEAL_Special:return hovering || organic?0:1;
        case NavalTargetingType::Naval_None:return -1;
        default:return 0;
    }
}

bool TechnoClass::IsCloseEnough3D(const CoordStruct& coords,int weapon) const {
    return IsCloseEnough(MapClass::Instance.GetCellAt(coords),weapon);
}
// YR 0x710550: walk the original passenger chain, including limbo occupants.
void TechnoClass::SetTargetForPassengers(AbstractClass* target) {
    for(auto* passenger=Passengers.FirstPassenger;passenger;) {
        passenger->SetTarget(target);
        auto* next=passenger->NextObject;
        passenger=next&&(next->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None
            ?static_cast<FootClass*>(next):nullptr;
    }
}
int TechnoClass::GetWeaponRange(int index) const {
    auto* weapon=GetWeapon(index)->WeaponType;
    if(!weapon)return 0;
    const int range=weapon->Range;
    if(!GetTechnoType()->OpenTopped)return range;
    const auto passenger_range=[&]{
        int result=INT_MAX;
        for(auto* passenger=Passengers.FirstPassenger;passenger;) {
            if(auto* held=passenger->GetTurretWeapon()->WeaponType;held && held->Range<result)result=held->Range;
            auto* next=passenger->NextObject;
            passenger=next && (next->AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(next):nullptr;
        }
        return result;
    };
    return range<passenger_range()?range:passenger_range();
}

void TechnoClass::ShortenTargetingDelay() {
    if(TargetingTimer.GetTimeLeft()>10 && !Unsorted::ScenarioInit)
        TargetingTimer.Start(ScenarioClass::Instance->Random.RandomRanged(4,8));
}

WeaponStruct* TechnoClass::GetWeapon(int index) const {
    auto* type = GetTechnoType();
    if (!type || index < 0 || index >= TechnoTypeClass::MaxWeapons) return nullptr;
    if (Veterancy.IsElite() && type->EliteWeapon[index].WeaponType) return &type->EliteWeapon[index];
    return &type->Weapon[index];
}

WeaponStruct* TechnoClass::GetTurretWeapon() const {
    auto* type = GetTechnoType();
    return GetWeapon(type->HasMultipleTurrets() ? CurrentWeaponNumber : 0);
}

int TechnoClass::CombatDamage(int index) const {
    const auto damage = [](const WeaponStruct* slot) {
        if (!slot || !slot->WeaponType) return 0;
        const auto* weapon = slot->WeaponType;
        return std::bit_cast<int>(static_cast<unsigned>(weapon->Damage) + static_cast<unsigned>(weapon->AmbientDamage));
    };
    if (index != -1) return damage(GetWeapon(index));
    auto* type = GetTechnoType();
    if (type->HasMultipleTurrets() && !type->IsGattling) return damage(GetWeapon(CurrentWeaponNumber));
    unsigned total = 0;
    int count = 0;
    for (int i = 0; i < 2; ++i) {
        auto* weapon = GetWeapon(i);
        if (weapon && weapon->WeaponType) { total += static_cast<unsigned>(damage(weapon)); ++count; }
    }
    return count ? std::bit_cast<int>(total) / count : 0;
}
double TechnoClass::GetStoragePercentage() const {
    auto* type = GetTechnoType();
    if (!type || !type->Storage) return 0.0;
    // Storage::GetTotalAmount truncates after EACH addition (0x006C9650).
    int amount = 0;
    for (float value : {Tiberium.Tiberium1, Tiberium.Tiberium2, Tiberium.Tiberium3, Tiberium.Tiberium4}) {
        const double sum = double(amount) + value;
        // FISTP returns integer-indefinite for nonrepresentable input.
        amount = std::isfinite(sum) && sum >= -2147483648.0 && sum < 2147483648.0
            ? static_cast<int>(sum) : INT_MIN;
    }
    return amount / double(type->Storage);
}
void TechnoClass::SetTarget(AbstractClass* requested) {
    // Only the low byte is cleared by the original, not the whole DWORD.
    ShouldLoseTargetNow &= 0xFFFFFF00u;
    if (requested == Target) return;
    if (Airstrike && Airstrike->Owner == this) Airstrike->ClearTarget();
    AbstractClass* target = requested;
    auto* type = GetTechnoType();
    if (WhatAmI() == AbstractType::Aircraft && requested && type && type->Spawned && !type->MissileSpawn &&
        Ammo && CurrentMission == Mission::Attack && MissionStatus > 4 && MissionStatus < 10) {
        Ammo = 0;
        target = nullptr;
    }
    if (target && (target->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None) {
        auto* techno = static_cast<TechnoClass*>(target);
        if (techno->BunkerLinkedItem && techno->WhatAmI() != AbstractType::Building) {
            if (TemporalImUsing) target = techno->BunkerLinkedItem;
            auto* weapon = GetWeapon(0);
            if (weapon && weapon->WeaponType && weapon->WeaponType->Warhead && weapon->WeaponType->Warhead->IsLocomotor)
                target = techno->BunkerLinkedItem;
            if (Airstrike && Airstrike->Owner == this) target = techno->BunkerLinkedItem;
        }
    }
    if (target == this) target = MapClass::Instance.GetCellAt(Location);
    else if (target && (target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None) {
        auto* object = static_cast<ObjectClass*>(target);
        if (!object->IsAlive || !object->Health) target = nullptr;
        else if ((object->AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None) {
            auto* foot = static_cast<FootClass*>(object);
            if (foot->IsSinking) target = nullptr;
            if (foot->WhatAmI() == AbstractType::Infantry && static_cast<InfantryClass*>(foot)->IsPlayingDeathSequence()) target = nullptr;
        }
    }
    Target = target;
    if (SpawnManager && !target) SpawnManager->SetTarget(nullptr);
    if (!Target) CurrentBurstIndex = 0;
    if (FireParticleSystem && !IsCloseEnoughToAttack(target)) {
        FireParticleSystem->UnInit();
        FireParticleSystem = nullptr;
    }
}
