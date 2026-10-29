#pragma once
// The helpers of the original background passes (YR 0x006D2DE0 and its siblings, OpenTS tactical.cpp) that the
// cell and object passes share: the status of the pass in progress, the view rectangle of the frame (both read from the context's frame), the overlap
// test and the cell walk of the original traversal. Header-only, used by TacticalClass::DrawShroud (still in the
// simulation files) and by the presentation passes.
#include "tactical_drawing.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>
namespace game {
inline bool tactical_pass_complete(const PresentationContext& ctx) {
    const auto* f = ctx.frame;
    return f && (f->status == DrawingStatus::drawn || f->status == DrawingStatus::skipped);
}
inline RectangleStruct tactical_pass_viewport(const PresentationContext& ctx) {
    const auto* f = ctx.frame;
    return f && f->bounds.Width > 0 ? f->bounds : TacticalClass::ViewBounds;
}
inline bool tactical_rects_overlap(const RectangleStruct& a, const RectangleStruct& b) {
    return a.Width > 0 && a.Height > 0 && b.Width > 0 && b.Height > 0 && a.X < b.X + b.Width && a.Y < b.Y + b.Height &&
           a.X + a.Width > b.X && a.Y + a.Height > b.Y;
}
// The cell traversal of the background passes: rows of the isometric diamond covering `area`.
template <class F>
void tactical_walk_cells(const PresentationContext& ctx, TacticalClass& tactical, const RectangleStruct& area, int extraRows, int extraColumns, int dy,
                         bool clampOrigin, F&& draw) {
    if (area.Width <= 0 || area.Height <= 0) return;
    const auto bounds = tactical_pass_viewport(ctx);
    auto at = tactical.ApplyMatrix_Pixel(
        {tactical.TacticalPos.X + area.X - bounds.X, tactical.TacticalPos.Y + area.Y - bounds.Y});
    if (clampOrigin) {
        at.X = std::max(0, at.X);
        at.Y = std::max(0, at.Y);
    }
    const CellStruct base{short(at.X / 256 - 2), short(at.Y / 256 + dy)};
    for (int row = 0; row < area.Height / 15 + extraRows && tactical_pass_complete(ctx); ++row) {
        CellStruct cell{short(base.X + row / 2), short(base.Y + (row + 1) / 2)};
        for (int col = 0; col < area.Width / 60 + extraColumns && tactical_pass_complete(ctx); ++col, ++cell.X, --cell.Y) {
            if (!MapClass::Instance.CoordinatesLegal(cell)) continue;
            auto* item = MapClass::Instance.TryGetCellAt(cell);
            if (!item) continue;
            const auto flat = TacticalClass::CoordsToScreen({cell.X * 256, cell.Y * 256, 0});
            draw(*item, Point2D{flat.X - tactical.TacticalPos.X - 30, flat.Y - tactical.TacticalPos.Y});
        }
    }
}
}
