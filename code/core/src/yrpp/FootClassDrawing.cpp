// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 foot.h::Draw_Object / foot.cpp::Draw_Voxel;
// YR 0x0041C090 / 0x004DAF10. The drawing moved to presentation/builders/techno_common.cpp.
#include "yrpp/FootClass.h"
#include "type_drawing.hpp"
#include <cstdlib>
ZGradient FootClass::GetZGradient() const { return Locomotor ? Locomotor->Z_Gradient() : ZGradient::Deg90; }
// The drawing moved to the presentation layer. A call here means a caller bypassed the builders.
[[noreturn]] static void foot_draw_trap(const char* what) {
    game::record_drawing_failure(game::DrawingStatus::unsupported, what);
    std::abort();
}
void FootClass::Draw_A_SHP(SHPStruct*, int, Point2D*, RectangleStruct*, DWORD, DWORD, DWORD, ZGradient, DWORD, int, DWORD,
                           SHPStruct*, DWORD, DWORD, DWORD, DWORD) {
    foot_draw_trap("FootClass::Draw_A_SHP called outside the builders");
}
void FootClass::Draw_A_VXL(VoxelStruct*, int, int, IndexClass<VoxelIndexKey, VoxelCacheStruct*>*, RectangleStruct*,
                           Point2D*, Matrix3D*, int, BlitterFlags, DWORD) {
    foot_draw_trap("FootClass::Draw_A_VXL called outside the builders");
}
