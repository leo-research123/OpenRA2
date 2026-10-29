// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 anim.cpp::Draw_It.
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// YR 0x00422CA0. The drawing moved to presentation/builders/anim.cpp.
#include "yrpp/AnimClass.h"
#include "type_drawing.hpp"
#include <bit>
#include <cstdlib>

int AnimClass::GetZ() const {
    return std::bit_cast<int>(unsigned(Location.Z) + unsigned(OwnerObject ? OwnerObject->Location.Z : 0));
}
int AnimClass::GetEnd() const { return Type->End; }

// The drawing moved to the presentation layer (presentation/builders/anim.cpp). A call here means a caller
// bypassed the builders.
void AnimClass::DrawIt(Point2D*, RectangleStruct*) const {
    game::record_drawing_failure(game::DrawingStatus::unsupported, "AnimClass::DrawIt called outside the builders");
    std::abort();
}
