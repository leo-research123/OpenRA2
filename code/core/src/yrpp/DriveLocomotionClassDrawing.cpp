// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 drive.cpp / voxel.cpp; YR 0x4AFF60 / 0x4B0410.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/MapClass.h"
#include <cmath>
#include <cstring>
#include "voxel_ramp.hpp"

namespace {
void set_key(VoxelIndexKey* key,int ramp,bool cacheable) {
    if(!key)return;unsigned raw;std::memcpy(&raw,key,4);
    if(!cacheable)raw=0xFFFFFFFFu;else if(raw!=0xFFFFFFFFu)raw=(raw<<6)+unsigned(ramp);
    std::memcpy(key,&raw,4);
}
}
Matrix3D YRPP_STDCALL DriveLocomotionClass::Draw_Matrix(VoxelIndexKey* key) {
    const double ratio=SlopeTimer.GetRatePassed();
    const auto slope=ratio==1.0?Matrix3D::VoxelRampMatrix[CurrentRamp]:game::voxel_ramp_matrix(PreviousRamp,CurrentRamp,ratio);
    const float side=LinkedTo->AngleRotatedSideways,front=LinkedTo->AngleRotatedForwards;
    if(ratio==1.0&&std::abs(side)<0.005&&std::abs(front)<0.005) {
        set_key(key,CurrentRamp,true);return slope*LocomotionClass::Draw_Matrix(key);
    }
    set_key(key,0,false);
    const float y=float(LinkedTo->GetTechnoType()->VoxelScaleX),x=float(LinkedTo->GetTechnoType()->VoxelScaleY);
    const float fs=float(Math::sin(front)),fc=float(Math::cos(front)),ss=float(Math::sin(side)),sc=float(Math::cos(side));
    auto vertical=Matrix3D::GetIdentity(),tilt=Matrix3D::GetIdentity();
    vertical.TranslateZ(float(int(std::abs(ss)*y+std::abs(fs)*x)));
    tilt.TranslateX(float(int(x-float(int(fc*x)))*(front<0?-1:1)));
    tilt.TranslateY(float(int(y-float(int(sc*y)))*(side>0?-1:1)));
    tilt*=Matrix3D{1,0,0,0,0,sc,-ss,0,0,ss,sc,0};
    tilt*=Matrix3D{fc,0,fs,0,0,1,0,0,-fs,0,fc,0};
    return vertical*slope*LocomotionClass::Draw_Matrix(key)*tilt;
}
Matrix3D YRPP_STDCALL DriveLocomotionClass::Shadow_Matrix(VoxelIndexKey* key) {
    const bool cacheable=SlopeTimer.GetRatePassed()==1.0&&std::abs(LinkedTo->AngleRotatedSideways)<0.005&&std::abs(LinkedTo->AngleRotatedForwards)<0.005;
    if(!cacheable&&key)key->Invalidate();
    return LocomotionClass::Shadow_Matrix(key);
}
