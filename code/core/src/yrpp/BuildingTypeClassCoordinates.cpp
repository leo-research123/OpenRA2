// Existing placement virtual, YR 0x00464A70 -> Map 0x00578080 -> Cell 0x0047B3A0.
// Preserve X/Y; floor height includes the subcell ramp, not only Cell::Level.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/MapClass.h"
#include "building_render_rect.hpp"
#include <bit>

// OpenTS 44fac744 builtype.cpp Get_Draw_Rect; YR 0x465570.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
RectangleStruct* BuildingTypeClass::GetDrawRect(RectangleStruct* output) {
    RectangleStruct rect{unknown_1538, unknown_153C, unknown_1540, unknown_1544};
    if (rect.X || rect.Y || rect.Width || rect.Height) {
        *output = rect;
        return output;
    }
    // The computation is shared with the drawing, which asks for it without the memo below.
    if (!game::compute_building_type_draw_rect(*this, rect)) {
        *output = {};
        return output;
    }
    unknown_1538 = rect.X;
    unknown_153C = rect.Y;
    unknown_1540 = rect.Width;
    unknown_1544 = rect.Height;
    *output = rect;
    return output;
}

CoordStruct* BuildingTypeClass::vt_entry_6C(CoordStruct* dest, CoordStruct* source) const {
    if (!dest || !source) return nullptr;
    const auto location = *source;
    const auto* cell = MapClass::Instance.GetCellAt(location);
    *dest = {location.X, location.Y, cell->GetFloorHeight({location.X, location.Y})};
    return dest;
}
