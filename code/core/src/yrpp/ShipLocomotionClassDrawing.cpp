// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 drive.cpp / voxel.cpp scaffolding, calibrated to Ship YR
// 0x69F0C0 / 0x69F1B0 / 0x69F5E0 / 0x69F670 / 0x69FB20.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ShipLocomotionClass.h"
#include "locomotor_drawing.hpp"
#include "yrpp/FootClass.h"
#include "voxel_ramp.hpp"
#include "x87_integer.hpp"
#include "Matrix3DArithmetic.hpp"
#include <bit>
#include <cmath>

double ShipLocomotionClass::Slope_Ratio() {
    const int rate = SlopeTimer.Rate;
    return rate ? matrix_divide(rate - SlopeTimer.GetTimeLeft(), rate) : 1.0;
}

Matrix3D ShipLocomotionClass::Slope_Matrix() {
    if (Slope_Ratio() < 1.0) return game::voxel_ramp_matrix(PreviousRamp, CurrentRamp, Slope_Ratio());
    return CurrentRamp ? Matrix3D::VoxelRampMatrix[CurrentRamp] : Matrix3D::GetIdentity();
}

BOOL ShipLocomotionClass::Is_Stable() {
    return Slope_Ratio() == 1.0 && std::abs(LinkedTo->AngleRotatedSideways) < 0.005 &&
           std::abs(LinkedTo->AngleRotatedForwards) < 0.005;
}

Matrix3D game::ship_drawing_matrix(ShipLocomotionClass& self, VoxelIndexKey* key, const TechnoTypeClass& type) {
    if (self.Is_Stable()) {
        if (key && key->Is_Valid_Key())
            key->Value = std::bit_cast<int>((unsigned(key->Value) << 6) + unsigned(self.CurrentRamp));
        const auto facing = self.LocomotionClass::Draw_Matrix(key);
        return drawing_matrix_product(self.Slope_Matrix(), facing);
    }
    auto vertical = Matrix3D::GetIdentity(), tilt = Matrix3D::GetIdentity();
    const float y = matrix_store_float(type.VoxelScaleX);
    const float x = matrix_store_float(type.VoxelScaleY);
    const float frontCos = matrix_store_float(Math::cos(self.LinkedTo->AngleRotatedForwards));
    const float frontSin = matrix_store_float(Math::sin(self.LinkedTo->AngleRotatedForwards));
    const float sideCos = matrix_store_float(Math::cos(self.LinkedTo->AngleRotatedSideways));
    const double sideSin = Math::sin(self.LinkedTo->AngleRotatedSideways);
    const int height = game::x87_integer(std::abs(sideSin) * y + std::abs(double(frontSin)) * x);
    int forward = game::x87_integer(double(x) - game::x87_integer(double(frontCos) * x));
    int sideways = game::x87_integer(double(y) - game::x87_integer(double(sideCos) * y));
    if (self.LinkedTo->AngleRotatedForwards < 0) forward = std::bit_cast<int>(0u - unsigned(forward));
    if (self.LinkedTo->AngleRotatedSideways > 0) sideways = std::bit_cast<int>(0u - unsigned(sideways));
    vertical.TranslateZ(float(height));
    tilt.TranslateX(float(forward));
    tilt.TranslateY(float(sideways));
    tilt.RotateX(self.LinkedTo->AngleRotatedSideways);
    tilt.RotateY(self.LinkedTo->AngleRotatedForwards);
    if (key) key->Invalidate();
    const auto facing = self.LocomotionClass::Draw_Matrix(key);
    const auto slope = self.Slope_Matrix();
    return drawing_matrix_product(drawing_matrix_product(drawing_matrix_product(vertical, slope), facing), tilt);
}

Matrix3D YRPP_STDCALL ShipLocomotionClass::Draw_Matrix(VoxelIndexKey* key) { return game::ship_drawing_matrix(*this, key, *LinkedTo->GetTechnoType()); }
Matrix3D YRPP_STDCALL ShipLocomotionClass::Shadow_Matrix(VoxelIndexKey* key) {
    if ((Slope_Ratio() != 1.0 || std::abs(LinkedTo->AngleRotatedSideways) >= 0.005 ||
         std::abs(LinkedTo->AngleRotatedForwards) >= 0.005) &&
        key)
        key->Invalidate();
    return LocomotionClass::Shadow_Matrix(key);
}
