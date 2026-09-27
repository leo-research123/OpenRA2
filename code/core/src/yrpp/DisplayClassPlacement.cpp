// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 display.cpp
// Passes_Proximity_Check / Passes_Shroud_Check; YR 0x004A8EB0/0x004A9070.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"
#include <bit>

namespace {
bool bypass(ObjectTypeClass* type, int house, CellStruct* foundation, CellStruct* at) {
    return !HouseClass::CurrentPlayer || house != HouseClass::CurrentPlayer->ArrayIndex
        || Unsorted::ArmageddonMode || !foundation || !at || *at == CellStruct{-1, -1}
        || !type || type->WhatAmI() != AbstractType::BuildingType;
}
bool build_off_ally() {
#if defined(RA2_YRPP_GAME)
    return SessionClass::Instance.Config.BuildOffAlly;
#else
    const auto* houses = game::scenario_runtime().houses;
    return houses && houses->session && houses->session->Config.BuildOffAlly;
#endif
}
int wrap(unsigned value) { return std::bit_cast<int>(value); }
}

bool DisplayClass::PassesProximityCheck(ObjectTypeClass* object, int houseIndex,
                                        CellStruct* foundation, CellStruct* position) {
    if (bypass(object, houseIndex, foundation, position)) return true;
    const auto* type = static_cast<BuildingTypeClass*>(object);
    const int width = type->GetFoundationWidth(), height = type->GetFoundationHeight(false);
    const unsigned adjacent = unsigned(type->Adjacent) + 1u;
    const int x0 = wrap(unsigned(position->X) - adjacent), y0 = wrap(unsigned(position->Y) - adjacent);
    const int x1 = wrap(unsigned(position->X) + adjacent + unsigned(width));
    const int y1 = wrap(unsigned(position->Y) + adjacent + unsigned(height));
    const bool allowAlly = build_off_ally();
    auto* house = HouseClass::Array.GetItemOrDefault(houseIndex);
    bool found = false;
    for (int x = x0; x < x1; ++x) for (int y = y0; y < y1; ++y) {
        const CellStruct at{short(x), short(y)};
        if (at.X >= position->X && at.X < position->X + width
            && at.Y >= position->Y && at.Y < position->Y + height) continue;
        if (!Game::IsActive) continue;
        auto* cell = GetCellAt(at);
        for (auto* current = cell->FirstObject; current; current = current->NextObject) {
            if (current->WhatAmI() != AbstractType::Building) continue;
            const auto* building = static_cast<BuildingClass*>(current);
            if (building->Owner->ArrayIndex == houseIndex && building->Type->BaseNormal) found = true;
            // YR asks the anchor's owner about the builder, not the reverse.
            if (allowAlly && building->Owner->IsAlliedWith(house) && building->Type->EligibileForAllyBuilding) found = true;
            break; // Original GetBuilding returns only the first building.
        }
    }
    return found;
}

bool DisplayClass::PassesShroudCheck(ObjectTypeClass* object, int houseIndex,
                                     CellStruct* foundation, CellStruct* position) {
    if (bypass(object, houseIndex, foundation, position)) return true;
    const auto* type = static_cast<BuildingTypeClass*>(object);
    const int width = type->GetFoundationWidth(), height = type->GetFoundationHeight(false);
    for (int x = position->X; x < position->X + width; ++x)
        for (int y = position->Y; y < position->Y + height; ++y) {
            CoordStruct at{short(x) * 256 + 128, short(y) * 256 + 128, 0};
            at.Z = GetCellFloorHeight(at);
            if (IsLocationShrouded(at)) return type->ToTile != nullptr;
        }
    return true;
}
