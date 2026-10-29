// YR 0x486E70: tile, overlay and overlay-shadow bounds, then radar refresh.
#include "background_invalidation.hpp"
#include "map_world.hpp"
#include "yrpp/CellClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>

#if !defined(RA2_YRPP_GAME)
RectangleStruct* CellClass::GetTileRect(RectangleStruct* output) const {
    *output = {};
    auto* tactical = TacticalClass::Instance;
    if (!tactical) return output;
    Point2D point{};
    const CoordStruct corner{MapCoords.X * 256, MapCoords.Y * 256, 0};
    tactical->CoordsToClient(&corner, &point);
    *output = {point.X - 30, point.Y + DSurface::ViewBounds.Y - 15 * static_cast<signed char>(Level), 60, 30};
    if (IsoTileTypeIndex == 0xFFFF) return output;
    auto* type = IsometricTileTypeClass::Array.GetItemOrDefault(IsoTileTypeIndex);
    if (!type) return output;
    const auto* image = reinterpret_cast<const TMPStruct*>(type->GetImage());
    if (!image || image->Columns <= 0 || image->Rows <= 0) {
        *output = {};
        return output;
    }
    const int sub = std::min(int(static_cast<unsigned char>(Height)), image->Columns * image->Rows - 1);
    const TMPImage* tile = nullptr;
    if (image->GetSubTile(sub, tile) && (tile->Flags & 0x1u)) {
        const int extra = std::max(0, tile->Y - tile->ExtraY);
        output->Y -= extra;
        output->Height += extra;
    }
    return output;
}

void CellClass::MarkForRedraw() {
    // Preserve the owning cell even when the original bounds are empty. The native background is recorded per
    // cell and a cell's records read only its own fields, so this cell is the whole notification; the original
    // dirty rectangle (tile, overlay and shadow bounds) would only expand into the surrounding cells.
    game::invalidate_background_cell(MapCoords, std::uint32_t(game::BackgroundLayerMask::all));
    game::map_object_changed();
    RadarClass::Instance.RadarBackground(MapCoords);
}
#endif
