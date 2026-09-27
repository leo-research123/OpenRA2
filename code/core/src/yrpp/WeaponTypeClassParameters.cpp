// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 weapon.cpp:
// Init_Max_Speed / Allowed_Threats. Adapted to YR 0x007729F0 / 0x00772A90.
// EA Section 7 terms and warranty disclaimers: third_party/opents/LICENSE.md.
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/YRMath.h"
#include "RulesClassReaders.hpp"
#include <cstdlib>

bool WeaponTypeClass::IsWallDestroyer() const { return Warhead && Warhead->Wall; }

int WeaponTypeClass::GetSpeed(int distance) const {
    if(!Projectile || Projectile->ROT)return Speed;
    const double gravity=RulesClass::Instance->Gravity*(Projectile->Floater?0.5:1.0);
    return rule_integer(Math::sqrt(double(distance)*gravity*1.2));
}

void WeaponTypeClass::CalculateSpeed() {
    if (!Projectile || Projectile->ROT != 0) return;
    if (!RulesClass::Instance) std::abort(); // Required original world dependency.
    double gravity = RulesClass::Instance->Gravity;
    if (Projectile->Floater) gravity *= 0.5;
    Speed = rule_integer(Math::sqrt(double(Range) * gravity * 1.2));
}

ThreatType WeaponTypeClass::AllowedThreats() {
    if (!Projectile) std::abort(); // Original also requires a resolved projectile.
    auto result = Projectile->AA ? ThreatType::Air : ThreatType::Normal;
    if (Projectile->AG)
        result |= ThreatType::Infantry | ThreatType::Vehicles | ThreatType::Boats | ThreatType::Buildings;
    return result;
}
