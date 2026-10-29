// Copyright 2025 Electronic Arts Inc.
// Copyright 2026 OpenTS contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 additional terms and warranty disclaimers apply;
// see code/third_party/opents/LICENSE.md. Modified by OpenRA2, 2026.
// Adapted from OpenTS building.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR 0x0043D290 DrawIt, 0x0043DA80 Draw, 0x0043E900 GetZAdjustment. The drawing moved to
// presentation/builders/building.cpp.
#include "yrpp/BuildingClass.h"
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

// The drawing moved to the presentation layer (presentation/builders/building.cpp). A call here means a caller
// bypassed the builders.
[[noreturn]] static void building_draw_trap(const char* what) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, what);
    std::abort();
}
void BuildingClass::DrawIt(Point2D*, RectangleStruct*) const {
    building_draw_trap("BuildingClass::DrawIt called outside the builders");
}
void BuildingClass::Draw(const Point2D&, const RectangleStruct&) {
    building_draw_trap("BuildingClass::Draw called outside the builders");
}

int BuildingClass::GetZAdjustment() const {
    const int z = -TacticalClass::AdjustForZ(GetZ());
    return Type && Type->TurretAnimIsVoxel ? z + Type->BuildingAnim[9].ZAdjust : z;
}
