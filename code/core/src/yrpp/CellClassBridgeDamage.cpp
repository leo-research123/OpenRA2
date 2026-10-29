// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 cell.cpp Destroy_Bridge/Set_Under_Bridge/On_Bridge_Collapse.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md. Calibrated to YR.
#include "background_invalidation.hpp"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/AnimClass.h"
#include "x87_integer.hpp"
#include <cstdlib>
#include <new>

#if !defined(RA2_YRPP_GAME)
void CellClass::BlowUpBridge() {
    if (Unsorted::ArmageddonMode) return;
    try {
        for (auto* object = FirstObject; object;) {
            auto* next = object->NextObject;
            object->ReceiveDamage(&object->Health, 0, RulesClass::Instance->C4Warhead, nullptr, true, true, nullptr);
            object = next;
        }
        for (auto* object = AltObject; object;) {
            auto* next = object->NextObject;
            object->DropAsBomb();
            object = next;
        }
        MapClass::Instance.CellStructs1.AddItem(MapCoords);
        auto& rules = *RulesClass::Instance;
        auto& random = ScenarioClass::Instance->Random;
        const auto pick = [&] { return double(random.RandomRanged(0, 0x7FFFFFFE)) * 4.656612877414201e-10; };
        if (!rules.BridgeExplosions.Count || !(pick() < 0.95)) return;
        CoordStruct at{int(MapCoords.X) * 256 + 128, int(MapCoords.Y) * 256 + 128,
                       Unsorted::LevelHeight * int(Level) + BridgeHeight};
        at.X = game::x87_integer(game::x87_truncated_sum(
            game::x87_truncated_product(game::x87_truncated_difference(pick(), 0.5), 50.0), at.X));
        at.Y = game::x87_integer(game::x87_truncated_sum(
            game::x87_truncated_product(game::x87_truncated_difference(pick(), 0.5), 50.0), at.Y));
        if (pick() < 0.5) {
            // Allocation precedes the type pick in the original.
            if (auto* storage = YRMemory::Allocate(sizeof(AnimClass))) {
                const int index = random.RandomRanged(0, rules.MetallicDebris.Count - 1);
                ::new (storage) AnimClass(rules.MetallicDebris[index], at, 0, 1, 0x600, 0, false);
            }
        }
        if (auto* storage = YRMemory::Allocate(sizeof(AnimClass))) {
            const int delay = random.RandomRanged(1, 5);
            const int index = random.RandomRanged(0, rules.BridgeExplosions.Count - 1);
            ::new (storage) AnimClass(rules.BridgeExplosions[index], at, delay, 1, 0x600, 0, false);
        }
    } catch (...) {
        std::abort();
    }
}

void CellClass::SetBridge(FacingType direction, bool present) {
    try {
        const unsigned orientation = direction == FacingType::North ? 0x800u : 0u;
        const unsigned former = present ? 0u : 0x400u;
        const auto update = [&](CellClass* cell, unsigned clear, unsigned set) {
            // Direct bridge state changes can precede the enclosing redraw.
            game::invalidate_background_cell(cell->MapCoords, 0x1F);
            cell->Flags =
                static_cast<CellFlags>((unsigned(cell->Flags) & ~clear) | orientation | former | (present ? set : 0u));
            cell->OverlayData = present && direction != FacingType::North ? 9 : 0;
            if (!present) cell->BlowUpBridge();
            RadarClass::Instance.RadarBackground(cell->MapCoords);
        };
        // The deck cell retains its borrowed owner pointer.
        update(this, 0x11F80u, 0x11380u);
        auto* first = GetNeighbourCell(direction);
        first->BridgeOwnerCell = present ? this : nullptr;
        update(first, 0x11F00u, 0x11300u);
        auto* second = first->GetNeighbourCell(direction);
        second->BridgeOwnerCell = present ? this : nullptr;
        update(second, 0x11F00u, 0x11100u);
        auto* third = second->GetNeighbourCell(direction);
        game::invalidate_background_cell(third->MapCoords, 0x1F);
        third->Flags = static_cast<CellFlags>((unsigned(third->Flags) & ~0x1000u) | (present ? 0x1000u : 0u));
        auto* back = GetNeighbourCell(static_cast<FacingType>((unsigned(direction) - 4u) & 7u));
        back->BridgeOwnerCell = present ? this : nullptr;
        update(back, 0x11F00u, 0x10300u);
        if (direction == FacingType::West) {
            auto* shadow = back->GetNeighbourCell(FacingType::East);
            game::invalidate_background_cell(shadow->MapCoords, 0x1F);
            shadow->BridgeOwnerCell = present ? this : nullptr;
            shadow->Flags = static_cast<CellFlags>((unsigned(shadow->Flags) & ~0x10000u) | (present ? 0x10000u : 0u));
        }
    } catch (...) {
        std::abort();
    }
}
void CellClass::SetWoodBridge(FacingType direction, bool present) {
    // 0x0047E470 has the same writes and calls as 0x0047E040.
    SetBridge(direction, present);
}

void CellClass::OnBridgeCollapse() {
    try {
        if (!(AltOccupationFlags & 0xFFu)) return;
        CoordStruct at{int(MapCoords.X) * 256 + 128, int(MapCoords.Y) * 256 + 128, 0};
        at.Z = MapClass::Instance.GetCellFloorHeight(at) + BridgeHeight;
        for (int x = -2; x < 3; ++x)
            for (int y = -2; y < 3; ++y) {
                auto* cell = MapClass::Instance.GetCellAt(CellStruct{short(MapCoords.X + x), short(MapCoords.Y + y)});
                if (cell == this) continue;
                for (bool bridge : {true, false}) {
                    for (auto* object = bridge ? cell->AltObject : cell->FirstObject; object;) {
                        auto* next = object->NextObject;
                        if ((object->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None) {
                            auto* foot = static_cast<FootClass*>(object);
                            if (!foot->Locomotor) std::abort();
                            if (foot->Locomotor->Is_Moving_Here(at)) {
                                int amount = foot->Health;
                                foot->ReceiveDamage(&amount, 0, RulesClass::Instance->C4Warhead, nullptr, true, true,
                                                    nullptr);
                            }
                        }
                        object = next;
                    }
                }
            }
    } catch (...) {
        std::abort();
    }
}
#endif
