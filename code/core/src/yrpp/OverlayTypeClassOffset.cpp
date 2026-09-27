// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "yrpp/OverlayTypeClass.h"

// OpenTS 44fac744 overlay.cpp::Overlay_Draw_Offset, calibrated to YR 0x005FDCC0.
// GPL-3.0-or-later with EA Section 7; code/third_party/opents/LICENSE.md.
Point2D* YRPP_FASTCALL OverlayTypeClass::GetDrawOffset(Point2D* output, int index) {
    const auto* type = Array.GetItemOrDefault(index);
    int y = 0;
    if (type) {
        if (type->Tiberium || type->Wall || type->ArrayIndex == 126 || type->Crate) y = -12;
        if (type->LandType == LandType::Railroad) --y;
    }
    if (index == 126) --y;
    *output = {0, y};
    return output;
}

