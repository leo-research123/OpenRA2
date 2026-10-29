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
#include <bit>

namespace {
bool bypass(ObjectTypeClass* type, int house, CellStruct* foundation, CellStruct* at) {
    return !HouseClass::CurrentPlayer || house != HouseClass::CurrentPlayer->ArrayIndex || Unsorted::ArmageddonMode ||
           !foundation || !at || *at == CellStruct{-1, -1} || !type || type->WhatAmI() != AbstractType::BuildingType;
}
bool build_off_ally() { return SessionClass::Instance.Config.BuildOffAlly; }
int wrap(unsigned value) { return std::bit_cast<int>(value); }
}

bool DisplayClass::PassesProximityCheck(ObjectTypeClass* object, int houseIndex, CellStruct* foundation,
                                        CellStruct* position) {
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
    for (int x = x0; x < x1; ++x)
        for (int y = y0; y < y1; ++y) {
            const CellStruct at{short(x), short(y)};
            if (at.X >= position->X && at.X < position->X + width && at.Y >= position->Y && at.Y < position->Y + height)
                continue;
            if (!Game::IsActive) continue;
            auto* cell = GetCellAt(at);
            for (auto* current = cell->FirstObject; current; current = current->NextObject) {
                if (current->WhatAmI() != AbstractType::Building) continue;
                const auto* building = static_cast<BuildingClass*>(current);
                if (building->Owner->ArrayIndex == houseIndex && building->Type->BaseNormal) found = true;
                // YR asks the anchor's owner about the builder, not the reverse.
                if (allowAlly && building->Owner->IsAlliedWith(house) && building->Type->EligibileForAllyBuilding)
                    found = true;
                break;  // Original GetBuilding returns only the first building.
            }
        }
    return found;
}

bool DisplayClass::PassesShroudCheck(ObjectTypeClass* object, int houseIndex, CellStruct* foundation,
                                     CellStruct* position) {
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

// YR 0x4A91B0: callers supply the cell resolved by the input surface. The
// original 0x6DA360 adjustment is an identity function in this game version.
CellStruct* DisplayClass::MoveFoundation(CellStruct* previous, const CellStruct* cell) {
    *previous = CurrentFoundation_CenterCell;
    if (!CurrentFoundation_Data) {
        CurrentFoundation_CenterCell = *cell;
        return previous;
    }
    if (*cell == CurrentFoundation_CenterCell) return previous;
    if (CurrentFoundation_CenterCell != CellStruct{0, 0}) {
        CellStruct at{short(CurrentFoundation_CenterCell.X + CurrentFoundation_TopLeftOffset.X),
                      short(CurrentFoundation_CenterCell.Y + CurrentFoundation_TopLeftOffset.Y)};
        MarkFoundation(&at, false);
    }
    CellStruct at{short(cell->X + CurrentFoundation_TopLeftOffset.X),
                  short(cell->Y + CurrentFoundation_TopLeftOffset.Y)};
    if (*cell != CellStruct{0, 0}) MarkFoundation(&at, true);
    CurrentFoundation_CenterCell = *cell;
    unknown_1180 =
        PassesProximityCheck(CurrentBuildingType, CurrentBuildingOwnerArrayIndex, CurrentFoundation_Data, &at);
    unknown_1181 = PassesShroudCheck(CurrentBuildingType, CurrentBuildingOwnerArrayIndex, CurrentFoundation_Data, &at);
    return previous;
}

void DisplayClass::RightMouseButtonUp(DWORD miniMap) {
    if (CurrentBuilding && (CurrentBuilding->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None) {
        CurrentBuilding = nullptr;
        CurrentBuildingType = nullptr;
        CurrentBuildingOwnerArrayIndex = -1;
        SetActiveFoundation(nullptr);
    } else if (RepairMode)
        SetRepairMode(0);
    else if (SellMode)
        SetSellMode(0);
    else if (PowerToggleMode)
        PowerToggleMode = false;
    else if (CurrentSWTypeIndex != -1)
        CurrentSWTypeIndex = -1;
    else if (PlanningMode)
        PlanningMode = false;
    else if (PlaceBeaconMode)
        SetBeaconMode(0);
    else
        MapClass::UnselectAll();
    SetCursor(MouseCursorType::Default, bool(static_cast<BYTE>(miniMap)));
}
