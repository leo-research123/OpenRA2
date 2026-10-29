// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 builtype.cpp Get_Draw_Rect / building.cpp Get_Render_Rect; YR 0x465570 / 0x455C20.
#include "building_render_rect.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/TacticalClass.h"
#include <bit>
#if !defined(RA2_YRPP_GAME) && !defined(RA2_IMAGE_GAME)
#include "presentation/state.hpp"
#include "api/drawing_resources.hpp"
#endif

namespace game {
bool compute_building_type_draw_rect(BuildingTypeClass& type, RectangleStruct& out) {
    RectangleStruct rect{};
    auto* shape = type.GetImage();
    if (!shape) {
        out = {};
        return false;
    }
    const int width = shape->Width, height = shape->Height;
    const auto add = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
    const auto merge = [&](RectangleStruct next) {
        if (rect.Width <= 0 || rect.Height <= 0) {
            rect = next;
            return;
        }
        if (next.Width <= 0 || next.Height <= 0) return;
        if (rect.X > next.X) {
            rect.Width = add(add(rect.X, rect.Width), -next.X);
            rect.X = next.X;
        }
        if (rect.Y > next.Y) {
            rect.Height = add(rect.Height, add(rect.Y, -next.Y));
            rect.Y = next.Y;
        }
        // Preserve the original Union's extra pixel on right/bottom expansion.
        if (add(rect.X, rect.Width) < add(next.X, next.Width))
            rect.Width = add(add(add(next.X, -rect.X), next.Width), 1);
        if (add(rect.Y, rect.Height) < add(next.Y, next.Height))
            rect.Height = add(add(add(next.Y, -rect.Y), next.Height), 1);
    };
    for (int i = 0; i < shape->Frames; ++i)
        merge(shape->GetFrameBounds(i));
    // Native BuildingType INI loading already owns the buildup allocation.
    // The EXE path retains its demand-load/free policy at 0x465570.
    if (type.Buildup)
        for (int i = 0; i < type.Buildup->Frames; ++i)
            merge(type.Buildup->GetFrameBounds(i));
    if (type.BibShape) merge(type.BibShape->GetFrameBounds(0));
    rect.X = add(rect.X, -width / 2);
    rect.Y = add(rect.Y, -height / 2);
    out = rect;
    return true;
}
RectangleStruct building_type_draw_rect(BuildingTypeClass& type, PresentationState* state) {
    const RectangleStruct cached{type.unknown_1538, type.unknown_153C, type.unknown_1540, type.unknown_1544};
    if (cached.X || cached.Y || cached.Width || cached.Height) return cached;
#if !defined(RA2_YRPP_GAME) && !defined(RA2_IMAGE_GAME)
    const int index = type.ArrayIndex;
    if (state && index >= 0 && index < BuildingTypeClass::Array.Count && BuildingTypeClass::Array[index] == &type) {
        auto& entries = state->building_rects;
        if (entries.size() <= std::size_t(index)) entries.resize(std::size_t(index) + 1);
        auto& entry = entries[index];
        const auto generation = shape_resource_generation();
        if (!entry.valid || entry.type != &type || entry.generation != generation) {
            RectangleStruct computed{};
            entry.valid = compute_building_type_draw_rect(type, computed);
            entry.rect = computed;
            entry.type = &type;
            entry.generation = shape_resource_generation();
        }
        return entry.rect;
    }
#endif
    RectangleStruct computed{};
    compute_building_type_draw_rect(type, computed);
    return computed;
}
RectangleStruct place_building_draw_rect(RectangleStruct type_rect, const CoordStruct& render, const Point2D& camera) {
    Point2D point;
    TacticalClass::CoordsToClient(render, camera, TacticalClass::ViewBounds, point);
    type_rect.X = std::bit_cast<int>(unsigned(type_rect.X) + unsigned(point.X));
    type_rect.Y = std::bit_cast<int>(unsigned(type_rect.Y) + unsigned(point.Y));
    return type_rect;
}
RectangleStruct building_render_rect(const BuildingClass& building, PresentationState* state) {
    const auto render = building.GetRenderCoords();
    const auto camera = TacticalClass::Instance->TacticalPos;
    if (render == building.unknown_coord_64C && camera.X == building.unknown_int_658 &&
        unsigned(camera.Y) == building.unknown_65C)
        return building.unknown_rect_63C;
    return place_building_draw_rect(building_type_draw_rect(*building.Type, state), render, camera);
}
}
