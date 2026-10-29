// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 drive.cpp / voxel.cpp; YR 0x4AFF60 / 0x4B0410.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/DriveLocomotionClass.h"
#include "locomotor_drawing.hpp"
#include "yrpp/MapClass.h"
#include <cmath>
#include <cstring>
#include "voxel_ramp.hpp"
#include "x87_integer.hpp"
#include "Matrix3DArithmetic.hpp"
#include <bit>

namespace {
void set_key(VoxelIndexKey* key, int ramp, bool cacheable) {
    if (!key) return;
    unsigned raw;
    std::memcpy(&raw, key, 4);
    if (!cacheable)
        raw = 0xFFFFFFFFu;
    else if (raw != 0xFFFFFFFFu)
        raw = (raw << 6) + unsigned(ramp);
    std::memcpy(key, &raw, 4);
}
}
Matrix3D game::drive_drawing_matrix(DriveLocomotionClass& self, VoxelIndexKey* key, const TechnoTypeClass& type) {
    const auto slope_matrix = [&] {
        const double ratio = self.SlopeTimer.GetRatePassed();
        if (ratio < 1.0) return game::voxel_ramp_matrix(self.PreviousRamp, self.CurrentRamp, ratio);
        return self.CurrentRamp ? Matrix3D::VoxelRampMatrix[self.CurrentRamp] : Matrix3D::GetIdentity();
    };
    const double ratio = self.SlopeTimer.GetRatePassed();
    const float side = self.LinkedTo->AngleRotatedSideways, front = self.LinkedTo->AngleRotatedForwards;
    if (ratio == 1.0 && std::abs(side) < 0.005 && std::abs(front) < 0.005) {
        set_key(key, self.CurrentRamp, true);
        const auto facing = self.LocomotionClass::Draw_Matrix(key);
        return drawing_matrix_product(slope_matrix(), facing);
    }
    const float y = matrix_store_float(type.VoxelScaleX);
    const float x = matrix_store_float(type.VoxelScaleY);
    const float fc = matrix_store_float(Math::cos(front)), fs = matrix_store_float(Math::sin(front));
    const float sc = matrix_store_float(Math::cos(side));
    const double ss = Math::sin(side);
    auto vertical = Matrix3D::GetIdentity(), tilt = Matrix3D::GetIdentity();
    const int height = game::x87_integer(std::abs(ss) * y + std::abs(double(fs)) * x);
    int forward = game::x87_integer(double(x) - game::x87_integer(double(fc) * x));
    int sideways = game::x87_integer(double(y) - game::x87_integer(double(sc) * y));
    if (front < 0) forward = std::bit_cast<int>(0u - unsigned(forward));
    if (side > 0) sideways = std::bit_cast<int>(0u - unsigned(sideways));
    vertical.TranslateZ(float(height));
    tilt.TranslateX(float(forward));
    tilt.TranslateY(float(sideways));
    tilt.RotateX(side);
    tilt.RotateY(front);
    set_key(key, 0, false);
    const auto facing = self.LocomotionClass::Draw_Matrix(key);
    const auto slope = slope_matrix();
    return drawing_matrix_product(drawing_matrix_product(drawing_matrix_product(vertical, slope), facing), tilt);
}
Matrix3D YRPP_STDCALL DriveLocomotionClass::Draw_Matrix(VoxelIndexKey* key) { return game::drive_drawing_matrix(*this, key, *LinkedTo->GetTechnoType()); }
Matrix3D YRPP_STDCALL DriveLocomotionClass::Shadow_Matrix(VoxelIndexKey* key) {
    const bool cacheable = SlopeTimer.GetRatePassed() == 1.0 && std::abs(LinkedTo->AngleRotatedSideways) < 0.005 &&
                           std::abs(LinkedTo->AngleRotatedForwards) < 0.005;
    if (!cacheable && key) key->Invalidate();
    return LocomotionClass::Shadow_Matrix(key);
}
