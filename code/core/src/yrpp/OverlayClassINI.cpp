// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in
// third_party/opents/LICENSE.md. OpenTS
// 44fac744f70235e0d5ddca107364a68f95132ce9 overlay.cpp Read_INI/Mark; YR
// 0x5FD2E0/0x5FC570. Native packed-map loading subset.
#include "map_world.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/Straws.h"
#include "yrpp/Unsorted.h"
#include <vector>
namespace {
bool unpack(CCINIClass &ini, const char *section,
            std::vector<unsigned char> &out) {
  if (!ini.GetSection(section))
    return true;
  std::vector<unsigned char> compressed(1024 * 1024);
  const auto n = ini.ReadUUBlock(section, compressed.data(), compressed.size());
  if (!n || n >= compressed.size())
    return false;
  BufferStraw source(compressed.data(), int(n));
  LCWStraw decoder(1, 8192);
  decoder.Get_From(source);
  return decoder.Get(out.data(), int(out.size())) == int(out.size());
}
} // namespace
bool OverlayClass::ReadINI(CCINIClass &ini,
                           unsigned int &rejectedRecords) noexcept {
  rejectedRecords = 0;
  try {
    auto &map = MapClass::Instance;
    std::vector<unsigned char> overlays(MapClass::MaxCells, 255),
        data(MapClass::MaxCells, 0);
    if (!unpack(ini, "OverlayPack", overlays) ||
        !unpack(ini, "OverlayDataPack", data))
      return false;
    for (int i = 0; i < map.Cells.Capacity; ++i)
      if (auto *cell = map.Cells[i])
        if (overlays[i] != 255) {
          auto *type = OverlayTypeClass::Array.GetItemOrDefault(overlays[i]);
          if (!type) {
            ++rejectedRecords;
            continue;
          }
          cell->OverlayTypeIndex = overlays[i];
          if (type->Wall)
            for (int face = 0; face < 8; ++face) {
              const auto offset = Unsorted::AdjacentCell[face];
              ++map.GetCellAt(CellStruct{short(cell->MapCoords.X + offset.X),
                                         short(cell->MapCoords.Y + offset.Y)})
                    ->BlockedNeighbours;
            }
          if (overlays[i] == OVERLAY_BRIDGEHEAD11 ||
              overlays[i] == OVERLAY_BRIDGEHEAD21)
            cell->InitializeBridge(FacingType::North);
          else if (overlays[i] == OVERLAY_BRIDGEHEAD12 ||
                   overlays[i] == OVERLAY_BRIDGEHEAD22)
            cell->InitializeBridge(FacingType::West);
          cell->RecalcAttributes(-1);
          if (type->CellAnim)
            new AnimClass(
                type->CellAnim,
                CoordStruct{cell->MapCoords.X * 256 + 128,
                            cell->MapCoords.Y * 256 + 128,
                            static_cast<signed char>(cell->Level) * 104},
                0, 1, 0x600, 0, false);
        }
    // The packed data pass follows all placements, including bridge neighbours.
    for (int i = 0; i < map.Cells.Capacity; ++i)
      if (auto *cell = map.Cells[i]) {
        cell->OverlayData = data[i];
        game::map_resource_changed(*cell);
      }
    return true;
  } catch (...) {
    return false;
  }
}
