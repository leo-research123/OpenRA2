// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Firing_AI; YR 0x736DF0 supplies the branches.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "com_error.hpp"
#include <bit>
#include <cstdlib>

// OpenTS UnitClass::Turret_Facing; YR 0x746E30 tests Type->Turret (0xCA1).
// All vehicle FLH, shot direction and directional muzzle animation queries
// must follow the same facing as the turret drawn by Unit.DrawAsVXL.
DirStruct* UnitClass::TurretFacing(DirStruct* output) const {
    *output = Type->Turret ? SecondaryFacing.Current() : PrimaryFacing.Current();
    return output;
}

// YR Unit.Fire 0x741340: admission and vehicle-specific post-fire state.
BulletClass* UnitClass::Fire(AbstractClass* target, int index) {
    auto* weapon = GetWeapon(index)->WeaponType;
    if (!weapon) return nullptr;
    bool synchronized = false;
    if (index == 0) {
        const int burst = CurrentBurstIndex % weapon->Burst;
        const int frames[] = {Type->FiringSyncFrame0, Type->FiringSyncFrame1};
        synchronized = CurrentFiringFrame != -1 && burst < 2 && frames[burst] != -1;
    }
    if (GetFireError(target, index, true) != FireError::OK) return nullptr;
    auto* bullet = TechnoClass::Fire(target, index);
    if (bullet) {
        if (Type->Ammo > 0 && Ammo < Type->Ammo && !Type->ManualReload) StartReloading();
        if (!synchronized && Type->FiringFrames > 0) CurrentFiringFrame = 2 * Type->FiringFrames - 1;
        if (bullet->Type->Scalable) bullet->InitScalable();
        if (Type->DisguiseWhenStill) {
            // Retain the disguise used by retaliation and acquisition.
            DisguiseBlinkTimer.Start(weapon->DisguiseFakeBlinkTime);
        }
    }
    return bullet;
}

// OpenTS Can_Fire, calibrated against the complete YR 0x740FD0 branch order.
FireError UnitClass::GetFireError(AbstractClass* target, int index, bool checkRange) const {
    if (DeathFrameCounter >= 0) return FireError::CANT;
    if (SpawnManager && SpawnManager->CountLaunchingSpawns()) return FireError::BUSY;
    if (DirectRockerLinkedUnit) return FireError::ILLEGAL;
    const auto result = TechnoClass::GetFireError(target, index, checkRange);
    if (result == FireError::OK || result == FireError::FACING) {
        // 0x741078..0x7410A6 queries the cell before deployment admission.
        const bool buildable = MapClass::Instance.GetCellAt(Location)->CanBuildHere();
        if (Type->DeployToFire && CanDeployNow() && !buildable) return FireError::MUST_DEPLOY;
    }
    if (result != FireError::OK) return result;
    if (IsTether && GetNthLink()->WhatAmI() == AbstractType::Building) return FireError::CANT;
    const auto* weapon = GetWeapon(index)->WeaponType;
    if (CombatDamage(-1) < 0) {
        auto* object = target && (target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None
                           ? static_cast<ObjectClass*>(target)
                           : nullptr;
        if (!object || !object->IsStrange() || object->GetHealthPercentage() >= RulesClass::Instance->ConditionGreen)
            return FireError::ILLEGAL;
    }
    if (!Type->MobileFire && Destination) return FireError::MOVING;
    if (!weapon->FireWhileMoving) {
        if (Type->BalloonHover) {
            // Original 0x7411A3 checks the COM pointer only on this branch.
            // Preserve E_POINTER; the resource/host boundary catches it.
            if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
            if (Locomotor->Is_Moving_Now()) return FireError::MOVING;
        } else if (Destination)
            return FireError::MOVING;
    }
    if ((weapon->UseSparkParticles || weapon->UseFireParticles || TemporalImUsing) && Destination)
        return FireError::MOVING;
    if (!IsFiring && unknown_bool_6AF && !weapon->Projectile->ROT) return FireError::ROTATING;
    if (!weapon->OmniFire && !Type->LargeVisceroid && !Type->SmallVisceroid) {
        // YR 0x741288 checks Turret (+0xCA1), not the model's Voxel flag.
        const auto facing = Type->Turret ? SecondaryFacing.Current() : PrimaryFacing.Current();
        DirStruct direction;
        GetDirectionTo(&direction, target);
        const auto delta = std::bit_cast<short>(static_cast<unsigned short>(facing.Raw - direction.Raw));
        if (std::abs(int(delta)) > (weapon->Projectile->ROT ? 0x1000 : 0x800)) return FireError::FACING;
    }
    // 0x741300: facing and rotation rejection precede this COM dependency.
    if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
    return Locomotor->Can_Fire();
}

void UnitClass::UpdateFiring() {
    const auto advance = [&] { TurretAnimFrame = std::bit_cast<int>(unsigned(TurretAnimFrame) + 1u); };
    if (!Target || !GetWeapon(0)->WeaponType) {
        if (Type->IsGattling) {
            GattlingRateDown(1);
            if (GattlingValue > 0) advance();
        }
        return;
    }
    const int weapon = SelectWeapon(Target);
    const auto error = GetFireError(Target, weapon, true);
    if ((error == FireError::OK || error == FireError::FACING) && CanDeployNow()) {
        QueueMission(Mission::Unload, false);
        return;
    }
    switch (error) {
        case FireError::OK:
            if (!Type->unknown_E10) IsFiring = false;
            if (Type->LargeVisceroid || Type->SmallVisceroid) {
                constexpr int frames[] = {100, 105, 110, 115, 120, 125, 90, 95};
                DirStruct direction;
                GetDirectionTo(&direction, Target);
                Animation.Value = frames[direction.GetValue<3>()];
                Animation.Rate = 5;
                Animation.Timer.Start(5);
            }
            Fire(Target, weapon);
            break;
        case FireError::FACING: {
            DirStruct direction;
            GetDirectionTo(&direction, Target);
            // 0x736F7E / 0x736F88: +0xE11 is OpenTS IsLockTurret (legacy
            // HasTurret name), +0xCA1 is Turret. A voxel body need not have a turret.
            if (!Type->HasTurret && Type->Turret)
                SecondaryFacing.SetDesired(direction);
            else if (!Destination) {
                if (!Locomotor) game::raise_com_error(static_cast<HRESULT>(0x80004003u));
                if (!Locomotor->Is_Moving()) {
                    PrimaryFacing.SetDesired(direction);
                    SecondaryFacing.SetDesired(PrimaryFacing.Desired());
                }
            }
            break;
        }
        case FireError::ILLEGAL:
            if (CombatDamage(weapon) < 0) {
                auto* object = Target && (Target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None
                                   ? static_cast<ObjectClass*>(Target)
                                   : nullptr;
                if (!object || object->WhatAmI() != AbstractType::Unit ||
                    object->GetHealthPercentage() >= RulesClass::Instance->ConditionGreen)
                    SetTarget(nullptr);
            }
            break;
        case FireError::CANT:
            if (SpawnManager) SpawnManager->ResetTarget();
            break;
        case FireError::RANGE:
        case FireError::MUST_DEPLOY:
            IsFiring = false;
            break;
        case FireError::CLOAKED:
            IsFiring = false;
            if (IsCloseEnough(Target, weapon)) Uncloak(false);
            break;
        default:
            break;
    }
    if (Type->IsGattling) {
        if (error == FireError::OK || error == FireError::REARM || error == FireError::FACING ||
            error == FireError::ROTATING)
            GattlingRateUp(1);
        else
            GattlingRateDown(1);
        if (GattlingValue > 0) advance();
    } else if (error == FireError::OK || error == FireError::REARM)
        advance();
}
