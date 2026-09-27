// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 builtype.cpp Legal_Placement; YR 0x00464AC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingTypeClass.h"

bool BuildingTypeClass::CanCreateHere(const CellStruct& cell, HouseClass* owner) const {
    return PlaceAnywhere || TechnoTypeClass::CanCreateHere(cell, owner);
}

bool BuildingTypeClass::CanPlaceHere(CellStruct* cell, HouseClass* owner) const {
    return cell && CanCreateHere(*cell, owner);
}
