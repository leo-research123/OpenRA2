// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Rocking_AI; YR 0x70B570 adds sinking direction,
// tilt-crash limits, reciprocal rocker acceleration and overturn damage.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/RulesClass.h"
#include <algorithm>
#include <cmath>

void TechnoClass::vt_entry_41C() {
    if(IsSinking) {
        if(std::abs(AngleRotatedForwards)<double(0.78539819f)) {
            const auto face=PrimaryFacing.Current().GetValue<3>();
            AngleRotatedForwards=float(AngleRotatedForwards+(face&&face<=5?0.01:-0.01));
        }
        return;
    }
    if(IsCrashing) {
        AngleRotatedForwards+=RockingForwardsPerFrame;AngleRotatedSideways+=RockingSidewaysPerFrame;
        if(GetTechnoType()->TiltCrashJumpjet) {
            AngleRotatedForwards=std::max(AngleRotatedForwards,-0.78539819f);
            AngleRotatedSideways=std::clamp(AngleRotatedSideways,-0.78539819f,0.78539819f);
        }
        return;
    }
    const auto rock=[&](float& angle,float& velocity,float limit,bool sideways) {
        const bool positive=angle>0.00002,negative=angle< -0.00002;
        const bool overturned=angle>1.5707964f||angle< -1.5707964f;
        if(velocity==0)angle=0;
        else {
            const float old=angle;angle+=velocity;
            if(!DirectRockerLinkedUnit&&!overturned) {
                if(angle>limit&&old<limit){angle=limit;velocity=0;}
                if(angle< -limit&&old> -limit){angle=-limit;velocity=0;}
            }
            if(overturned)velocity+=positive?0.002f:-0.002f;
            else if(DirectRockerLinkedUnit)velocity+=(positive?-1:1)*RulesClass::Instance->DirectRockingCoefficient*0.002f;
            else {
                const double amount=sideways&&((positive&&velocity>0)||(!positive&&velocity<0))?double(0.002f):double(0.005f);
                velocity=float(velocity+(positive?-amount:amount));
            }
        }
        if((positive&&angle<0.00002)||(negative&&angle> -0.00002)){velocity=0;angle=0;}
    };
    rock(AngleRotatedSideways,RockingSidewaysPerFrame,0.78539819f,true);
    const bool crush=(AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None&&static_cast<FootClass*>(this)->IsCrushingSomething;
    rock(AngleRotatedForwards,RockingForwardsPerFrame,crush?0.31415927f:0.78539819f,false);
    if(std::abs(AngleRotatedSideways)>double(3.1415927f)||std::abs(AngleRotatedForwards)>double(3.1415927f)) {
        int damage=GetTechnoType()->Strength;ReceiveDamage(&damage,0,RulesClass::Instance->C4Warhead,nullptr,true,false,nullptr);
    }
}
