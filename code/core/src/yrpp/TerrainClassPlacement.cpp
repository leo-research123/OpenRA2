// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 terrain.cpp Mark/Unlimbo/Limbo; YR
// 0x71BFB0/0x71D000/0x71C930.
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/TacticalClass.h"
namespace {
void adjacency(CellStruct at, int amount) {
  for (int i = 0; i < 8; ++i) {
    const auto offset = Unsorted::AdjacentCell[i];
    MapClass::Instance
        .GetCellAt(CellStruct{short(at.X + offset.X), short(at.Y + offset.Y)})
        ->BlockedNeighbours += amount;
  }
}
} // namespace
bool TerrainClass::Mark(MarkType value) {
  if (!ObjectClass::Mark(value))
    return false;
  auto at = GetMapCoords();
  if (value == MarkType::Up)
    MapClass::Instance.RemoveContentAt(&at, this);
  else if (value == MarkType::Down || value == MarkType::ChangeRedraw)
    MapClass::Instance.AddContentAt(&at, this);
  return true;
}
bool TerrainClass::Unlimbo(const CoordStruct &where, DirType facing) {
  if (!Type || !ObjectClass::Unlimbo(where, facing))
    return false;
  adjacency(GetMapCoords(), 1);
  // 0x0071D000 stores absolute projected coordinates at +0xD8/+0xDC.
  const auto projected=TacticalClass::CoordsToScreen(GetCenterCoords());
  unknown_rect_D0.Width=projected.X;unknown_rect_D0.Height=projected.Y;
  return true;
}
bool TerrainClass::Limbo() {
  const auto at = GetMapCoords();
  if (!InLimbo)
    adjacency(at, -1);
  const bool result = ObjectClass::Limbo();
  if (auto *cell = MapClass::Instance.TryGetCellAt(at))
    cell->RecalcAttributes(-1);
  return result;
}
