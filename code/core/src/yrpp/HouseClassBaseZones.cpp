// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Recalc_Center / Which_Zone; calibrated to YR.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>

namespace {
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
int delta(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
int distance(const CoordStruct& a, const CoordStruct& b, bool radiusOrder) {
    const double x = delta(a.X, b.X), y = delta(a.Y, b.Y), z = delta(a.Z, b.Z);
    return game::x87_integer(Math::sqrt(radiusOrder ? x * x + z * z + y * y : x * x + y * y + z * z));
}
bool active(const BuildingClass* b) { return b && !b->InLimbo && b->Health > 0; }
}

void HouseClass::RecalculateBaseCenter() {
    BaseSpawnCell = CellStruct::Empty;
    Radius = 0;
    for (auto& zone : ZoneInfos)
        zone = {};
    if (OwnedBuildings <= 0) return;
    int weight = 0, x = 0, y = 0;
    const int count = Buildings.Count;
    for (int i = 0; i < count; ++i) {
        auto* b = Buildings[i];
        if (!active(b)) continue;
        const int n = b->Type->GetActualCost(this) / 1000 + 1;
        if (n <= 0) continue;
        weight = add(weight, n);
        // Preserve both original virtual coordinate queries for each weight unit.
        for (int j = 0; j < n; ++j) {
            x = add(x, b->GetCoords().X);
            y = add(y, b->GetCoords().Y);
        }
    }
    if (weight > 0) {
        const CellStruct average{short(x / weight / 256), short(y / weight / 256)};
        MapClass::Instance.NearByLocation(BaseSpawnCell, average, SpeedType(0), -1, MovementZone(0), false, 1, 1, false,
                                          false, false, true, CellStruct{}, false, false);
    }
    if (weight <= 1) {
        Radius = 512;
        return;
    }
    int total = 0;
    for (int i = 0; i < count; ++i)
        if (auto* b = Buildings[i]; active(b)) {
            CoordStruct center;
            GetBaseCenterCoords(&center);
            total = add(total, distance(center, b->GetCoords(), true));
        }
    Radius = std::max(512, total / weight);
    const int zoneCount = Buildings.Count;
    for (int i = 0; i < zoneCount; ++i)
        if (auto* b = Buildings[i]; active(b)) {
            const int zone = WhichZone(b->GetCoords());
            if (zone == -1) continue;
            ZoneInfos[zone].Armor = add(ZoneInfos[zone].Armor, b->GetAntiArmorValue());
            ZoneInfos[zone].Aircraft = add(ZoneInfos[zone].Aircraft, b->GetAntiAirValue());
            ZoneInfos[zone].Infantry = add(ZoneInfos[zone].Infantry, b->GetAntiInfantryValue());
        }
}

int HouseClass::WhichZone(const CoordStruct& coordinate) const {
    if (coordinate == CoordStruct::Empty) return -1;
    CoordStruct center;
    GetBaseCenterCoords(&center);
    const int length = distance(center, coordinate, false);
    if (length <= Radius) return 0;
    if (length > std::bit_cast<int>(unsigned(Radius) * 4u)) return -1;
    GetBaseCenterCoords(&center);
    const double angle = Math::atan2(double(center.Y) - coordinate.Y, double(coordinate.X) - center.X);
    const auto raw = static_cast<unsigned short>(game::x87_integer((angle - 1.570796326794897) * -10430.06004058427));
    const auto direction = static_cast<unsigned char>(((raw >> 7) + 1) >> 1);
    // YR 0x004FFCF1..0x004FFD32 has overlapping/impossible quadrant tests.
    // Preserve the executable rather than replacing them with TS's quadrants.
    if (direction > 0x20 && direction < 0xE0) return 1;
    if (direction > 0xA0 && direction < 0x60) return 3;
    return direction <= 0x20 ? 2 : 4;
}

bool HouseClass::CanRebuildBaseNode(const BaseNodeClass* node) const {
    const int index = node->BuildingTypeIndex;
    if (index < 0) return true;
    if (index >= BuildingTypeClass::Array.Count) return false;
    auto* type = BuildingTypeClass::Array[index];
    if (type->ResourceGatherer && type->ResourceDestination)
        return CountResourceGatherers < RulesClass::Instance->AISlaveMinerNumber[int(AIDifficulty)];
    if (SessionClass::Instance.GameMode == GameMode::Campaign) return true;
    if (type->Weapon[0].WeaponType) return true;
    if (type->Wall) {
        for (int i = 0; i < 8; ++i) {
            auto* cell = MapClass::Instance.GetCellAt(node->MapCoords)->GetNeighbourCell(FacingType(i));
            auto* building = cell->GetBuilding();
            if (building && building->Owner == this) return true;
        }
        return false;
    }
    return type->PowerBonus > 0 || add(RulesClass::Instance->AIRestrictReplaceTime, LATime) <= Unsorted::CurrentFrame;
}

void HouseClass::InvalidateBaseNodePosition(BuildingClass* building) {
    if (Unsorted::ArmageddonMode) return;
    const CellStruct cell{short(building->Location.X / 256), short(building->Location.Y / 256)};
    auto& nodes = Base.BaseNodes;
    for (int i = 0; i < nodes.Count; ++i) {
        if (nodes[i].BuildingTypeIndex != building->Type->ArrayIndex || nodes[i].MapCoords != cell) continue;
        for (int j = 0; j < nodes.Count; ++j)
            if (j != i && nodes[j].MapCoords == cell) nodes[j].MapCoords = CellStruct::Empty;
        if (building->Type->IsBaseDefense && SessionClass::Instance.GameMode != GameMode::Campaign) {
            nodes[i].BuildingTypeIndex = -1;
            nodes[i].MapCoords = CellStruct::Empty;
        }
        return;
    }
}

BuildingClass* HouseClass::FindBuildingOfType(int index, int sector) const {
    if (OwnedBuildingTypes.GetItemCount(index) <= 0) return nullptr;
    const int count = Buildings.Count;
    for (int i = 0; i < count; ++i) {
        auto* building = Buildings[i];
        if (!building || building->InLimbo) continue;
        int typeIndex = -1;
        for (int j = 0; j < BuildingTypeClass::Array.Count; ++j)
            if (BuildingTypeClass::Array[j] == building->Type) {
                typeIndex = j;
                break;
            }
        if (typeIndex == index && (sector == -1 || WhichZone(building->GetCoords()) == sector)) return building;
    }
    return nullptr;
}
