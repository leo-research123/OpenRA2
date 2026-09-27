// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 quat.cpp Axis_To_Quat / Slerp; YR 0x646480 / 0x646590.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/Matrix3D.h"
#include "yrpp/YRMath.h"
#include "Matrix3DArithmetic.hpp"

Quaternion* YRPP_FASTCALL Quaternion::FromAxis(Quaternion* output,const Vector3D<float>& axis,float angle) noexcept {
    const double length=Math::sqrt(matrix_add(matrix_add(double(axis.Z)*axis.Z,double(axis.Y)*axis.Y),double(axis.X)*axis.X));
    const float x=length?matrix_store_float(matrix_divide(axis.X,length)):axis.X;
    const float y=length?matrix_store_float(matrix_divide(axis.Y,length)):axis.Y;
    const float z=length?matrix_store_float(matrix_divide(axis.Z,length)):axis.Z;
    const double sine=Math::sin(double(angle)*0.5);
    output->X=matrix_store_float(x*sine);output->Y=matrix_store_float(y*sine);output->Z=matrix_store_float(z*sine);
    output->W=matrix_store_float(Math::cos(double(angle)*0.5));return output;
}

Quaternion* YRPP_FASTCALL Quaternion::Slerp(Quaternion* output,const Quaternion& a,const Quaternion& b,float t) noexcept {
    const double dot=matrix_add(matrix_add(matrix_add(double(a.W)*b.W,double(a.Z)*b.Z),double(a.Y)*b.Y),double(a.X)*b.X);
    double left,right;
    if(matrix_add(dot,1.0)<=0.00001){
        constexpr double half_pi=1.570796326794895;
        left=Math::sin(matrix_multiply(matrix_add(1.0,-double(t)),half_pi));
        right=Math::sin(matrix_multiply(t,half_pi));
    }else if(matrix_add(1.0,-dot)<=0.00001){left=matrix_add(1.0,-double(t));right=t;}
    else {
        // Original acos 0x4CADB0 is pi/2 minus the quantized asin table.
        const double angle=matrix_add(Math::HalfPi,-Math::asin(dot)),sine=Math::sin(angle);
        left=matrix_divide(Math::sin(matrix_multiply(matrix_add(1.0,-double(t)),angle)),sine);
        right=matrix_divide(Math::sin(matrix_multiply(t,angle)),sine);
    }
    Quaternion result;
    for(int i=0;i<4;++i)result[i]=matrix_store_float(matrix_add(matrix_multiply(b[i],right),matrix_multiply(a[i],left)));
    *output=result;return output;
}
