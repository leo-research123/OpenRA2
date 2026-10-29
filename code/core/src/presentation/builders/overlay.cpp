// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// Background traversal derives from OpenTS 44fac744 tactical.cpp, calibrated to YR 0x006D6D10 (DrawOverlays).
#include "presentation/builders/overlay.hpp"
#include "presentation/builders/cell.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Drawing.h"
#include "tactical_drawing.hpp"
#include "overlay_drawing.hpp"
#include "type_drawing.hpp"
#include <algorithm>

namespace game {
void build_overlays(PresentationContext& ctx, TacticalClass& tactical, const RectangleStruct& area) {
    auto* frame = ctx.frame;
    try {
        // YR 0x006D6D10 differs from OpenTS: base (-4,-2), last row
        // height/15+21 inclusive. Reverse rows, ascending columns; bodies then shadows.
        const auto bounds = frame && frame->bounds.Width > 0 ? frame->bounds : TacticalClass::ViewBounds;
        const auto clip = Drawing::Intersect(bounds, area);
        auto at = tactical.ApplyMatrix_Pixel({tactical.TacticalPos.X + area.X - bounds.X, tactical.TacticalPos.Y + area.Y - bounds.Y});
        at.X = std::max(at.X, 0);
        at.Y = std::max(at.Y, 0);
        const auto origin = MapClass::Instance.GetCellAt(CoordStruct{at.X, at.Y, 0})->MapCoords;
        const CellStruct base{short(origin.X - 4), short(origin.Y - 2)};
        for (bool shadow : {false, true})
            for (int row = area.Height / 15 + 21; row >= 0; --row) {
                CellStruct cellAt{short(base.X + row / 2), short(base.Y + (row + 1) / 2)};
                for (int col = 0; col < area.Width / 60 + 4; ++col, ++cellAt.X, --cellAt.Y) {
                    if (!MapClass::Instance.CoordinatesLegal(cellAt)) continue;
                    auto* cell = MapClass::Instance.GetCellAt(cellAt);
                    if (cell->OverlayTypeIndex == -1) continue;
                    auto rect = cell_overlay_bounds(*cell, ctx.overlay ? ctx.overlay->tactical_rect.Y : bounds.Y, shadow);
                    rect = Drawing::Intersect(rect, clip);
                    if (rect.Width <= 0 || rect.Height <= 0) continue;
                    auto point = TacticalClass::CoordsToScreen(CoordStruct{cellAt.X * 256, cellAt.Y * 256, 0});
                    point.X -= tactical.TacticalPos.X + 30;
                    point.Y -= tactical.TacticalPos.Y;
                    if (shadow)
                        build_cell_overlay_shadow(ctx, *cell, point, clip);
                    else
                        build_cell_overlay(ctx, *cell, point, clip);
                    const auto* drawing = ctx.overlay;
                    const auto status = drawing ? drawing->status : game::DrawingStatus::skipped;
                    if (!drawing_completed(status)) {
                        game::record_tactical_drawing(ctx, game::record_drawing_failure(
                            status, shadow ? "Cell DrawOverlayShadow" : "Cell DrawOverlay"));
                        return;
                    }
                }
            }
    } catch (...) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}
}
