// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 techno.cpp Draw_Voxel; YR 0x00706640. The drawing moved to presentation/builders/techno_common.cpp.
#include "yrpp/TechnoClass.h"
#include "type_drawing.hpp"
#include <cstdlib>

// The drawing moved to the presentation layer. A call here means a caller bypassed the builders.
void TechnoClass::DrawVoxel(const VoxelStruct&, DWORD, int, const IndexClass<VoxelIndexKey, VoxelCacheStruct*>&,
                            const RectangleStruct&, const Point2D&, const Matrix3D&, int, DWORD, DWORD) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, "TechnoClass::DrawVoxel called outside the builders");
    std::abort();
}
