// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 builtype.cpp Legal_Placement; YR 0x00464AC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"

bool BuildingTypeClass::CanCreateHere(const CellStruct& cell, HouseClass* owner) const {
    return PlaceAnywhere || TechnoTypeClass::CanCreateHere(cell, owner);
}

bool BuildingTypeClass::CanPlaceHere(CellStruct* cell, HouseClass* owner) const {
    return cell && CanCreateHere(*cell, owner);
}

// 0x45EE70: 0 is clear, 1 requests allied occupants to scatter, 2 is blocked.
int BuildingTypeClass::CheckFoundation(const CellStruct& origin, HouseClass* owner) {
    if (origin == CellStruct::Empty) return 0;
    bool moving = false;
    for (auto* offset = GetFoundationData(true); *offset != CellStruct{0x7FFF, 0x7FFF}; ++offset) {
        const CellStruct at{short(origin.X + offset->X), short(origin.Y + offset->Y)};
        if (!MapClass::Instance.CoordinatesLegal(at)) continue;
        auto* cell = MapClass::Instance.GetCellAt(at);
        if (cell->OverlayTypeIndex != -1 && (this != RulesClass::Instance->WallTower || cell->OverlayTypeIndex != 2))
            return 2;
        auto* object = cell->FirstObject;
        if (!object) continue;
        if (object->WhatAmI() == AbstractType::Terrain) return 2;
        if ((object->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) continue;
        if (object->WhatAmI() == AbstractType::Building) {
            if (!static_cast<BuildingClass*>(object)->CanUpgrade(this, owner)) return 2;
        } else {
            if (!static_cast<TechnoClass*>(object)->Owner->IsAlliedWith(owner) ||
                (object->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None)
                return 2;
            moving = true;
            auto* foot = static_cast<FootClass*>(object);
            if (!foot->Destination || MapClass::Instance.GetCellAt(foot->Location) == foot->Destination)
                MapClass::Instance.GetCellAt(at)->ScatterContent(CoordStruct::Empty, true, true, false);
        }
    }
    return moving ? 1 : 0;
}
