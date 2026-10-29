// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 terrain.cpp::Draw_It.
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// YR 0x0071C1B0 DrawIt, 0x0071CC50 DrawIfVisible, 0x0071C360 DrawAgain: the drawing moved to
// presentation/builders/terrain.cpp and presentation/object_draw.cpp.
#include "yrpp/TerrainClass.h"
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include "terrain_render_rect.hpp"
#include <cstdlib>

// The drawing moved to the presentation layer. A call here means a caller bypassed the builders.
[[noreturn]] static void terrain_draw_trap(const char* what) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, what);
    std::abort();
}
bool TerrainClass::DrawIfVisible(RectangleStruct*, bool, DWORD) const {
    terrain_draw_trap("TerrainClass::DrawIfVisible called outside the builders");
}
void TerrainClass::DrawAgain(const Point2D&, const RectangleStruct&) const {
    terrain_draw_trap("TerrainClass::DrawAgain called outside the builders");
}
void TerrainClass::DrawIt(Point2D*, RectangleStruct*) const {
    terrain_draw_trap("TerrainClass::DrawIt called outside the builders");
}

RectangleStruct* TerrainClass::GetRenderDimensions(RectangleStruct* output) {
    *output = game::terrain_render_rect(*this);
    return output;
}
