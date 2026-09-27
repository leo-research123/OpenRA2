// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 voxel.cpp Init_Voxel_Matrices / SlopeMatrices and
// matrix3d.cpp terrain-ramp constructor. YR 0x754CB0 / 0x5AE6F0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/Matrix3D.h"
#include "yrpp/YRMath.h"

Matrix3D::Matrix3D(float rotate_z,float rotate_x) noexcept {
    MakeIdentity();RotateZ(rotate_z);RotateX(rotate_x);RotateZ(-rotate_z);
}

namespace {
// 0x754A20 / 0x754A50 use the original quantized atan table. For these
// positive arguments atan2 selects the same entries; host std::atan does not.
const float slope_angle=float(Math::atan2(104.0,256.0));
const float diagonal_angle=float(Math::atan2(208.0,Math::sqrt(2.0*256.0*256.0)));
constexpr double pi=3.1415;
// Preserve the original swapped slope angles, repeated corner ramps and
// identity entries 17..20. Storage belongs to the existing Matrix3D table.
Matrix3D ramp_matrices[21]{
    Matrix3D::GetIdentity(),
    {float(3*pi/2),diagonal_angle},{float(pi),diagonal_angle},
    {float(pi/2),diagonal_angle},{0.0f,diagonal_angle},
    {float(5*pi/4),slope_angle},{float(3*pi/4),slope_angle},
    {float(pi/4),slope_angle},{float(7*pi/4),slope_angle},
    {float(5*pi/4),slope_angle},{float(3*pi/4),slope_angle},
    {float(pi/4),slope_angle},{float(7*pi/4),slope_angle},
    {float(5*pi/4),diagonal_angle},{float(3*pi/4),diagonal_angle},
    {float(pi/4),diagonal_angle},{float(7*pi/4),diagonal_angle},
    Matrix3D::GetIdentity(),Matrix3D::GetIdentity(),
    Matrix3D::GetIdentity(),Matrix3D::GetIdentity()
};
Quaternion ramp_quaternion(Vector3D<float> axis,float angle) noexcept {
    Quaternion result;Quaternion::FromAxis(&result,axis,angle);return result;
}
Vector3D<float> diagonal_axis(double heading) noexcept {
    const float angle=float(heading);
    return {float(Math::cos(angle)),float(Math::sin(angle)),0};
}
// Original Init_Voxel_Matrices uses exact cardinal axes for ramps 1..4,
// and normalized, table-rotated axes for the diagonal groups.
Quaternion ramp_quaternions[21]{
    {},ramp_quaternion({0,-1,0},diagonal_angle),ramp_quaternion({-1,0,0},diagonal_angle),
    ramp_quaternion({0,1,0},diagonal_angle),ramp_quaternion({1,0,0},diagonal_angle),
    ramp_quaternion(diagonal_axis(5*pi/4),slope_angle),ramp_quaternion(diagonal_axis(3*pi/4),slope_angle),
    ramp_quaternion(diagonal_axis(pi/4),slope_angle),ramp_quaternion(diagonal_axis(7*pi/4),slope_angle),
    ramp_quaternion(diagonal_axis(5*pi/4),slope_angle),ramp_quaternion(diagonal_axis(3*pi/4),slope_angle),
    ramp_quaternion(diagonal_axis(pi/4),slope_angle),ramp_quaternion(diagonal_axis(7*pi/4),slope_angle),
    ramp_quaternion(diagonal_axis(5*pi/4),diagonal_angle),ramp_quaternion(diagonal_axis(3*pi/4),diagonal_angle),
    ramp_quaternion(diagonal_axis(pi/4),diagonal_angle),ramp_quaternion(diagonal_axis(7*pi/4),diagonal_angle),
    {},{},{},{}
};
}
Matrix3D (&Matrix3D::VoxelRampMatrix)[21]=ramp_matrices;
Quaternion (&Quaternion::VoxelRampQuaternion)[21]=ramp_quaternions;
