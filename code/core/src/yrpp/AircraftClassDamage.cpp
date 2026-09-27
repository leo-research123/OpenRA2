// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 aircraft.cpp Take_Damage; YR 0x4165C0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/AircraftClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/ScenarioClass.h"

DamageState AircraftClass::ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) {
    const auto result=FootClass::ReceiveDamage(damage,distance,warhead,source,ignoreDefenses,preventEscape,sourceHouse);
    if(result!=DamageState::NowDead)return result;
    Destroyed(nullptr);
    if(Type->Explosion.Count>0) {
        auto* animation=Type->Explosion[unsigned(ScenarioClass::Instance->Random.Random())%unsigned(Type->Explosion.Count)];
        if(auto* storage=YRMemory::Allocate(sizeof(AnimClass)))
            ::new(storage) AnimClass(animation,GetTargetCoords(),0,1,0x600,0,false);
    }
    if(!Crash(source))UnInit();
    return result;
}
