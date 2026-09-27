// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 matrix3d.cpp Rotate_X / Rotate_Y / Rotate_Z; YR 0x5AEF60 / 0x5AF080 / 0x5AF1A0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/Matrix3D.h"
#include "yrpp/YRMath.h"
#include "Matrix3DArithmetic.hpp"
void Matrix3D::RotateX(float theta) noexcept {
    const float sine=matrix_store_float(Math::sin(theta));const double cosine=Math::cos(theta);
    for(int i=0;i<3;++i){const double y=row[i][1],z=row[i][2];
        row[i][1]=matrix_store_float(matrix_add(y*cosine,z*sine));
        row[i][2]=matrix_store_float(matrix_add(z*cosine,-y*sine));}
}
void Matrix3D::RotateZ(float theta) {
    const float cosine=matrix_store_float(Math::cos(theta));const double sine=Math::sin(theta);
    for(int i=0;i<3;++i){const double x=row[i][0],y=row[i][1];
        row[i][0]=matrix_store_float(matrix_add(y*sine,x*cosine));
        row[i][1]=matrix_store_float(matrix_add(y*cosine,-x*sine));}
}

void Matrix3D::RotateY(float theta) noexcept {
    const float sine=matrix_store_float(Math::sin(theta));const double cosine=Math::cos(theta);
    for(int i=0;i<3;++i){const double x=row[i][0],z=row[i][2];
        row[i][0]=matrix_store_float(matrix_add(x*cosine,-z*sine));
        row[i][2]=matrix_store_float(matrix_add(z*cosine,x*sine));}
}
