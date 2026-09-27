// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp::AI, calibrated to YR 0x5F3E70.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include <algorithm>
#include <bit>
#include <cmath>

// OpenTS In_Which_Layer; YR adds the low-air layer and map-presence gate.
bool ObjectClass::IsInAir() const { return IsOnMap && GetHeight()>=2*Unsorted::LevelHeight; }
bool ObjectClass::IsOnFloor() const { return IsOnMap && GetHeight()<2*Unsorted::LevelHeight; }
Layer ObjectClass::InWhichLayer() const {
    if(!IsInAir())return Layer::Ground;
    return GetHeight()>=RulesClass::Instance->CruiseHeight?Layer::Top:Layer::Air;
}

void ObjectClass::Update() {
    if(!InLimbo) {
        if(GetType() && GetType()->AmbientSound!=-1) {
            const auto at=Location;
            VocClass::PlayAt(GetType()->AmbientSound,at,&AmbientSoundController);
        }
        if(CustomSound!=-1){const auto at=Location;VocClass::PlayAt(CustomSound,at,&CustomSoundController);}
    }
    if(!IsFallingDown)return;
    const auto previousLayer=InWhichLayer();
    const int z=std::bit_cast<int>(static_cast<unsigned>(FallRate)+static_cast<unsigned>(GetZ()));
    if(IsOnMap){Mark(MarkType::Up);Location.Z=z;Mark(MarkType::Down);}
    else Location.Z=z;
    if(GetHeight()<=0) {
        SetHeight(0);IsFallingDown=false;UpdatePosition(PCPType::End);
        if(Parachute)Parachute->RemainingIterations=0;
    }
    // Arrival can put the object into limbo. This branch still removes its layer.
    if(InLimbo){DisplayClass::Remove(this);return;}
    if(HasParachute) {
        FallRate=std::bit_cast<int>(static_cast<unsigned>(FallRate)-1u);
        FallRate=std::max(FallRate,RulesClass::Instance->ParachuteMaxFallRate);
    } else {
        const double rate=double(FallRate)-1.4;
        FallRate=rate>=-2147483648.0 && rate<2147483648.0?int(rate):INT32_MIN;
        FallRate=std::max(FallRate,RulesClass::Instance->NoParachuteMaxFallRate);
    }
    if(previousLayer!=InWhichLayer())DisplayClass::Submit(this);
    if(IsFallingDown)return;
    if(IsABomb && Health>0) {
        int damage=Health;
        ReceiveDamage(&damage,0,RulesClass::Instance->C4Warhead,nullptr,true,true,nullptr);
    }
    if(WhatAmI()==AbstractType::Anim) {
        auto* anim=static_cast<AnimClass*>(this);
        if(anim->Type->IsFlamingGuy) {
            anim->FlamingGuyExpire=true;anim->Animation.Start(1);
            anim->Animation.Value=std::bit_cast<int>(8u*static_cast<unsigned>(anim->Type->RunningFrames)+1u);
            const auto at=Location;
            if(MapClass::Instance.GetCellAt(at)->LandType==LandType::Water) {
                // Preserve allocation before re-reading the impact coordinates.
                if(void* storage=YRMemory::Allocate(sizeof(AnimClass))) {
                    const CoordStruct splash{Location.X,Location.Y,std::bit_cast<int>(static_cast<unsigned>(Location.Z)+3u)};
                    ::new(storage) AnimClass(RulesClass::Instance->SplashList[0],splash,0,1,0x600,0,false);
                }
            }
        }
    }
}
