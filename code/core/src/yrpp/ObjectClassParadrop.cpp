// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp Paradrop; YR 0x5F5940 landing-cell checks.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MapClass.h"

bool ObjectClass::SpawnParachuted(const CoordStruct& at) {
    auto& map=MapClass::Instance;
    if(!map.IsWithinUsableArea(at))return false;
    IsFallingDown=true;
    auto* cell=map.TryGetCellAt(at);if(!cell)return false;
    const auto flags=static_cast<unsigned>(cell->Flags);
    if(flags&0x100u){OnBridge=true;if(!(flags&0x200u))return false;}
    if((AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None) {
        const auto* type=static_cast<TechnoClass*>(this)->GetTechnoType();
        const int zone=map.GetMovementZoneType(cell->MapCoords,type->MovementZone,OnBridge);
        if(!cell->IsClearToMove(type->SpeedType,false,false,zone,type->MovementZone,-1,true))return false;
    }
    if(!Unlimbo(at,DirType::South))return false;
    SetLocation(at);
    const bool bomb=WhatAmI()==AbstractType::Bullet;
    auto location=at;if(!bomb)location.Z+=75;
    auto* animation=GameCreate<AnimClass>(bomb?RulesClass::Instance->BombParachute:RulesClass::Instance->Parachute,
        location,0,1,0x600,0,false);
    if(!bomb)Parachute=animation;
    if(animation){
        animation->SetOwnerObject(this);
        if(!bomb){animation->LightConvert=GetRemapColour();animation->TintColor=static_cast<short>(cell->Intensity_Normal);}
    }
    return true;
}
