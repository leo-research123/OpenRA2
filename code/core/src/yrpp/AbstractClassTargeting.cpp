// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.
// Copyright 2026 OpenTS contributors
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9, TechnoClass::Remove_Target.
// Modified for YR 0x70D4A0: Airstrike guard, names and original virtual dispatch.
// EA Section 7 terms apply; see third_party/opents/LICENSE.md.
#include "yrpp/AbstractClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/TeamClass.h"

void AbstractClass::BecomeUntargetable() {
    const auto* target = (AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None
        ? static_cast<const TechnoClass*>(this) : nullptr;
    for (int i = TechnoClass::Array.Count-1; i >= 0; --i) {
        auto* attacker = TechnoClass::Array[i];
        bool release = true;
        if (target && attacker->Airstrike && attacker->Airstrike->Target == this &&
            target->Health > 0)
            release = !target->IsAlive;
        if (attacker->Target != this || !release) continue;
        attacker->Mission_Revert();
        if (attacker->WhatAmI() == AbstractType::Aircraft && attacker->CurrentMission == Mission::Patrol) {
            attacker->MissionStatus = 0;
            static_cast<AircraftClass*>(attacker)->IsLocked = false;
        }
        // Restoring a suspended mission may already have assigned a new target.
        if (attacker->Target == this) attacker->SetTarget(nullptr);
    }
    for (int i = TeamClass::Array.Count-1; i >= 0; --i) {
        auto* team = TeamClass::Array[i];
        if (team->QueuedFocus == this) team->QueuedFocus = nullptr;
        if (team->Focus == this) team->Focus = nullptr;
    }
}
