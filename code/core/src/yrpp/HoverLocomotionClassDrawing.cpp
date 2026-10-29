// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 hover.cpp / voxel.cpp; YR 0x513F40 / 0x5142A0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HoverLocomotionClass.h"
#include "locomotor_drawing.hpp"
#include "yrpp/MapClass.h"
#include "x87_integer.hpp"
#include "Matrix3DArithmetic.hpp"
#include <bit>

Matrix3D game::hover_drawing_matrix(HoverLocomotionClass& self, VoxelIndexKey* key, const TechnoTypeClass& type) {
    const float side = self.LinkedTo->AngleRotatedSideways, front = self.LinkedTo->AngleRotatedForwards;
    if (std::abs(side) < 0.005 && std::abs(front) < 0.005) {
        if (self.Is_Powered()) return self.LocomotionClass::Draw_Matrix(key);
        const auto ramp = MapClass::Instance.GetCellAt(self.LinkedTo->Location)->SlopeIndex;
        auto matrix = Matrix3D::VoxelRampMatrix[ramp];
        matrix.RotateZ(
            matrix_store_float((int(self.LinkedTo->PrimaryFacing.Current().GetValue<5>()) - 8) * -0.1963495408493621));
        if (key && key->Is_Valid_Key())
            key->Value = std::bit_cast<int>(((unsigned(key->Value) << 6) + unsigned(ramp)) * 32u |
                                            unsigned(self.LinkedTo->PrimaryFacing.Current().GetValue<5>()));
        return matrix;
    }
    if (key) key->Invalidate();
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
    const auto facing = self.LocomotionClass::Draw_Matrix(key);
    return drawing_matrix_product(drawing_matrix_product(vertical, facing), tilt);
}
Matrix3D YRPP_STDCALL HoverLocomotionClass::Draw_Matrix(VoxelIndexKey* key) { return game::hover_drawing_matrix(*this, key, *LinkedTo->GetTechnoType()); }
Matrix3D YRPP_STDCALL HoverLocomotionClass::Shadow_Matrix(VoxelIndexKey* key) {
    if (key && (std::abs(LinkedTo->AngleRotatedSideways) >= 0.005 || std::abs(LinkedTo->AngleRotatedForwards) >= 0.005))
        key->Invalidate();
    return LocomotionClass::Shadow_Matrix(key);
}
