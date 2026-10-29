// The two original 0x6DBE20 pulse accumulators have process lifetime.
#include "tactical_radial_cycle.hpp"
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
namespace game {
namespace { TacticalRadialState radial_state; }
const TacticalRadialState& tactical_radial_state() noexcept { return radial_state; }
void advance_tactical_radials() noexcept {
    if (!GameOptionsClass::Instance.UnitActionLines) return;
    const auto& display = DisplayClass::Instance;
    if (display.CurrentBuilding && display.CurrentBuilding->WhatAmI() == AbstractType::Building) {
        if (static_cast<const BuildingClass*>(display.CurrentBuilding)->GetRadialIndicatorRange() > 0)
            radial_state.building.step();
        return;
    }
    if (display.CurrentSWTypeIndex == -1) return;
    if (SuperWeaponTypeClass::Array[display.CurrentSWTypeIndex]->Range > 0) radial_state.superweapon.step();
}
}
