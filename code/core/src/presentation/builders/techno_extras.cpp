// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp; Copyright 2025 Electronic Arts Inc.;
// Copyright 2026 OpenTS contributors; EA terms: third_party/opents/LICENSE.md.
// Original TechnoClass::DrawExtras 0x006F5190, building and ordinary infantry display.
// Wrench uses its own animation palette and remains visible when unselected.
#include "yrpp/TechnoClass.h"
#include "presentation/builders/extras.hpp"
#include "presentation/context.hpp"
#include "bitfont_drawing.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/StringTable.h"
#include "yrpp/Drawing.h"
#include "techno_extras_drawing.hpp"
#include "type_drawing.hpp"
#include "surface_drawing.hpp"
#include "yrpp/Surface.h"
#include "map_runtime.hpp"
#include <algorithm>
#include <bit>
#include <cwchar>
namespace {
// (3 * from + to) / 4 per axis, with the 32-bit wrap and the truncating division
// of the original.
CoordStruct NearEnd(const CoordStruct& from, const CoordStruct& to) {
    const auto quarter = [](int a, int b) { return std::bit_cast<int>(3u * unsigned(a) + unsigned(b)) / 4; };
    return {quarter(from.X, to.X), quarter(from.Y, to.Y), quarter(from.Z, to.Z)};
}

// Calls the tactical line with the higher end first.
void DrawEdge(game::PresentationContext& ctx, const CoordStruct& end, const CoordStruct& anchor, int color) {
    if (anchor.Z <= end.Z)
        game::build_world_line(ctx, end, anchor, unsigned(color));
    else
        game::build_world_line(ctx, anchor, end, unsigned(color));
}

// Front half of the building selection box: four corner brackets, then the three
// quarter-length edges that rise to the top front corner.
void DrawBuildingSelectionFront(game::PresentationContext& ctx, const TechnoClass& techno, int color) {
    CoordStruct dimensions;
    techno.GetType()->Dimension2(&dimensions);
    const int x = dimensions.X / 2, y = dimensions.Y / 2, z = dimensions.Z;
    const CoordStruct center = techno.GetCoords();
    const auto sum = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
    const auto at = [&](int dx, int dy, int dz) {
        return CoordStruct{sum(center.X, dx), sum(center.Y, dy), sum(center.Z, dz)};
    };
    CoordStruct left = at(-x, y, 0), right = at(x, -y, 0), front = at(x, y, 0);
    CoordStruct topLeft = at(-x, y, z), topRight = at(x, -y, z), topFront = at(x, y, z);
    game::build_selection_bracket(ctx, &left, &front, color);
    game::build_selection_bracket(ctx, &right, &front, color);
    game::build_selection_bracket(ctx, &topLeft, &left, color);
    game::build_selection_bracket(ctx, &topRight, &right, color);
    DrawEdge(ctx, NearEnd(front, topFront), front, color);
    DrawEdge(ctx, NearEnd(topRight, topFront), topRight, color);
    DrawEdge(ctx, NearEnd(topLeft, topFront), topLeft, color);
}
}


namespace game {
bool build_world_line(PresentationContext& ctx, const CoordStruct& from, const CoordStruct& to, unsigned color) {
    if (ctx.extras && ctx.extras->world_line) {
        ctx.extras->world_line(ctx.extras->line_context, from, to, color);
        return true;
    }
    const auto project = [](const CoordStruct& at) {
        const auto screen = TacticalClass::CoordsToScreen(at);
        return Point2D{screen.X - TacticalClass::Instance->TacticalPos.X, screen.Y - TacticalClass::Instance->TacticalPos.Y};
    };
    const auto* bounds = map_runtime().drawing_bounds;
    submit_color_line(ctx, bounds ? *bounds : DSurface::ViewBounds, project(from), project(to), WORD(color),
                      14 - TacticalClass::AdjustForZ(from.Z), 14 - TacticalClass::AdjustForZ(to.Z));
    return true;
}
void build_selection_bracket(PresentationContext& ctx, CoordStruct* from, CoordStruct* to, int color) {
    DrawEdge(ctx, NearEnd(*from, *to), *from, color);
    DrawEdge(ctx, NearEnd(*to, *from), *to, color);
}

void build_techno_extras(PresentationContext& ctx, const TechnoClass& self, Point2D* point, RectangleStruct* bounds) {
    auto* frame = ctx.extras;
    if (!frame) return;
    if (!point || !bounds) {
        frame->status = game::DrawingStatus::invalid_argument;
        return;
    }
    if (self.WhatAmI() != AbstractType::Building && self.WhatAmI() != AbstractType::Infantry && self.WhatAmI() != AbstractType::Unit &&
        self.WhatAmI() != AbstractType::Aircraft) {
        frame->status = game::DrawingStatus::unsupported;
        return;
    }
    if (self.IsSinking) return;
    const auto record = [&](game::DrawingStatus status) {
        if (status != game::DrawingStatus::skipped) frame->status = status;
        return status == game::DrawingStatus::drawn || status == game::DrawingStatus::skipped;
    };
    if (self.WhatAmI() == AbstractType::Building) {
        const auto& building = *static_cast<const BuildingClass*>((&self));
        const auto* cell = MapClass::Instance.TryGetCellAt(self.Location);
        if (building.IsBeingRepaired && cell && !cell->IsShrouded()) {
            if (!frame->wrench || !frame->wrench_palette) {
                frame->status = game::DrawingStatus::unavailable;
                return;
            }
            const int period = std::max(2, GameOptionsClass::Instance.GetAnimSpeed(14) / 4);
            auto position = TacticalClass::CoordsToScreen(self.GetRenderCoords());
            position.X -= frame->camera.X;
            position.Y -= frame->camera.Y;
            game::ShapeDrawingRequest r;
            r.target = frame->drawing.target;
            r.palette = frame->wrench_palette;
            r.image = frame->wrench;
            r.position = position;
            r.frame = 6 * (Unsorted::CurrentFrame % period) / (period - 1);
            r.flags = 0xE00;
            r.clip = *bounds;
            if (!record(game::submit_type_shape(frame->drawing, r))) return;
        }
    }
    // 0x6F5372: rank insignia is visible even without selection or hover.
    if (!self.IsDisguisedAs(HouseClass::CurrentPlayer) && self.VisualCharacter(false, nullptr) != VisualType::Hidden)
        build_veterancy_pips(ctx, self, point, bounds);
    if (!game::drawing_completed(frame->status)) return;
    if (self.WhatAmI() == AbstractType::Building && self.IsSelected && frame->selection_palette) {
        DrawBuildingSelectionFront(ctx, *(&self), game::selection_color(*frame, self.GetHeight() < -4 ? 12 : 15));
        game::collect_line_status(ctx, *frame);
        if (!game::drawing_completed(frame->status)) return;
    }
    if (!frame->pips) return;
    if (self.IsSelected) build_health_bar(ctx, self, point, bounds, false);
    // 0x6F5E37: hover is a frame-local original object flag. The disguise
    // branch queries GetDisguiseHouse (vtable 0xD0), not GetDisguise (0xCC).
    else if (self.IsMouseHovering && (!self.IsDisguisedAs(HouseClass::CurrentPlayer) || self.GetDisguiseHouse(true)))
        build_health_bar(ctx, self, point, bounds, true);
    // Skip the original speech bubble at 0x6F5E8D: the checked stock YR maps and AI configurations do not invoke it.
}

void build_veterancy_pips(PresentationContext& ctx, const TechnoClass& self, Point2D* point, RectangleStruct* bounds) {
    auto* frame = ctx.extras;
    if (!frame || !point || !bounds) return;
    self.GetType();
    // The original negative-rank query also accepts an unordered comparison.
    const bool negative = !(self.Veterancy.Veterancy >= 0.0f);
    int index = self.Veterancy.IsElite() ? 15 : self.Veterancy.IsVeteran() ? 14 : negative ? 19 : -1;
    if (index < 0) return;
    if (!frame->pips || !frame->palette) {
        frame->status = game::DrawingStatus::unavailable;
        return;
    }
    game::ShapeDrawingRequest request;
    request.target = frame->drawing.target;
    request.palette = frame->palette;
    request.image = frame->pips;
    request.frame = index;
    request.position = {point->X + 5, point->Y + 2};
    if (self.WhatAmI() != AbstractType::Infantry) {
        request.position.X += 5;
        request.position.Y += 4;
    }
    request.clip = *bounds;
    request.flags = 0xE00;
    request.depth_adjustment = -2;
    request.intensity = 1000;
    const auto status = game::submit_type_shape(frame->drawing, request);
    if (status != game::DrawingStatus::skipped) frame->status = status;
}

void build_extra_info(PresentationContext& ctx, const TechnoClass& self, const Point2D& origin, const Point2D& center, const RectangleStruct& bounds) {
    auto* frame = ctx.extras;
    if (!frame || !self.Owner) return;
    const bool building = self.WhatAmI() == AbstractType::Building;
    const auto* type = building ? static_cast<const BuildingClass*>((&self))->Type : nullptr;
    const wchar_t* label = nullptr;
    wchar_t buffer[128]{};
    if (self.IsPrimaryFactory)
        label = StringTable::LoadString(type && type->GetFoundationWidth() == 1 ? "TXT_PRI" : "TXT_PRIMARY");
    else if (type && type->PowerBonus > 0) {
        const auto* format = StringTable::LoadString("TXT_POWER_DRAIN2");
        if (!format) return;
        std::swprintf(buffer, 128, format, int(self.Owner->Power_Output()), int(self.Owner->Power_Drain()));
        label = buffer;
    }
    if (!label || !*label) return;
    auto* font = BitFont::Instance;
    if (!font || !font->InternalPTR) {
        frame->status = game::DrawingStatus::unavailable;
        return;
    }
    int width = 0, height = 0;
    if (!font->GetTextDimension(label, &width, &height, 0)) return;
    const RectangleStruct box =
        Drawing::Intersect({center.X - width / 2 - 4, center.Y - 2, width + 8, height + 4}, bounds);
    const auto c = self.Owner->Color;
    const WORD color = WORD((c.R >> 3) << 11 | (c.G >> 2) << 5 | (c.B >> 3));
    const auto fill = [&](RectangleStruct rect, WORD value) {
        game::RasterDrawingRequest r;
        r.target = frame->drawing.target;
        r.position = {rect.X, rect.Y};
        r.clip = bounds;
        r.width = rect.Width;
        r.height = rect.Height;
        r.color = value;
        const auto s = game::submit_type_raster(frame->drawing, r);
        if (s != game::DrawingStatus::skipped) frame->status = s;
        return s == game::DrawingStatus::drawn || s == game::DrawingStatus::skipped;
    };
    if (box.Width <= 0 || box.Height <= 0) return;
    if (!fill(box, 0) || !fill({box.X, box.Y, box.Width, 1}, color) ||
        !fill({box.X, box.Y + box.Height - 1, box.Width, 1}, color) || !fill({box.X, box.Y, 1, box.Height}, color) ||
        !fill({box.X + box.Width - 1, box.Y, 1, box.Height}, color))
        return;
    int x = center.X - width / 2;
    for (auto* ch = label; *ch; ++ch) {
        int next = x;
        const auto s = game::submit_bitfont_glyph(*font, frame->drawing, *ch, x, center.Y, bounds, color, next);
        if (s != game::DrawingStatus::skipped) frame->status = s;
        if (s != game::DrawingStatus::drawn && s != game::DrawingStatus::skipped) return;
        x = next;
    }
}
}
