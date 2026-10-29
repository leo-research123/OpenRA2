// YR 0x006D71E0 / 0x004801F0: per-view shroud state formerly consumed by drawing.
#include "shroud_cycle.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Drawing.h"
#include "cell_occlusion.hpp"
#include "debug/debug_state.hpp"
#include <algorithm>
#include <cstdlib>
namespace game {
void advance_shroud_view() noexcept {
    auto* tactical = TacticalClass::Instance;
    if (!tactical || debug_fog_hidden()) return;
    const auto bounds = TacticalClass::ViewBounds;
    if (bounds.Width <= 0 || bounds.Height <= 0) return;
    const auto camera = tactical->TacticalPos;
    auto at = tactical->ApplyMatrix_Pixel(camera);
    at.X = std::max(0, at.X); at.Y = std::max(0, at.Y);
    const CellStruct base{short(at.X / 256 - 2), short(at.Y / 256)};
    auto& map = MapClass::Instance;
    // The footprint of a cell is the 60x30 rectangle around its flat screen position,
    // CoordsToScreen({X * 256, Y * 256, 0}) = {30 * (X - Y), 15 * (X + Y)} for every short cell, and the cell is
    // handled when it meets the view. For view values far from the int range the wrapping Drawing::Intersect
    // decides instead of the plain comparison.
    constexpr int plain = 1 << 29;
    const bool plain_view = std::abs(camera.X) < plain && std::abs(camera.Y) < plain && std::abs(bounds.X) < plain &&
                            std::abs(bounds.Y) < plain && bounds.Width < plain && bounds.Height < plain;
    const int right = bounds.X + bounds.Width, bottom = bounds.Y + bounds.Height;
    for (int row = 0; row < bounds.Height / 15 + 17; ++row) {
        CellStruct cell{short(base.X + row / 2), short(base.Y + (row + 1) / 2)};
        for (int col = 0; col < bounds.Width / 60 + 4; ++col, ++cell.X, --cell.Y) {
            if (!map.CoordinatesLegal(cell)) continue;
            auto* item = try_table_cell(map, cell);
            if (!item) continue;
            const int flat_x = 30 * (int(cell.X) - int(cell.Y)), flat_y = 15 * (int(cell.X) + int(cell.Y));
            if (plain_view) {
                const int left = flat_x - camera.X - 30, top = bounds.Y + flat_y - camera.Y;
                // The footprint [left, left + 60) x [top, top + 30) must meet the view
                // [bounds.X, right) x [bounds.Y, bottom).
                if (left + 60 <= bounds.X || left >= right || top + 30 <= bounds.Y || top >= bottom) continue;
            } else {
                const RectangleStruct footprint{flat_x - camera.X - 30, bounds.Y + flat_y - camera.Y, 60, 30};
                const auto clip = Drawing::Intersect(bounds, footprint);
                if (clip.Width <= 0 || clip.Height <= 0) continue;
            }
            // GetOcclusion reads the neighbors only for a cell whose own bit is set.
            char shroud, fog;
            item->Visibility =
                occlusion_without_neighbors(*item, false, shroud) ? shroud : tactical->GetOcclusion(cell, false);
            item->Foggedness =
                occlusion_without_neighbors(*item, true, fog) ? fog : tactical->GetOcclusion(cell, true);
        }
    }
}
}
