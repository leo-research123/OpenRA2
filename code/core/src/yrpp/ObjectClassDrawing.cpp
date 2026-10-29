// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp::Render; YR 0x005F4B10.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "map_runtime.hpp"
#include <bit>
#include "yrpp/FileFormats/SHP.h"

RectangleStruct* ObjectClass::GetDimensions(RectangleStruct* output) const {
    *output = {};
    const auto& runtime = game::map_runtime();
    if (!runtime.drawing_bounds || !TacticalClass::Instance) return output;
    const auto coords = GetRenderCoords();
    Point2D point;
    TacticalClass::Instance->CoordsToClient(&coords, &point);
    auto* image = GetImage();
    if (!image) return output;
    auto rectangle = image->GetFrameBounds(0);
    rectangle.X += point.X - image->Width / 2;
    rectangle.Y += point.Y - image->Height / 2;
    const auto& viewport = *runtime.drawing_bounds;
    if (rectangle.X + rectangle.Width <= 0 || rectangle.Y + rectangle.Height <= 0 ||
        rectangle.X >= viewport.Width || rectangle.Y >= viewport.Height) return output;
    *output = rectangle;
    return output;
}

// The original skips an object that is not marked unless the pass is forced; no pass here is incremental, and
// NeedsRedraw is no longer read by the drawing.
bool ObjectClass::DrawIfVisible(RectangleStruct* bounds, bool, DWORD) const {
    const auto& runtime = game::map_runtime();
    if (!bounds || !runtime.drawing_bounds || !TacticalClass::Instance) return false;
    if (!(runtime.debug_map && *runtime.debug_map) && runtime.has_window && runtime.has_window() &&
        InLimbo)
        return false;
    CoordStruct coordinates;
    Point2D point{};
    if (!TacticalClass::Instance->CoordsToClient(GetRenderCoords(&coordinates), &point) &&
        WhatAmI() != AbstractType::ParticleSystem)
        return false;
    const auto& tactical = *runtime.drawing_bounds;
    Drawing::Intersect(bounds, *bounds, tactical);
    if (bounds->X > tactical.X)
        point.X = std::bit_cast<int>(unsigned(point.X) + unsigned(tactical.X) - unsigned(bounds->X));
    if (bounds->Y > tactical.Y)
        point.Y = std::bit_cast<int>(unsigned(point.Y) + unsigned(tactical.Y) - unsigned(bounds->Y));
    // Even an empty intersection reaches DrawIt and returns true in the EXE.
    DrawIt(&point, bounds);
    return true;
}
