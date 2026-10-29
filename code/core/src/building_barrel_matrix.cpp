// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 building.cpp Get_Barrel_Matrix; YR 0x00458810.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA terms: third_party/opents/LICENSE.md. Keep the float-store boundaries.
#include "building_barrel_matrix.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/Matrix3DArithmetic.hpp"

namespace {
void translate_barrel(Matrix3D& matrix, const CoordStruct& offset) {
    // Original Translate_X/Y/Z store each axis separately under x87 0x0E7F.
    const float axes[]{matrix_store_float(offset.X), matrix_store_float(offset.Y), matrix_store_float(offset.Z)};
    for (int axis = 0; axis < 3; ++axis)
        for (auto& row : matrix.row)
            row[3] = matrix_store_float(matrix_add(double(axes[axis]) * row[axis], row[3]));
}
float barrel_angle(const FacingClass& facing) {
    return matrix_store_float((int(facing.Current().GetValue<5>()) - 8) * -0.1963495408493621);
}
}

namespace game {
Matrix3D* building_barrel_offset_matrix(const BuildingClass& building, Matrix3D& output) {
    output.MakeIdentity();
    translate_barrel(output, building.Type->VoxelBarrelOffsetToBuildingPivotPoint);
    output.RotateZ(barrel_angle(building.PrimaryFacing));
    translate_barrel(output, building.Type->VoxelBarrelOffsetToRotatePivotPoint);
    output.RotateY(-barrel_angle(building.BarrelFacing));
    translate_barrel(output, building.Type->VoxelBarrelOffsetToPitchPivotPoint);
    const float scale = matrix_store_float(building.Type->VoxelBarrelScale);
    for (auto& row : output.row)
        for (int column = 0; column < 3; ++column)
            row[column] = matrix_store_float(double(row[column]) * scale);
    return &output;
}
}
