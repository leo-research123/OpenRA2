// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 cell.cpp
// Is_Clear_To_Build; calibrated to YR 0x0047C620, including naval terrain.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/CellClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Unsorted.h"

namespace {
// 0x0047C520/0x0047C4D0 inspect the ground chain including inactive objects.
// GetBuilding's existing native presentation fallback has different semantics.
ObjectClass* ground_object(const CellClass& cell, AbstractType kind) {
    if (Game::IsActive)
        for (auto* object = cell.FirstObject; object; object = object->NextObject)
            if (object->WhatAmI() == kind) return object;
    return nullptr;
}
}

bool CellClass::CanThisExistHere(::SpeedType speed, BuildingTypeClass* type, HouseClass* owner) const {
    if (Unsorted::ScenarioInit) return true;
    if (type) {
        if (type->LaserFence) {
            if (ground_object(*this, AbstractType::Building) || ground_object(*this, AbstractType::Terrain)) return false;
        } else if (type->LaserFencePost || type->Gate) {
            ObjectClass* object = GetSomeObject({0, 0}, false);
            if (object && (object->WhatAmI() != AbstractType::Building
                || !static_cast<BuildingClass*>(object)->Type->LaserFence || object->GetOwningHouse() != owner)) return false;
            if (OccupationFlags & 0x3Fu) return false;
        } else if (type->ToTile) {
            auto* tile = IsometricTileTypeClass::Array.GetItemOrDefault(IsoTileTypeIndex);
            if ((tile && !tile->Morphable) || ground_object(*this, AbstractType::Building)) return false;
        } else {
            if (GetAircraft(false) || FindTechnoNearestTo({0, 0}, false)
                || ground_object(*this, AbstractType::Terrain) || (OccupationFlags & 0x3Fu)) return false;
        }
    }
    const auto& map = MapClass::Instance;
    if (Unsorted::ArmageddonMode ? !map.CoordinatesLegal(MapCoords)
        : !map.IsWithinUsableArea(const_cast<CellClass*>(this), true)) return false;

    if (OverlayTypeIndex != -1) {
        auto* wallOwner = HouseClass::Array.GetItemOrDefault(WallOwnerIndex);
        const auto* rules = RulesClass::Instance;
        if (type) {
            const bool sameWall = type->ToOverlay && type->ToOverlay->ArrayIndex == OverlayTypeIndex && OverlayData >= 0x10;
            if ((OverlayTypeIndex == 0 || OverlayTypeIndex == 2)
                && (sameWall || type == rules->WallTower || type == rules->GDIGateOne || type == rules->GDIGateTwo)
                && wallOwner == owner) return true;
            if (OverlayTypeIndex == 26
                && (sameWall || type == rules->NodGateOne || type == rules->NodGateTwo)
                && wallOwner == owner) return true;
            if (type->LaserFence && (OverlayTypeIndex == 126 || GetContainedTiberiumIndex() != -1))
                return !(static_cast<unsigned>(Flags) & 0x500u) && !SlopeIndex;
        }
        const auto* overlay = OverlayTypeClass::Array.GetItemOrDefault(OverlayTypeIndex);
        if (!Unsorted::ArmageddonMode || !overlay || overlay->Wall) return false;
    }
    if (speed == ::SpeedType::None) {
        if ((static_cast<unsigned>(Flags) & 0x500u) || SlopeIndex) return false;
        // YR's naval branch uses the original 14 water tiles, not LandType.
        if (type && type->Naval)
            return IsoTileTypeIndex >= IsometricTileTypeClass::WaterSet
                && IsoTileTypeIndex < IsometricTileTypeClass::WaterSet + 14;
        return GroundType::Array[static_cast<int>(LandType)].Buildable;
    }
    return GroundType::Array[static_cast<int>(LandType)].Cost[static_cast<int>(speed)] != 0.0f;
}
