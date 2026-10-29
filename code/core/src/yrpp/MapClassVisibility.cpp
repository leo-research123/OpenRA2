// Fixed YR 586360 / 5864A0 / 5865E0. Height projects into shroud cells;
// odd half-levels consult both diagonal neighbours. No host visibility state.
#include "background_invalidation.hpp"
#include "visibility_grid.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/ScenarioClass.h"
#include <algorithm>
#include <bit>
#include <cstdlib>

namespace {
CellClass* projected_cell(const MapClass& map, const CoordStruct& world, int& level) {
    level = world.Z / Unsorted::LevelHeight;
    const int shift = level / 2 + (level & 1);
    return map.GetCellAt(CellStruct{std::bit_cast<short>(static_cast<unsigned short>(world.X / 256 - shift)),
                                    std::bit_cast<short>(static_cast<unsigned short>(world.Y / 256 - shift))});
}

}
bool MapClass::IsLocationShrouded(const CoordStruct& world) const {
    int level;
    auto* cell = projected_cell(*this, world, level);
    if (level & 1) {
        if ((cell->AltFlags & AltCellFlags::Mapped) != AltCellFlags{}) return false;
        cell = cell->GetNeighbourCell(static_cast<FacingType>(3));
    }
    return (cell->AltFlags & AltCellFlags::Mapped) == AltCellFlags{};
}
bool MapClass::IsLocationGapped(const CoordStruct& world) const {
    int level;
    auto* cell = projected_cell(*this, world, level);
    if (level & 1) {
        if (std::bit_cast<int>(cell->unknown_13C) <= 0) return false;
        cell = cell->GetNeighbourCell(static_cast<FacingType>(3));
    }
    return std::bit_cast<int>(cell->unknown_13C) > 0;
}
// This exact target's 5865E0 is xor al,al; ret 4. Gap dimming is 5864A0,
// not this disabled fog query, despite the old header's overlapping names.
bool MapClass::IsLocationFogged(const CoordStruct&) { return false; }

// YR 0x567DA0 recomputes the shroud-edge frame cache of a ring around the point. The frames are derived
// from cell flags only, and the flag writers (counted sight, one-shot reveals, bulk rewrites) now mark the
// cells whose inputs changed, so the caches are refreshed once per logic frame instead of per call.
void MapClass::RevealArea3(CoordStruct* coords, int startRadius, int radius, bool skipReveal) {
    radius = std::clamp(radius, 3, 11);
    startRadius = std::clamp(startRadius, 0, 8);
    const auto first = startRadius ? CellSpread::NumCells(static_cast<unsigned>(startRadius - 1)) : 0;
    const auto end = CellSpread::NumCells(static_cast<unsigned>(radius));
    if (skipReveal) return;
    if (first > end || !TacticalClass::Instance) std::abort();
    game::visibility_flush();
    // The cell the call is anchored at keeps the original immediate refresh (it is ring entry 0); the rest of the ring is covered
    // by the dirty-cell pass at the end of the logic frame.
    const auto project = [](int coordinate, int height) {
        const int delta = TacticalClass::AdjustForZ(height) / -30;
        return std::bit_cast<int>(static_cast<unsigned>(coordinate) + static_cast<unsigned>(delta) * 256u) / 256;
    };
    if (first) return;
    const CellStruct at{short(project(coords->X, coords->Z)), short(project(coords->Y, coords->Z))};
    auto* cell = GetCellAt(at);
    const char visibility = TacticalClass::Instance->GetOcclusion(at, false);
    if (visibility != cell->Visibility) {
        cell->Visibility = visibility;
        cell->VisibilityChanged = true;
        TacticalClass::Instance->RegisterCellAsVisible(cell);
    }
}

void MapClass::Reveal(HouseClass* house) {
    game::visibility_flush();
    if (house) HouseClass::Array[house->ArrayIndex]->MapIsClear = true;
    if (house && house != HouseClass::CurrentPlayer) return;
    const auto* session = &SessionClass::Instance;
    const bool reserved = session && (session->GameMode == GameMode::LAN || session->GameMode == GameMode::Internet) &&
                          session->MPGameMode && !session->MPGameMode->vt_entry_04();
    if (HouseClass::CurrentPlayer->Visionary) return;
    DisplayClass::Instance.WithdrawSharedSight(false, 0);
    HouseClass::CurrentPlayer->Visionary = true;
    auto& map = MapClass::Instance;
    map.CellIteratorReset();
    while (auto* cell = map.CellIteratorNext()) {
        game::invalidate_background_shroud(cell->MapCoords);
        const int x = cell->MapCoords.X, y = cell->MapCoords.Y;
        if (reserved && ((x == 7 && y == MapRect.Width + 5) || (x == 13 && y == MapRect.Width + 11) ||
                         (x == MapRect.Height + 13 && y == MapRect.Width + MapRect.Height - 15)))
            continue;
        cell->ShroudCounter = 0;
        cell->GapsCoveringThisCell = 0;
        cell->AltFlags = static_cast<AltCellFlags>(static_cast<unsigned>(cell->AltFlags) | 0x18u);
        cell->Flags = static_cast<CellFlags>(static_cast<unsigned>(cell->Flags) | 0x3u);
    }
    // Frame caches are stale from here on; counted sight re-entering the view recomputes them.
    game::visibility_cache_all_dirty();
    DisplayClass::Instance.RestoreSharedSight(false, 0);
    game::visibility_reconcile();
    MapClass::Instance.sub_657CE0();
    map.MarkNeedsRedraw(2);
}

void MapClass::Reshroud(HouseClass* house) {
    game::visibility_flush();
    if (house) HouseClass::Array[house->ArrayIndex]->MapIsClear = false;
    if (house && house != HouseClass::CurrentPlayer) return;
    DisplayClass::Instance.WithdrawSharedSight(false, 0);
    CellIteratorReset();
    while (auto* cell = CellIteratorNext()) {
        game::invalidate_background_shroud(cell->MapCoords);
        cell->AltFlags = static_cast<AltCellFlags>(static_cast<unsigned>(cell->AltFlags) & ~0x18u);
        cell->ShroudCounter = 1;
        cell->GapsCoveringThisCell = 0;
        cell->Flags = static_cast<CellFlags>(static_cast<unsigned>(cell->Flags) & ~0x3u);
    }
    // Frame caches are stale from here on; counted sight re-entering the view recomputes them.
    game::visibility_cache_all_dirty();
    DisplayClass::Instance.RestoreSharedSight(false, 0);
    game::visibility_reconcile();
    HouseClass::CurrentPlayer->Visionary = false;
    MapClass::Instance.sub_657CE0();
    MarkNeedsRedraw(2);
}

// YR 0x577C90 preserves counted sight and gaps, unlike the hard reset above.
void MapClass::ReshroudShroud(HouseClass* house) {
    game::visibility_flush();
    if (house) HouseClass::Array[house->ArrayIndex]->MapIsClear = false;
    if (house && house != HouseClass::CurrentPlayer) return;
    DisplayClass::Instance.WithdrawSharedSight(false, 0);
    CellIteratorReset();
    while (auto* cell = CellIteratorNext()) {
        game::invalidate_background_shroud(cell->MapCoords);
        cell->IncreaseShroudCounter();
        if (cell->ShroudCounter >= 0) {
            cell->AltFlags &= ~AltCellFlags(0x18);
            cell->Flags &= ~CellFlags(0x3);
        }
    }
    // Frame caches are stale from here on; counted sight re-entering the view recomputes them.
    game::visibility_cache_all_dirty();
    DisplayClass::Instance.RestoreSharedSight(false, 0);
    game::visibility_reconcile();
    HouseClass::CurrentPlayer->Visionary = false;
    MapClass::Instance.sub_657CE0();
    MarkNeedsRedraw(2);
}

void MapClass::RevealShroud(HouseClass* house) {
    game::visibility_flush();
    if (house) HouseClass::Array[house->ArrayIndex]->MapIsClear = true;
    if (house && house != HouseClass::CurrentPlayer) return;
    const auto& session = SessionClass::Instance;
    const bool reserved = (session.GameMode == GameMode::LAN || session.GameMode == GameMode::Internet) &&
                          session.MPGameMode && !session.MPGameMode->vt_entry_04();
    if (HouseClass::CurrentPlayer->Visionary) return;
    HouseClass::CurrentPlayer->Visionary = true;
    auto& map = Instance;
    map.CellIteratorReset();
    while (auto* cell = map.CellIteratorNext()) {
        game::invalidate_background_shroud(cell->MapCoords);
        const int x = cell->MapCoords.X, y = cell->MapCoords.Y;
        if (reserved && ((x == 7 && y == MapRect.Width + 5) || (x == 13 && y == MapRect.Width + 11) ||
                         (x == MapRect.Height + 13 && y == MapRect.Width + MapRect.Height - 15)))
            continue;
        DisplayClass::Instance.MapCellVisibility(&cell->MapCoords, HouseClass::CurrentPlayer);
    }
    game::visibility_cache_all_dirty();
    map.sub_657CE0();
    map.MarkNeedsRedraw(1);
}

CellClass* MapClass::FindShroudedCell(AbstractClass* origin) {
    const auto at = origin->GetCoords();
    const CellStruct center{short(at.X / 256), short(at.Y / 256)};
    CellStruct cells[24];
    int count = 0;
    const auto consider = [&](int x, int y) {
        if (count == 24) return;
        const CellStruct cell{short(x), short(y)};
        if (!IsWithinUsableArea(cell, true)) return;
        CoordStruct world{x * 256 + 128, y * 256 + 128, 0};
        world.Z = GetCellAt(world)->GetFloorHeight(Point2D{world.X, world.Y});
        if (IsLocationShrouded(world)) cells[count++] = cell;
    };
    const int limit = std::min(32, MapRect.Width + MapRect.Height);
    for (int radius = 0; radius < limit && count < 24; ++radius) {
        for (int x = -radius; x <= radius && count < 24; ++x) {
            consider(center.X + x, center.Y - radius);
            consider(center.X + x, center.Y + radius);
        }
        for (int y = -radius + 1; y <= radius - 1 && count < 24; ++y) {
            consider(center.X - radius, center.Y + y);
            consider(center.X + radius, center.Y + y);
        }
    }
    return GetCellAt(count ? cells[ScenarioClass::Instance->Random.RandomRanged(0, count - 1)] : center);
}

void MapClass::ShroudMap(HouseClass* house) {
    game::visibility_flush();
    if (house) HouseClass::Array[house->ArrayIndex]->MapIsClear = false;
    if (house && house != HouseClass::CurrentPlayer) return;
    CellIteratorReset();
    while (auto* cell = CellIteratorNext()) {
        game::invalidate_background_shroud(cell->MapCoords);
        cell->AltFlags &= ~AltCellFlags(0x18);
        cell->Flags &= ~CellFlags(0x3);
    }
    game::visibility_cache_all_dirty();
    DisplayClass::Instance.WithdrawSharedSight(false, 0);
    game::visibility_reconcile();
    HouseClass::CurrentPlayer->Visionary = false;
    MapClass::Instance.sub_657CE0();
    MarkNeedsRedraw(2);
}

int MapClass::GetZPos(CoordStruct* coords) {
    return MapClass::Instance.GetCellAt(*coords)->GetFloorHeight(Point2D{coords->X, coords->Y});
}
