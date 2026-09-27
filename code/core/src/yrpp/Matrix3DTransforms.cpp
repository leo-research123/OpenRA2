// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 quat.cpp Build_Matrix3D / matrix3d.cpp;
// YR 0x646980 / 0x5AFC20 / 0x5AF4D0 arithmetic and aliasing.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/Matrix3D.h"
#include "Matrix3DArithmetic.hpp"

Matrix3D* YRPP_FASTCALL Matrix3D::FromQuaternion(Matrix3D* output,const Quaternion* q) noexcept {
    const double zz=double(q->Z)*q->Z,xy=double(q->Y)*q->X,wz=double(q->W)*q->Z,xz=double(q->X)*q->Z;
    const float yy=matrix_store_float(double(q->Y)*q->Y),wy=matrix_store_float(double(q->W)*q->Y);
    const float saved_xz=matrix_store_float(xz),xx=matrix_store_float(double(q->X)*q->X);
    const double yz=double(q->Y)*q->Z,wx=double(q->W)*q->X;
    // Preserve compiler spills and operation order; algebraic simplification
    // changes original float results even for the same quaternion.
    const auto twice=[](double a,double b){const auto sum=matrix_add(a,b);return matrix_add(sum,sum);};
    Matrix3D result{
        matrix_store_float(matrix_add(1,-twice(yy,zz))),matrix_store_float(twice(xy,-wz)),matrix_store_float(twice(xz,wy)),0,
        matrix_store_float(twice(wz,xy)),matrix_store_float(matrix_add(1,-twice(xx,zz))),matrix_store_float(twice(yz,-wx)),0,
        matrix_store_float(twice(saved_xz,-wy)),matrix_store_float(twice(wx,yz)),matrix_store_float(matrix_add(1,-twice(xx,yy))),0};
    *output=result;return output;
}

Matrix3D* YRPP_FASTCALL Matrix3D::TransposeMatrix(Matrix3D* output,const Matrix3D* input) noexcept {
    Matrix3D result;
    for(int i=0;i<3;++i){
        for(int j=0;j<3;++j)result.row[i][j]=input->row[j][i];
        result.row[i][3]=matrix_store_float(-matrix_add(matrix_add(double(result.row[i][0])*input->row[0][3],
            double(result.row[i][2])*input->row[2][3]),double(result.row[i][1])*input->row[1][3]));
    }
    *output=result;return output;
}

Vector3D<float>* Matrix3D::__RotateVector(Vector3D<float>* output,const Vector3D<float>* input) const noexcept {
    Vector3D<float> result;
    result.X=matrix_store_float(matrix_add(matrix_add(double(row[0][1])*input->Y,double(row[0][2])*input->Z),double(row[0][0])*input->X));
    result.Y=matrix_store_float(matrix_add(matrix_add(double(row[1][1])*input->Y,double(row[1][0])*input->X),double(row[1][2])*input->Z));
    result.Z=matrix_store_float(matrix_add(matrix_add(double(row[2][1])*input->Y,double(row[2][0])*input->X),double(row[2][2])*input->Z));
    *output=result;return output;
}
