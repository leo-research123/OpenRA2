// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 techtype.cpp
// Legal_Placement; calibrated to YR 0x00716150.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/MapClass.h"

bool TechnoTypeClass::CanCreateHere(const CellStruct& position, HouseClass* owner) const {
    // 0x00B0EB58 is {-1,-1}; YRpp's generic Vector2D::Empty is {0,0}.
    if (position == CellStruct{-1, -1}) return false;
    const bool building = WhatAmI() == AbstractType::BuildingType;
    auto* type = building ? const_cast<BuildingTypeClass*>(static_cast<const BuildingTypeClass*>(this)) : nullptr;
    bool blocked = false, clear = false;
    for (auto* offset = GetFoundationData(true); offset && *offset != CellStruct{0x7FFF, 0x7FFF}; ++offset) {
        const CellStruct at{short(position.X + offset->X), short(position.Y + offset->Y)};
        auto* cell = MapClass::Instance.GetCellAt(at);
        const bool allowed = building ? cell->CanThisExistHere(SpeedType, type, owner)
            : cell->IsClearToMove(SpeedType, false, false, -1, ::MovementZone::Normal, -1, true);
        blocked |= !allowed;
        clear |= allowed;
    }
    // Tile-laying products need one usable cell; ordinary buildings need all.
    return type && type->ToTile ? clear : !blocked;
}
