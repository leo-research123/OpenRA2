// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// Adapted from OpenTS 44fac744 overlay.cpp::Which_Tiberium_Type.
// YR 0x005FDD20. The release diagnostic calls nullsub_1 (0x004068E0).
// An unmatched overlay marked Tiberium falls back to resource zero, not -1.
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
int TiberiumClass::FindIndex(int overlay) {
    const auto* type = OverlayTypeClass::Array.GetItemOrDefault(overlay);
    if (!type || !type->Tiberium) return -1;
    for (const auto* resource : Array) {
        if (!resource || !resource->Image) continue;
        const int first = resource->Image->ArrayIndex;
        if (overlay >= first && overlay < first + resource->NumImages + resource->NumSlopes)
            return resource->ArrayIndex;
    }
    return 0;
}
