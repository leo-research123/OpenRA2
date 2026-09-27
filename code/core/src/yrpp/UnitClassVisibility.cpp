// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp::Render; YR 0x0073B0B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"

bool UnitClass::DrawIfVisible(RectangleStruct* bounds,bool forced,DWORD) const {
    if(IsTether) {
        const auto* radio=GetNthLink();
        if(radio && radio->WhatAmI()==AbstractType::Building
            && (radio->GetCurrentMission()==Mission::Unload || radio->QueuedMission==Mission::Unload)) {
            auto& door=const_cast<BuildingClass*>(static_cast<const BuildingClass*>(radio))->UnloadTimer;
            if(door.AreStates11() || door.AreStates10() || door.AreStates01() || door.AreStates00())return false;
        }
    }
    return ObjectClass::DrawIfVisible(bounds,forced,0);
}
