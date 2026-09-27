// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Fire_Coord/Turret_Coord/Predict_Target_Coord;
// YR 0x6F3AD0/0x6F3D60/0x70BCB0. EA terms: third_party/opents/LICENSE.md.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "RulesClassReaders.hpp"
#include <bit>
#include <cstdlib>

namespace {
double angle32(DirStruct dir){return (int(dir.GetValue<5>())-8)*-0.1963495408493621;}
void translate_fire(Matrix3D& matrix,float x,float y,float z) {
    // 0x5AE890 keeps each row's sum in x87 until its final float store.
    // Three TranslateX/Y/Z calls introduce extra rounding at each axis.
    matrix.row[0][3]=float(((double(y)*matrix.row[0][1]+double(z)*matrix.row[0][2])
        +double(x)*matrix.row[0][0])+matrix.row[0][3]);
    for(int row=1;row<3;++row)
        matrix.row[row][3]=float(((double(x)*matrix.row[row][0]+double(y)*matrix.row[row][1])
            +double(z)*matrix.row[row][2])+matrix.row[row][3]);
}
Matrix3D fire_matrix(const TechnoClass& actor) {
    auto matrix=Matrix3D::GetIdentity();
    double angle=angle32(actor.TurretFacing());
    if((actor.AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None) {
        const auto& loco=static_cast<const FootClass&>(actor).Locomotor;
        if(loco){matrix=loco->Draw_Matrix(nullptr);angle-=angle32(actor.PrimaryFacing.Current());}
    }
    translate_fire(matrix,float(actor.GetTechnoType()->TurretOffset),0.0f,0.0f);
    matrix.RotateZ(float(angle));return matrix;
}
CoordStruct* output_coords(const TechnoClass& actor,const Matrix3D& matrix,CoordStruct* output) {
    const auto offset=matrix*Vector3D<float>{0,0,0};
    const int x=rule_integer(offset.X),y=rule_integer(-double(offset.Y)),z=rule_integer(offset.Z);
    const auto base=actor.GetRenderCoords();
    *output={std::bit_cast<int>(unsigned(base.X)+unsigned(x)),std::bit_cast<int>(unsigned(base.Y)+unsigned(y)),
        std::bit_cast<int>(unsigned(base.Z)+unsigned(z))};return output;
}
}
CoordStruct* TechnoClass::GetFLH(CoordStruct* output,int weapon,CoordStruct base) const {
    CoordStruct offset{};
    if(weapon>=0)offset=GetWeapon(weapon)->FLH;
    else if(weapon>=-5)offset=GetTechnoType()->AlternativeFLH[-weapon-1];
    auto matrix=fire_matrix(*this);
    const int x=std::bit_cast<int>(unsigned(base.X)+unsigned(offset.X));
    const int y=std::bit_cast<int>(unsigned(base.Y)+unsigned(offset.Y));
    const int z=std::bit_cast<int>(unsigned(base.Z)+unsigned(offset.Z));
    const int lateral=CurrentBurstIndex%2?std::bit_cast<int>(0u-unsigned(y)):y;
    translate_fire(matrix,float(x),float(lateral),float(z));return output_coords(*this,matrix,output);
}
CoordStruct* TechnoClass::vt_entry_300(CoordStruct* output,DWORD) const {
    return output_coords(*this,fire_matrix(*this),output);
}
CoordStruct* TechnoClass::PredictTargetCoords(CoordStruct* output) const {
    *output=Target?Target->GetCenterCoords():CoordStruct::Empty;
    if(!Target || Target->WhatAmI()!=AbstractType::Unit)return output;
    auto* unit=static_cast<UnitClass*>(Target);
    if(!unit->Locomotor)std::abort();
    if(!unit->Locomotor->Is_Moving())return output;
    const int speed=unit->GetCurrentSpeed(),distance=DistanceFrom3D(Target);
    auto* weapon=GetTurretWeapon();
    if(!weapon || !weapon->WeaponType)return output;
    const int travel=rule_integer(double(distance)/(double(weapon->WeaponType->GetSpeed(distance))*0.9)*speed);
    const double angle=(std::bit_cast<short>(unit->PrimaryFacing.Current().Raw)-0x3FFF)*-0.00009587672516830327;
    output->Y=rule_integer(double(output->Y)-Math::sin(angle)*travel);
    output->X=rule_integer(Math::cos(angle)*travel+double(output->X));return output;
}
