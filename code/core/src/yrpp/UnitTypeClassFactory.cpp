// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unittype.cpp; YR 0x7474B0 / 0x747560.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/ScenarioClass.h"
ObjectClass* UnitTypeClass::CreateObject(HouseClass* owner) {
    return GameCreate<UnitClass>(this,owner);
}
bool UnitTypeClass::SpawnAtMapCoords(CellStruct* cell,HouseClass* owner) {
    if(!cell)return false;
    auto* object=static_cast<UnitClass*>(CreateObject(owner));if(!object)return false;
    if(!object->InitializeLocomotor()){GameDelete(object);return false;}
    const auto facing=static_cast<DirType>(ScenarioClass::Instance->Random.Random()&0xFF);
    if(object->Unlimbo({cell->X*256+128,cell->Y*256+128,0},facing))return true;
    GameDelete(object);return false;
}
