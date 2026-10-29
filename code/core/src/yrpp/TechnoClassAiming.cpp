// OpenTS 44fac744 techno.cpp Barrel_Pitch / Should_Use_High_Arc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md. YR 0x006FDA00/0x0070D590.
#include "yrpp/TechnoClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "projectile_ballistics.hpp"
#if !defined(RA2_YRPP_GAME)
bool TechnoClass::ShouldUseHighArc(int index) const {
    const auto* weapon = GetWeapon(index)->WeaponType;
    if (!weapon) return true;
    if (weapon->Lobber) return true;
    if (Target) {
        const int dz = std::bit_cast<int>(unsigned(Target->GetCoords().Z) - unsigned(Location.Z));
        if (dz > 0) {
            const auto at = Target->GetCoords();
            const double x = std::bit_cast<int>(unsigned(Location.X) - unsigned(at.X));
            const double y = std::bit_cast<int>(unsigned(Location.Y) - unsigned(at.Y));
            return game::x87_integer(Math::sqrt(x * x + y * y)) < dz;
        }
    }
    return false;
}
DirStruct* TechnoClass::vt_entry_2C8(DirStruct* output, AbstractClass* target) const {
    output->Raw = WORD(unsigned(GetTechnoType()->FireAngle) << 8);
    if (target) {
        CoordStruct predicted;
        PredictTargetCoords(&predicted);
        const int index = GetTechnoType()->HasMultipleTurrets() ? CurrentWeaponNumber : 0;
        CoordStruct origin;
        vt_entry_300(&origin, index);
        const double x = std::bit_cast<int>(unsigned(predicted.X) - unsigned(origin.X));
        const double y = std::bit_cast<int>(unsigned(predicted.Y) - unsigned(origin.Y));
        const int z = std::bit_cast<int>(unsigned(predicted.Z) - unsigned(origin.Z));
        const auto* weapon = GetWeapon(index)->WeaponType;
        const double gravity =
            RulesClass::Instance->Gravity * (weapon->Projectile && weapon->Projectile->Floater ? 0.5 : 1.0);
        const int distance = game::x87_integer(Math::sqrt(x * x + y * y));
        const int speed = weapon->GetSpeed(distance);
        short pitch;
        if (!game::projectile_pitch(ShouldUseHighArc(index), speed, distance, z, gravity, pitch))
            pitch = std::bit_cast<short>(WORD(0xE000));
        output->Raw = static_cast<WORD>(pitch);
    }
    return output;
}
#endif
