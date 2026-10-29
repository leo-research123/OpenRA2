// YR 0x6DA9D0 selected-building rally drawing. Dashed pixel traversal follows
// OpenTS 44fac744 xsurface.cpp, calibrated to DSurface 0x4C0750's ABuffer gates.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/TacticalClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Surface.h"
#include "type_drawing.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>

#include "presentation/builders/tactical_extras.hpp"
#include "presentation/context.hpp"
#if !defined(RA2_YRPP_GAME)
namespace {
int add(int a, int b) noexcept { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
int sub(int a, int b) noexcept { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
WORD pixel(BYTE r, BYTE g, BYTE b) noexcept {
    return WORD((r >> Drawing::RedShiftRight << Drawing::RedShiftLeft) |
                (g >> Drawing::GreenShiftRight << Drawing::GreenShiftLeft) |
                (b >> Drawing::BlueShiftRight << Drawing::BlueShiftLeft));
}
// Internal rendering helper, not an additional TacticalClass original entry.
game::DrawingStatus dashed_line(const game::TypeDrawingContext& drawing, const RectangleStruct& clip, Point2D a,
                                Point2D b, WORD color, int phase, bool foreground) noexcept {
    int increment = 1;
    if (a.X > b.X) {
        std::swap(a, b);
        phase = (phase + std::max(std::abs(a.X - b.X), std::abs(a.Y - b.Y) + 1)) % 16;
        increment = -1;
    }
    const int dx = b.X - a.X, dy = std::abs(b.Y - a.Y), sy = b.Y >= a.Y ? 1 : -1;
    const bool horizontal = dx > dy;
    const int count = std::max(dx, dy) + int(dx == 0 || dy == 0);
    int error = horizontal ? 2 * dy - dx : 2 * dx - dy;
    auto result = game::DrawingStatus::skipped;
    for (int i = 0; i < count; ++i) {
        phase = (phase + 16) % 16;
        // Original constant at 0x842930: five on, three off, repeated twice.
        if (phase % 8 < 5) {
            game::RasterDrawingRequest r;
            r.target = drawing.target;
            r.clip = clip;
            r.position = a;
            r.width = r.height = 1;
            r.color = color;
            r.blend_mode = foreground ? game::RasterBlendMode::light_zero : game::RasterBlendMode::light_nonzero;
            const auto status = game::submit_type_raster(drawing, r);
            if (!game::drawing_completed(status)) return status;
            if (status == game::DrawingStatus::drawn) result = status;
        }
        phase += increment;
        if (horizontal) {
            if (error > 0) {
                a.Y += sy;
                error -= 2 * dx;
            }
            error += 2 * dy;
            ++a.X;
        } else {
            if (error > 0) {
                ++a.X;
                error -= 2 * dy;
            }
            error += 2 * dx;
            a.Y += sy;
        }
    }
    return result;
}
}
namespace game {
void build_rally_points(PresentationContext& ctx, const TacticalClass& self, bool foreground) noexcept {
    try {
        const int phase = std::bit_cast<int>(0x7FFFFFFFu - unsigned(Unsorted::CurrentFrame)) % 15;
        // Black RGB constant at 0xA83CD9, initialized by 0x4E8120.
        const WORD black = pixel(0, 0, 0);
        for (int i = ObjectClass::CurrentObjects.Count - 1; i >= 0; --i) {
            auto* object = ObjectClass::CurrentObjects[i];
            if (object->WhatAmI() != AbstractType::Building || !object->IsAlive || !object->IsSelected) continue;
            auto* building = static_cast<BuildingClass*>(object);
            if (building->Owner != HouseClass::CurrentPlayer || !building->IsUnitFactory() || !building->ArchiveTarget)
                continue;
            const auto* drawing = ctx.type;
            if (!drawing) {
                game::record_type_drawing_result(ctx, game::DrawingStatus::unavailable);
                return;
            }
            const auto* resources = ctx.resources;
            auto clip = resources ? resources->clip : TacticalClass::ViewBounds;
            CoordStruct origin;
            object->GetCoords(&origin);
            auto first = TacticalClass::CoordsToScreen(origin);
            first = {add(sub(first.X, self.TacticalPos.X), clip.X), add(sub(first.Y, self.TacticalPos.Y), clip.Y)};
            CoordStruct target;
            building->ArchiveTarget->GetCoords(&target);
            auto& map = MapClass::Instance;
            target.Z = map.GetCellFloorHeight(target);
            if (unsigned(map.GetCellAt(target)->Flags) & 0x100u) target.Z = add(target.Z, CellClass::BridgeHeight);
            auto last = TacticalClass::CoordsToScreen(target);
            last = {add(sub(last.X, self.TacticalPos.X), clip.X), add(sub(last.Y, self.TacticalPos.Y), clip.Y)};
            first.Y = add(first.Y, 2);
            last.Y = add(last.Y, 2);
            for (int line = 0; line < 3; ++line) {
                const auto& color = building->Owner->Color;
                const WORD ink = line ? pixel(color.R, color.G, color.B) : black;
                // Clipping mutates both endpoints. The next stripe decrements these
                // clipped points, rather than re-projecting or starting from a saved copy.
                if (Line_In_Bounds(&first, &last, &clip)) {
                    const auto status = dashed_line(*drawing, clip, first, last, ink, phase, foreground);
                    game::record_type_drawing_result(ctx, status);
                    if (!game::drawing_completed(status)) return;
                }
                first.Y = sub(first.Y, 1);
                last.Y = sub(last.Y, 1);
            }
        }
    } catch (...) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::backend_failure);
    }
}
}

#endif
