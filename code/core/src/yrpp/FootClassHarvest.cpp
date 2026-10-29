// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Goto_Tiberium/Search_For_Tiberium/Find_Docking_Bay;
// calibrated to YR 0x4DCE80..0x4DD0A0, 0x4DEE50..0x4DF040.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#if defined(RA2_YRPP_GAME)
#include "yrpp/SessionClass.h"
#else
#endif
#include <cmath>
#include <bit>

CellStruct* FootClass::ScanForTiberium(CellStruct* output, int range, DWORD) const {
    auto& map = MapClass::Instance;
    const auto center = GetMapCoords();
    *output = CellStruct::Empty;
    if (GetCell()->LandType == LandType::Tiberium) {
        *output = center;
        return output;
    }
#if defined(RA2_YRPP_GAME)
    const bool campaign = SessionClass::Instance.GameMode == GameMode::Campaign;
#else
    const bool campaign = SessionClass::Instance.GameMode == GameMode::Campaign;
#endif
    int value = -1;
    const auto inspect = [&](int dx, int dy) {
        CellStruct at{short(center.X + dx), short(center.Y + dy)};
        if (!map.IsWithinUsableArea(at, true)) return;
        // 0x4DCE80 excludes unexplored candidates only for the campaign player.
        if (campaign && IsOwnedByCurrentPlayer) {
            CoordStruct position{int(at.X) * 256 + 128, int(at.Y) * 256 + 128, 0};
            position.Z = map.GetCellFloorHeight(position);
            if (map.IsLocationShrouded(position)) return;
        }
        const auto from = CellClass::Coord2Cell(GetDestination());
        const bool fromBridge = IsOnBridge(nullptr);
        if (!map.IsSameCellZone(from, at, GetTechnoType()->MovementZone, fromBridge, false, false)) return;
        auto* cell = map.TryGetCellAt(at);
        if (!cell || cell->LandType != LandType::Tiberium ||
            IsCellOccupied(cell, FacingType(-1), -1, nullptr, true) != Move::OK)
            return;
        const int candidate = cell->GetContainedTiberiumValue();
        if (candidate > value) {
            value = candidate;
            *output = at;
        }
    };
    // Original examines a complete square ring, choosing the richest cell in
    // the first ring containing ore. Preserve edge order and strict tie-breaking.
    for (int radius = 1; radius < range && value == -1; ++radius)
        for (int offset = -radius; offset <= radius; ++offset) {
            inspect(offset, -radius);
            inspect(offset, radius);
            inspect(-radius, offset);
            inspect(radius, offset);
        }
    return output;
}
bool FootClass::MoveToTiberium(int radius, bool scanClose) {
    if (Destination) return false;
    CellStruct at;
    ScanForTiberium(&at, radius, scanClose);
    if (at == CellStruct::Empty) return false;
    if (at == GetMapCoords()) return true;
    SetDestination(MapClass::Instance.GetCellAt(at), true);
    return false;
}
BuildingClass* FootClass::FindCloserDockBuilding(BuildingTypeClass* type, DWORD, DWORD allowBusy, int* distance) const {
    if (!type || !Owner || !distance) return nullptr;
    BuildingClass* best = nullptr;
    // 0x4DEF26 bypasses the ground path-zone query for aircraft. The shared
    // distance is squared XY distance, and a primary factory wins a tie/order.
    if (WhatAmI() == AbstractType::Aircraft) {
        if (!Owner->CountOwnedNow(type)) return nullptr;
        for (auto* building : Owner->Buildings) {
            if (!building || building->InLimbo || building->Type != type ||
                (allowBusy != 1 && !building->HasFreeLink(this)))
                continue;
            if (const_cast<FootClass*>(this)->SendCommand(RadioCommand::QueryCanEnter, building) !=
                RadioCommand::AnswerPositive)
                continue;
            const auto here = GetCoords(), there = building->GetCoords();
            const unsigned x = unsigned(here.X) - unsigned(there.X);
            const unsigned y = unsigned(here.Y) - unsigned(there.Y);
            const int candidate = std::bit_cast<int>(x * x + y * y);
            if (*distance == -1 || candidate < *distance || building->IsPrimaryFactory) {
                best = building;
                *distance = candidate;
            }
        }
        return best;
    }
    auto& map = MapClass::Instance;
    const auto origin = GetDestination();
    const int zone = map.GetMovementZoneType(CellClass::Coord2Cell(origin), GetTechnoType()->MovementZone, OnBridge);
    for (auto* building : Owner->Buildings) {
        if (!building || building->InLimbo || !building->IsAlive || building->Type != type ||
            (!allowBusy && !HasFreeLink()))
            continue;
        // The refinery's docking cell is passable even though its center is not.
        const auto anchor = building->GetMapCoords();
        const CellStruct dock{short(anchor.X + 3), short(anchor.Y + 1)};
        if (map.GetMovementZoneType(dock, GetTechnoType()->MovementZone, false) != zone) continue;
        if (const_cast<FootClass*>(this)->SendCommand(RadioCommand::QueryCanEnter, building) !=
            RadioCommand::AnswerPositive)
            continue;
        const auto delta = building->GetCoords() - GetCoords();
        const int candidate =
            int(std::sqrt(double(delta.X) * delta.X + double(delta.Y) * delta.Y + double(delta.Z) * delta.Z));
        if (*distance == -1 || candidate < *distance || building->IsPrimaryFactory) {
            best = building;
            *distance = candidate;
        }
    }
    return best;
}
BuildingClass* FootClass::FindNearestDockBuilding(BuildingTypeClass* type, DWORD arg, DWORD busy) const {
    int distance = -1;
    return FindCloserDockBuilding(type, arg, busy, &distance);
}
BuildingClass* FootClass::TryNearestDockBuilding(TypeList<BuildingTypeClass*>* types, DWORD arg, DWORD busy) const {
    BuildingClass* result = nullptr;
    int distance = -1;
    if (types)
        for (auto* type : *types)
            if (auto* found = FindCloserDockBuilding(type, arg, busy, &distance)) result = found;
    return result;
}

// OpenTS Search_For_Weed / Weed_Check / Goto_Weed; YR 0x4DD890..0x4DDB90.
#include "yrpp/SessionClass.h"
#include "debug/debug_state.hpp"
#if !defined(RA2_YRPP_GAME)
bool FootClass::MoveToWeed(int radius) {
    if (Destination) return false;
    auto& map = MapClass::Instance;
    const auto center = CellClass::Coord2Cell(GetCoords());
    CellStruct selected = CellStruct::Empty;
    const auto check = [&](CellStruct& origin, int dx, int dy) {
        const CellStruct at{short(origin.X + dx), short(origin.Y + dy)};
        if (!map.IsWithinUsableArea(at, true)) return false;
        origin = at;
        if (SessionClass::Instance.GameMode == GameMode::Campaign && IsOwnedByCurrentPlayer) {
            CoordStruct position{int(at.X) * 256 + 128, int(at.Y) * 256 + 128, 0};
            position.Z = map.GetCellFloorHeight(position);
            if (game::debug_player_location_shrouded(position)) return false;
        }
        const auto from = CellClass::Coord2Cell(GetDestination());
        const bool bridge = IsOnBridge(nullptr);
        if (!map.IsSameCellZone(from, at, GetTechnoType()->MovementZone, bridge, false, false)) return false;
        auto* cell = map.GetCellAt(at);
        return IsCellOccupied(cell, FacingType(-1), -1, nullptr, true) == Move::OK &&
               cell->LandType == LandType::Weeds && cell->OverlayData >= 0x30;
    };
    auto origin = center;
    if (check(origin, 0, 0)) selected = origin;
    else {
        for (int r = 1; r < radius; ++r) {
            for (int i = -r; i <= r; ++i) {
                for (const CellStruct offset : {CellStruct{short(i), short(-r)}, CellStruct{short(i), short(r)},
                                               CellStruct{short(-r), short(i)}, CellStruct{short(r), short(i)}}) {
                    auto at = origin;
                    if (check(at, offset.X, offset.Y)) selected = at;
                }
            }
            if (selected != CellStruct::Empty) break;
        }
    }
    if (selected != CellStruct::Empty) {
        if (selected == CellClass::Coord2Cell(GetCoords())) return true;
        SetDestination(map.GetCellAt(selected), true);
    }
    return false;
}
#endif
