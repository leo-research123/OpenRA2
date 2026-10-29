// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tactical.cpp Draw_Radial_Indicators; YR 0x006DBE20 adds
// placement-neighbour and superweapon rings.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TacticalClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <limits>

#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "clock.hpp"
#include "x87_integer.hpp"

#include "presentation/context.hpp"
#include "presentation/builders/radial.hpp"
#include "tactical_radial_cycle.hpp"
#include "surface_drawing.hpp"
#include "yrpp/Surface.h"
#include <cmath>
namespace {
ColorStruct fade(ColorStruct color, int amount) {
    color.R = BYTE(int(color.R) - int(color.R) * amount / 256);
    color.G = BYTE(int(color.G) - int(color.G) * amount / 256);
    color.B = BYTE(int(color.B) - int(color.B) * amount / 256);
    return color;
}
short distance(CellStruct from, BuildingClass& building) {
    const auto at = building.GetCoords();
    const double dx = std::bit_cast<short>(WORD(unsigned(from.X) - unsigned(at.X / 256)));
    const double dy = std::bit_cast<short>(WORD(unsigned(from.Y) - unsigned(at.Y / 256)));
    return std::bit_cast<short>(WORD(game::x87_integer(Math::sqrt(dx * dx + dy * dy))));
}
}
namespace game {
void build_radial_shape(PresentationContext& ctx, bool spokes, bool adjust, CoordStruct coords, ColorStruct color,
                                             float cells, bool pixels, bool truncate) {
    double range = cells;
    if (truncate) range = game::x87_integer(range);
    const int radius = game::x87_integer(pixels ? range : (range + 0.5) / Math::sqrt(2.0) * 60.0);
    Point2D center;
    TacticalClass::Instance->CoordsToClient(&coords, &center);
    const auto clip = DSurface::ViewBounds;
    center.X += clip.X;
    center.Y += clip.Y;
    if (radius <= 0 || center.X + radius <= clip.X || center.X - radius >= clip.X + clip.Width ||
        center.Y + radius / 2 <= clip.Y || center.Y - radius / 2 >= clip.Y + clip.Height)
        return;
    auto ellipse_color = color;
    if (adjust) {
        ellipse_color.R -= BYTE(int(ellipse_color.R) * 50 / 256);
        ellipse_color.G -= BYTE(int(ellipse_color.G) * 50 / 256);
        ellipse_color.B -= BYTE(int(ellipse_color.B) * 50 / 256);
    }
    const WORD packed = WORD((ellipse_color.R >> 3) << 11 | (ellipse_color.G >> 2) << 5 | (ellipse_color.B >> 3));
    game::submit_ellipse(ctx, clip, center, radius, radius / 2, packed);
    game::submit_ellipse(ctx, clip, center, radius - 1, radius / 2 - 1, packed);
    if (!spokes) return;
    constexpr double pi = 3.141592653589793;
    constexpr float opacity[]{std::bit_cast<float>(0x3D4CCCCCu), std::bit_cast<float>(0x3E4CCCCCu),
                              std::bit_cast<float>(0x3ECCCCCCu), 1.0f};
    const double rx = radius, ry = radius / 2;
    for (unsigned i = 0; i < 4; ++i) {
        const double time = std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) + i) * 0.005;
        const double angle = time - double(game::x87_integer(time / (2 * pi))) * (2 * pi);
        Point2D endpoint = center;
        if (std::abs(angle - pi / 2) < 0.001)
            endpoint.Y -= game::x87_integer(ry);
        else if (std::abs(angle - 3 * pi / 2) < 0.001)
            endpoint.Y += game::x87_integer(ry);
        else {
            const double slope = Math::tan(angle);
            const double x = Math::sqrt(1.0 / (slope * slope / (ry * ry) + 1.0 / (rx * rx)));
            double y = Math::sqrt((1.0 - x * x / (rx * rx)) * ry * ry);
            if (angle < pi) y = -y;
            endpoint.X += game::x87_integer(angle > pi / 2 && angle < 3 * pi / 2 ? -x : x);
            endpoint.Y += game::x87_integer(y);
        }
        game::submit_subtractive_line(ctx, clip, {center.X - clip.X, center.Y - clip.Y},
                                      {endpoint.X - clip.X, endpoint.Y - clip.Y}, color, -500, -500, true, true, true,
                                      opacity[i]);
    }
}

void build_building_radial(PresentationContext& ctx, const BuildingClass& self, DWORD drawLine) {
    const int range = self.GetRadialIndicatorRange();
    if (range <= 0 || !self.HasPower || !self.Owner->IsControlledByCurrentPlayer()) return;
    if (self.Type->ConcentricRadialIndicator) {
        auto color = self.Owner->Color;
        const unsigned phase = (game::clock_milliseconds() >> 1) & 0x3FF;
        if (!(phase & 0x200)) {
            if (!(phase & 0x100)) {
                const int ratio = (~phase) & 0xFF;
                color.R += BYTE((255 - color.R) * ratio / 256);
                color.G += BYTE((255 - color.G) * ratio / 256);
                color.B += BYTE((255 - color.B) * ratio / 256);
            }
            build_radial_shape(ctx, false, false, self.GetCoords(), color, float(range), false, true);
        }
        const unsigned radius =
            unsigned(game::x87_integer((double(range) + 0.5) / Math::sqrt(2.0) * 60.0)) * phase >> 10;
        if (radius > 32) build_radial_shape(ctx, false, false, self.GetCoords(), self.Owner->Color, float(radius), true, true);
    } else
        build_radial_shape(ctx, (drawLine & 0xFF) != 0, true, self.GetCoords(), self.Owner->Color, float(range), false, true);
}
void build_radial_indicators(PresentationContext& ctx) {
    for (auto* object : ObjectClass::CurrentObjects)
        if (object->GetType() && object->GetType()->HasRadialIndicator && object->WhatAmI() == AbstractType::Building) build_building_radial(ctx, *static_cast<const BuildingClass*>(object), 1);
    if (!GameOptionsClass::Instance.UnitActionLines) return;
    auto& display = DisplayClass::Instance;
    if (display.CurrentBuilding && display.CurrentBuilding->WhatAmI() == AbstractType::Building) {
        auto* building = static_cast<BuildingClass*>(display.CurrentBuilding);
        const int range = building->GetRadialIndicatorRange();
        if (range <= 0) return;
        const auto cell = display.CurrentFoundation_CenterCell;
        const auto at = MapClass::Instance.GetCellAt(cell)->GetCoords();
        const auto pulse = tactical_radial_state().building;
        build_radial_shape(ctx, false, true, at, fade(building->Owner->Color, pulse.value), float(range), false,
                                  true);
        auto& owned = HouseClass::CurrentPlayer->Buildings;
        int nearest = std::numeric_limits<int>::max();
        for (auto* other : owned)
            if (other && other->IsAlive && other != building && other->Type == building->Type) {
                const int d = distance(cell, *other);
                if (d <= 2 * range && d < nearest) nearest = d;
            }
        for (auto* other : owned)
            if (other && other->IsAlive && other != building && other->Type == building->Type &&
                distance(cell, *other) == nearest)
                build_building_radial(ctx, *other, 0);
        return;
    }
    if (display.CurrentSWTypeIndex == -1) return;
    const auto* type = SuperWeaponTypeClass::Array[display.CurrentSWTypeIndex];
    if (type->Range <= 0) return;
    auto at = MapClass::Instance.GetCellAt(display.CurrentFoundation_CenterCell)->GetCoords();
    at.X -= 126;
    at.Y -= 126;
    const auto pulse = tactical_radial_state().superweapon;
    const auto color = HouseClass::CurrentPlayer->Color;
    if (type->LineMultiplier <= 0)
        build_radial_shape(ctx, false, true, at, fade(color, pulse.value), type->Range, false, true);
    else {
        auto concentric = pulse;
        for (int i = 0; double(i) <= double(type->LineMultiplier) * type->Range; ++i) {
            concentric.step();
            build_radial_shape(ctx, false, true, at, fade(color, concentric.value), float(i) / type->LineMultiplier,
                                      false, false);
        }
    }
}
}
