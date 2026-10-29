// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp::Render; YR 0x0043CEA0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

// The visibility test and the dispatch to DrawIt / Draw moved to the presentation layer
// (presentation/object_draw.cpp, draw_object_if_visible). A call here means a caller bypassed it.
bool BuildingClass::DrawIfVisible(RectangleStruct*, bool, DWORD) const {
    game::record_drawing_failure(game::DrawingStatus::unsupported,
                                 "BuildingClass::DrawIfVisible called outside the builders");
    std::abort();
}
