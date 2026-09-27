// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Can_Build; YR 0x4F7870 ground production branches.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
CanBuildResult HouseClass::CanBuild(const TechnoTypeClass* type,bool limitOnly,bool allowInProduction) const {
 if(!type)return CanBuildResult::Unbuildable;
 if(!limitOnly){
  bool override=false;
  for(int index:type->PrerequisiteOverride)if(index>=0&&OwnedBuildingTypes.GetItemCount(index)>0){override=true;break;}
  if(!override){
   const DWORD country=DWORD(1)<<(Type->GetArrayIndex()&31);
   if(type->TechLevel<0||type->TechLevel>TechLevel||!HasAllStolenTech(type)||!type->InRequiredHouses(country)||type->InForbiddenHouses(country))return CanBuildResult::Unbuildable;
   for(int index:type->Prerequisite){
    bool satisfied=false;
    if(index>=0)satisfied=OwnedBuildingTypes.GetItemCount(index)>0;
    else{
     const auto& rules=*RulesClass::Instance;
     const TypeList<int>* groups[]{&rules.PrerequisitePower,&rules.PrerequisiteFactory,&rules.PrerequisiteBarracks,&rules.PrerequisiteRadar,&rules.PrerequisiteTech,&rules.PrerequisiteProc};
     if(index>=-6)for(int item:*groups[-index-1])if(OwnedBuildingTypes.GetItemCount(item)>0){satisfied=true;break;}
     if(index==-6&&rules.PrerequisiteProcAlternate&&OwnedUnitTypes.GetItemCount(rules.PrerequisiteProcAlternate->GetArrayIndex())>0)satisfied=true;
    }
    if(!satisfied)return CanBuildResult::Unbuildable;
   }
  }
 }
 const int index=type->GetArrayIndex();int active=0,produced=0;
 switch(type->WhatAmI()){
 case AbstractType::InfantryType:active=ActiveInfantryTypes.GetItemCount(index);produced=FactoryProducedInfantryTypes.GetItemCount(index);break;
 case AbstractType::UnitType:active=ActiveUnitTypes.GetItemCount(index);produced=FactoryProducedUnitTypes.GetItemCount(index);break;
 case AbstractType::AircraftType:active=ActiveAircraftTypes.GetItemCount(index);produced=FactoryProducedAircraftTypes.GetItemCount(index);break;
 case AbstractType::BuildingType:active=ActiveBuildingTypes.GetItemCount(index);produced=FactoryProducedBuildingTypes.GetItemCount(index);break;
 default:return CanBuildResult::Unbuildable;
 }
 if(type->BuildLimit<=0)return static_cast<CanBuildResult>(produced<-static_cast<long long>(type->BuildLimit)?1:0);
 if(active<type->BuildLimit)return static_cast<CanBuildResult>(1);
 if(allowInProduction&&(type->WhatAmI()!=AbstractType::InfantryType||active==type->BuildLimit))
  for(const auto* factory:FactoryClass::Array)if(factory->Owner==this&&factory->Object&&factory->Object->GetTechnoType()==type)return CanBuildResult::Buildable;
 return CanBuildResult::TemporarilyUnbuildable;
}
