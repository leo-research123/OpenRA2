// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Draw_Pre_Render; YR 0x006F60D0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "techno_extras_drawing.hpp"
#include "type_drawing.hpp"
#include <bit>
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Drawing.h"
#include "yrpp/Surface.h"
#include <cstdlib>


#include "presentation/builders/extras.hpp"
#include "presentation/context.hpp"
namespace game {
void build_techno_behind(PresentationContext& ctx, const TechnoClass& self, Point2D* point, RectangleStruct* bounds) {
    // YR has no TS limpet-color branch. The second RTTI query and the
    // dimensions/center virtuals are retained in their original order.
    if (self.WhatAmI() != AbstractType::Building || !self.IsSelected || self.WhatAmI() == AbstractType::Infantry) return;
    auto* frame = ctx.extras;
    if (!frame || !bounds) return;
    if (!frame->selection_palette) {
        frame->status = game::DrawingStatus::unavailable;
        return;
    }
    const int color = int(game::selection_color(*frame, self.GetHeight() < -4 ? 12 : 15));
    CoordStruct dimensions;
    self.GetType()->Dimension2(&dimensions);
    const int x = dimensions.X / 2, y = dimensions.Y / 2, z = dimensions.Z;
    const auto center = self.GetCoords();
    const auto sum = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
    const auto at = [&](int dx, int dy, int dz) {
        return CoordStruct{sum(center.X, dx), sum(center.Y, dy), sum(center.Z, dz)};
    };
    CoordStruct bottomBack = at(-x, -y, 0), topBack = at(-x, -y, z), bottomRight = at(x, -y, 0),
                bottomLeft = at(-x, y, 0), topLeft = at(-x, y, z), topRight = at(x, -y, z);
    build_selection_bracket(ctx, &bottomBack, &topBack, color);
    build_selection_bracket(ctx, &bottomBack, &bottomRight, color);
    build_selection_bracket(ctx, &bottomBack, &bottomLeft, color);
    build_selection_bracket(ctx, &topBack, &topLeft, color);
    build_selection_bracket(ctx, &topBack, &topRight, color);
    game::collect_line_status(ctx, *frame);
}
}
