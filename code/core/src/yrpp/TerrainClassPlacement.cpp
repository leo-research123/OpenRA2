// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 terrain.cpp Mark/Unlimbo/Limbo; YR
// 0x71BFB0/0x71D000/0x71C930.
#include "background_invalidation.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarClass.h"
namespace {
void adjacency(CellStruct at, int amount) {
    for (int i = 0; i < 8; ++i) {
        const auto offset = Unsorted::AdjacentCell[i];
        MapClass::Instance.GetCellAt(CellStruct{short(at.X + offset.X), short(at.Y + offset.Y)})->BlockedNeighbours +=
            amount;
    }
}
}  // namespace
bool TerrainClass::Mark(MarkType value) {
    game::invalidate_background_object(*this);
    if (!ObjectClass::Mark(value)) return false;
    auto at = GetMapCoords();
    if (value == MarkType::Up)
        MapClass::Instance.RemoveContentAt(&at, this);
    else if (value == MarkType::Down || value == MarkType::ChangeRedraw) {
        MapClass::Instance.AddContentAt(&at, this);
        const auto projected = TacticalClass::CoordsToScreen(GetRenderCoords());
        unknown_rect_D0.Width = projected.X;
        unknown_rect_D0.Height = projected.Y;
    }
    return true;
}
bool TerrainClass::Unlimbo(const CoordStruct& where, DirType facing) {
    if (!ObjectClass::Unlimbo(where, facing)) return false;
    adjacency({short(where.X / 256), short(where.Y / 256)}, 1);
    // 0x0071D000 stores absolute projected coordinates at +0xD8/+0xDC.
    const auto projected = TacticalClass::CoordsToScreen(GetRenderCoords());
    unknown_rect_D0.Width = projected.X;
    unknown_rect_D0.Height = projected.Y;
    auto* cell = MapClass::Instance.GetCellAt(where);
    if (cell->OverlayTypeIndex != -1 && OverlayTypeClass::Array[cell->OverlayTypeIndex]->Tiberium) {
        cell->OverlayTypeIndex = -1;
        cell->OverlayData = 0;
    }
    return true;
}
bool TerrainClass::Limbo() {
    game::invalidate_background_object(*this);
    if (!InLimbo) {
        adjacency(GetMapCoords(), -1);
        MapClass::Instance.GetCellAt(Location)->OccupationFlags &= ~0x40u;
    }
    const auto at = GetMapCoords();
    const bool result = ObjectClass::Limbo();
    auto& map = MapClass::Instance;
    map.GetCellAt(at)->RecalcAttributes(-1);
    if (!Unsorted::ScenarioInit) {
        map.ResetZones(at);
        map.RecalculateSubZones(at);
        RadarClass::Instance.RadarBackground(at);
    }
    return result;
}

void TerrainClass::MarkAllOccupationBits(const CoordStruct& coords) {
    if (!Type || !ScenarioClass::Instance) return;
    const int bits = ScenarioClass::Instance->Theater == TheaterType::Snow ? Type->SnowOccupationBits
                                                                         : Type->TemperateOccupationBits;
    const CellStruct at{short(coords.X / 256), short(coords.Y / 256)};
    MapClass::Instance.GetCellAt(at)->OccupationFlags |= (unsigned(bits) & 7u) << 2;
}

void TerrainClass::UnmarkAllOccupationBits(const CoordStruct& coords) {
    if (!Type || !ScenarioClass::Instance) return;
    const int bits = ScenarioClass::Instance->Theater == TheaterType::Snow ? Type->SnowOccupationBits
                                                                         : Type->TemperateOccupationBits;
    const CellStruct at{short(coords.X / 256), short(coords.Y / 256)};
    MapClass::Instance.GetCellAt(at)->OccupationFlags &= ~((unsigned(bits) & 7u) << 2);
}

Move TerrainClass::IsCellOccupied(CellClass* destination, FacingType, int, CellClass*, bool) const {
    const auto speed = Type->WaterBound ? SpeedType::Float : SpeedType::Track;
    for (const auto* offset = GetFoundationData(false); *offset != CellStruct{0x7FFF, 0x7FFF}; ++offset) {
        const CellStruct at{short(destination->MapCoords.X + offset->X), short(destination->MapCoords.Y + offset->Y)};
        if (!MapClass::Instance.GetCellAt(at)->CanThisExistHere(speed, nullptr, nullptr)) return Move::No;
    }
    return Move::OK;
}
