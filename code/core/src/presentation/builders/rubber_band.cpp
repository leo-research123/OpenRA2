// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 tactical.cpp::Draw_Rubber_Band; YR 0x006DA180.
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include <algorithm>
#include <cstdlib>

#include "presentation/builders/tactical_extras.hpp"
#include "presentation/context.hpp"
namespace game {
void build_rubber_band(PresentationContext& ctx, const TacticalClass& self) {
    // 0x006DA195 tests the original band state, not a host mouse-drag flag.
    if (!self.Band.Left && !self.Band.Top) return;
    const auto* drawing = ctx.type;
    const auto* resources = ctx.resources;
    if (!drawing || !resources || !resources->normal_palette) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::unavailable);
        return;
    }
    const auto& clip = resources->clip;
    const int left = std::min(self.Band.Left, self.Band.Right) + clip.X, top = std::min(self.Band.Top, self.Band.Bottom) + clip.Y;
    const int width = std::abs(self.Band.Right - self.Band.Left) + 1, height = std::abs(self.Band.Bottom - self.Band.Top) + 1;
    // NormalDrawer->Convert_Pixel(15), RGB565 host target. The four fills are
    // the backend decomposition of the original Surface::Draw_Rect call.
    const auto color = resources->normal_palette->Entries[15];
    for (const auto rect : {RectangleStruct{left, top, width, 1}, RectangleStruct{left, top + height - 1, width, 1},
                            RectangleStruct{left, top, 1, height}, RectangleStruct{left + width - 1, top, 1, height}}) {
        game::RasterDrawingRequest r;
        r.target = drawing->target;
        r.clip = clip;
        r.position = {rect.X, rect.Y};
        r.width = rect.Width;
        r.height = rect.Height;
        r.color = std::uint16_t((color.R >> 3) << 11 | (color.G >> 2) << 5 | (color.B >> 3));
        const auto status = game::submit_type_raster(*drawing, r);
        game::record_type_drawing_result(ctx, status);
        if (!game::drawing_completed(status)) return;
    }
}
}
