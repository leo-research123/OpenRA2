// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp Set_Occupy_Bit/Clear_Occupy_Bit.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"

void InfantryClass::MarkAllOccupationBits(const CoordStruct& coord) {
    auto* cell = MapClass::Instance.GetCellAt(coord);
    const unsigned bit = 1u << CellClass::InfantrySubpositionIndex(coord);
    const bool bridge = MapClass::Instance.GetCellFloorHeight(coord) + CellClass::BridgeHeight <= coord.Z
        && (cell->Flags & CellFlags::BridgeHead) != CellFlags{};
    auto& flags = bridge ? cell->AltOccupationFlags : cell->OccupationFlags;
    auto& owner = bridge ? cell->AltInfantryOwnerIndex : cell->InfantryOwnerIndex;
    flags |= bit;
    owner = GetOwningHouseIndex();
}

void InfantryClass::UnmarkAllOccupationBits(const CoordStruct& coord) {
    auto* cell = MapClass::Instance.GetCellAt(coord);
    const unsigned bit = 1u << CellClass::InfantrySubpositionIndex(coord);
    // YR clear (0x00521850), unlike set, does NOT test the bridge flag.
    const bool bridge = MapClass::Instance.GetCellFloorHeight(coord) + CellClass::BridgeHeight <= coord.Z;
    auto& flags = bridge ? cell->AltOccupationFlags : cell->OccupationFlags;
    auto& owner = bridge ? cell->AltInfantryOwnerIndex : cell->InfantryOwnerIndex;
    flags &= ~bit;
    if (!(flags & 0x1Cu)) owner = -1;
}
