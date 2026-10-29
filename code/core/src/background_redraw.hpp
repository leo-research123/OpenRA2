#pragma once

#include "background_invalidation.hpp"
#include "yrpp/CellClass.h"
#include "map_world.hpp"

namespace game {
// The cell fields the background records read (the audit's CellFields), with the Flags bits it exempts
// (0x40002B: occupancy and visibility) masked off. Equal inputs mean the cell's records cannot differ.
struct BackgroundCellInputs {
    int tile, overlay;
    unsigned flags;
    unsigned char height, slope, overlay_data;
    signed char level;
    explicit BackgroundCellInputs(const CellClass& cell)
        : tile(cell.IsoTileTypeIndex), overlay(cell.OverlayTypeIndex),
          flags(static_cast<unsigned>(cell.Flags) & ~0x40002Bu), height(static_cast<unsigned char>(cell.Height)),
          slope(cell.SlopeIndex), overlay_data(cell.OverlayData), level(static_cast<signed char>(cell.Level)) {}
    bool operator==(const BackgroundCellInputs&) const = default;
};
// RecalcAttributes for a caller whose own work since `before` was taken is the only other writer of those
// fields: the cell is invalidated only if its inputs differ afterwards. Plain RecalcAttributes keeps
// invalidating unconditionally, which callers that change fields before calling it rely on.
void recalc_cell_attributes_tracked(CellClass& cell, const BackgroundCellInputs& before);
// Original overlay transitions register a dirty rectangle for the cached background redraw. The native
// background is recorded per cell: a cell's overlay and its shadow are drawn from that cell's own fields
// (CellClass::DrawOverlay and DrawOverlayShadow read no neighbour), so the exact cell is the whole
// notification. Neighbours that change with it (wall connections) register themselves.
inline void register_background_overlay_redraw(CellClass& cell) {
    invalidate_background_cell(cell.MapCoords, 0x6);
    map_object_changed();
}
}
