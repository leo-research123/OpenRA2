// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 objecttype.cpp Who_Can_Build_Me; YR 0x5F7900.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/HouseClass.h"
BuildingClass* ObjectTypeClass::FindFactory(bool allowOccupied,bool requirePower,bool requireCanBuild,HouseClass const* owner) const {
 if(!owner)return nullptr;
 BuildingClass* result=nullptr;
 for(auto* building:owner->Buildings){
  if(building->InLimbo||building->Type->Factory!=WhatAmI()||(requirePower&&!building->HasPower)||
    building->GetCurrentMission()==Mission::Selling||building->QueuedMission==Mission::Selling||
    (requireCanBuild&&owner->CanBuild(static_cast<const TechnoTypeClass*>(this),true,true)<=CanBuildResult::Unbuildable)||
    !(building->Type->GetOwners()&GetOwners()))continue;
  const auto* type=static_cast<const TechnoTypeClass*>(this);
  if(building->Type->Naval!=(WhatAmI()==AbstractType::UnitType&&type->Naval))continue;
  if(!allowOccupied&&WhatAmI()==AbstractType::AircraftType&&building->HasAnyLink()&&!building->HasFreeLink())continue;
  if(building->IsPrimaryFactory)return building;
  result=building;
 }
 return result;
}
