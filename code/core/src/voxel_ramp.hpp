// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 voxel.cpp ramp rotations; YR ramp matrix table 0xB45188.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#pragma once
#include "yrpp/Matrix3D.h"
#include <bit>
#include <cmath>
#include <cstdint>
namespace game {
// Original Get_Slope_Transition_Matrix 0x755A40. The caller handles a
// completed timer by taking VoxelRampMatrix directly, even if ramps differ.
inline Matrix3D voxel_ramp_matrix(int previous,int current,double ratio) {
    if(previous==current)return Matrix3D::VoxelRampMatrix[current];
    float time=float(ratio);
    if(std::abs(double(time))>std::abs(ratio))time=std::bit_cast<float>(std::bit_cast<std::uint32_t>(time)-1u);
    Quaternion q;Quaternion::Slerp(&q,Quaternion::VoxelRampQuaternion[previous],Quaternion::VoxelRampQuaternion[current],time);
    return Matrix3D::FromQuaternion(q);
}
}
