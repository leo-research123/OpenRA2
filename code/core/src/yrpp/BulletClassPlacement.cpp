// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 bullet.cpp Unlimbo/Shape_Number and fuse.cpp Arm_Fuse;
// YR 0x468670/0x468000/0x4E1130. EA Section 7 terms: third_party/opents/LICENSE.md.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
#include "yrpp/BulletClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"
#include "projectile_diagnostics.hpp"
#include <bit>
#include <climits>

namespace {
int difference(int a,int b){return std::bit_cast<int>(unsigned(a)-unsigned(b));}
void set_speed(BulletVelocity& v,double speed) {
    if(v.X==0.0 && v.Y==0.0 && v.Z==0.0)v.X=100.0;
    const double scale=speed/Math::sqrt(v.Z*v.Z+v.Y*v.Y+v.X*v.X);
    v.X*=scale;v.Y*=scale;v.Z*=scale;
}
void arm_fuse(BulletData& fuse,const CoordStruct& from,const CoordStruct& target,int arm) {
    fuse.UnknownTimer.Start(INT_MAX);fuse.ArmTimer.Start(arm);fuse.Location=target;
    const int x=difference(from.X,target.X),y=difference(from.Y,target.Y),z=difference(from.Z,target.Z);
    fuse.Distance=rule_integer(Math::sqrt(double(x)*x+double(z)*z+double(y)*y));
}
}

bool BulletClass::MoveTo(const CoordStruct& where,const BulletVelocity& velocity) {
    if(!ObjectClass::Unlimbo(where,DirType::North))return false;
    Velocity=velocity;SourceCoords=where;LastMapCoords=CellClass::Coord2Cell(SourceCoords);
    DisplayClass::Remove(this);
    auto target=Target->GetCenterCoords();TargetCoords=target;
    if(Type->FlakScatter && Type->Inviso) {
        // YR stores X/Y and the final length as floats, then truncates before
        // the wrapped integer product/division. The decompiler loses sqrt's
        // x87 return value and the sin/cos calls here.
        const float x=static_cast<float>(difference(where.X,target.X));
        const float y=static_cast<float>(difference(target.Y,where.Y));
        const double z=difference(where.Z,target.Z);
        const float length=static_cast<float>(Math::sqrt(double(x)*x+double(y)*y+z*z));
        auto& random=ScenarioClass::Instance->Random;
        const int jitter=random.RandomRanged(0,std::bit_cast<int>(2u*unsigned(RulesClass::Instance->BallisticScatter)));
        const int radius=std::bit_cast<int>(unsigned(rule_integer(length))*unsigned(jitter))/WeaponType->Range;
        const double angle=double(random.RandomRanged(0,INT_MAX-1))*4.656612877414201e-10*6.283185307179586;
        const auto facing=std::bit_cast<short>(static_cast<unsigned short>(rule_integer((angle-1.570796326794897)*-10430.06004058427)));
        const double radians=(int(facing)-0x3FFF)*-0.00009587672516830327;
        const int yOut=rule_integer(double(target.Y)-Math::sin(radians)*radius);
        target.X=rule_integer(Math::cos(radians)*radius+double(target.X));target.Y=yOut;
    }
    if(Type->Inviso) {
        auto* house=Owner?Owner->Owner:nullptr;
        auto blocked=MapClass::Instance.FindFirstFirestorm(where,target,house);
        if(blocked==CoordStruct::Empty) {
            auto* obstacle=TrajectoryHelper::FindFirstObstacle(where,target,Type,house);
            SetLocation(obstacle?obstacle->GetCoords():target);
            // Both writes are present in 0x468963..0x468986, including when
            // an obstacle was found. Do not silently 'fix' the original.
            SetLocation(target);Speed=0;set_speed(Velocity,0.0);
        }else {
            blocked.Z=MapClass::Instance.GetCellFloorHeight(blocked);SetLocation(blocked);
        }
    }
    arm_fuse(Data,Location,target,Target && Target->WhatAmI()==AbstractType::Aircraft?0:Type->Arm);
    if(Type->ROT>0)set_speed(Velocity,1.0);
    if(IsAlive)DisplayClass::Submit(this);
    game::projectile_log_event(*this,"launched");
    return true;
}

BYTE BulletClass::GetAnimFrame() const {
    constexpr BYTE facingFrames[]{28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,31,30,29};
    BYTE frame=0;
    if(!Type->NoRotate) {
        const auto facing=static_cast<unsigned short>(rule_integer((Math::atan2(-Velocity.Y,Velocity.X)-1.570796326794897)*-10430.06004058427));
        frame=facingFrames[(((facing>>10)+1)>>1)&0x1F];
    }
    return Type->AnimLow || Type->AnimHigh?AnimFrame:frame;
}
