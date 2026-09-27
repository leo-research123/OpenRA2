// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techtype.cpp; calibrated to YR 0x48DCD0 / 0x712040.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
TechnoTypeClass* YRPP_FASTCALL TechnoTypeClass::GetByTypeAndIndex(AbstractType kind,int index){
 switch(kind){
 case AbstractType::Infantry:case AbstractType::InfantryType:return InfantryTypeClass::Array.GetItemOrDefault(index);
 case AbstractType::Unit:case AbstractType::UnitType:return UnitTypeClass::Array.GetItemOrDefault(index);
 case AbstractType::Aircraft:case AbstractType::AircraftType:return AircraftTypeClass::Array.GetItemOrDefault(index);
 case AbstractType::Building:case AbstractType::BuildingType:return BuildingTypeClass::Array.GetItemOrDefault(index);
 default:return nullptr;
 }
}
SHPStruct* TechnoTypeClass::GetCameo() const {
 const auto* owner=HouseClass::CurrentPlayer;
 if(owner&&Trainable&&AltCameo&&((WhatAmI()==AbstractType::InfantryType&&owner->BarracksInfiltrated)||
    (WhatAmI()==AbstractType::UnitType&&owner->WarFactoryInfiltrated)))return AltCameo;
 return Cameo;
}
