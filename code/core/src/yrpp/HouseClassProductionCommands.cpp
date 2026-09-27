// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Begin/Suspend/Abandon_Production, Place_Object.
// YR 0x4FA350 / 0x4FA910 / 0x4FAA10 / 0x4FB0E0 / 0x4FB6B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "type_resources.hpp"

void HouseClass::SetPrimaryFactory(FactoryClass* factory,AbstractType type,bool naval,BuildCat category){
    switch(type){
    case AbstractType::Infantry:case AbstractType::InfantryType:Primary_ForInfantry=factory;break;
    case AbstractType::Unit:case AbstractType::UnitType:(naval?Primary_ForShips:Primary_ForVehicles)=factory;break;
    case AbstractType::Aircraft:case AbstractType::AircraftType:Primary_ForAircraft=factory;break;
    case AbstractType::Building:case AbstractType::BuildingType:(category==BuildCat::Combat?Primary_ForDefenses:Primary_ForBuildings)=factory;break;
    default:break;
    }
}
int HouseClass::BeginProduction(AbstractType type,int index,bool naval,bool resume){
    auto* product=TechnoTypeClass::GetByTypeAndIndex(type,index);if(!product)return 3;
    bool hold=false;
    if(!product->FindFactory(false,true,true,this)){
        if(!resume||!product->FindFactory(true,false,true,this))return 3;
        hold=true;
    }
    const auto category=product->WhatAmI()==AbstractType::BuildingType?static_cast<BuildingTypeClass*>(product)->BuildCat:BuildCat(0);
    auto* factory=GetPrimaryFactory(type,naval,category);
    if(!factory){factory=GameCreate<FactoryClass>();if(!factory)return 3;}
    if(factory->Production.Rate&&!factory->IsSuspended&&type==AbstractType::BuildingType)return 3;
    SetPrimaryFactory(factory,type,naval,category);
    const bool held=factory->IsSuspended&&factory->Object&&factory->Object->GetTechnoType()==product;
    if(held||factory->DemandProduction(product,this,resume)){
        if(!factory->QueuedObjects.Count||resume||held)factory->Unsuspend(hold);
        if(this==CurrentPlayer)SidebarClass::Instance.LinkFactory(factory,type,index);
        return 0;
    }
    if(!factory->QueuedObjects.Count&&!factory->Object){SetPrimaryFactory(nullptr,type,naval,category);GameDelete(factory);}
    return 3;
}
int HouseClass::SuspendProduction(AbstractType type,int index,bool naval){
    auto* product=TechnoTypeClass::GetByTypeAndIndex(type,index);
    const auto category=product&&product->WhatAmI()==AbstractType::BuildingType?static_cast<BuildingTypeClass*>(product)->BuildCat:BuildCat(0);
    auto* factory=GetPrimaryFactory(type,naval,category);if(!factory)return 3;
    factory->Suspend(true);if(this==CurrentPlayer)SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(type,index,0));return 0;
}
void HouseClass::AbandonProduction(AbstractType type,int index,bool naval,bool all){
    auto* product=TechnoTypeClass::GetByTypeAndIndex(type,index);
    const auto category=product&&product->WhatAmI()==AbstractType::BuildingType?static_cast<BuildingTypeClass*>(product)->BuildCat:BuildCat(0);
    auto* factory=GetPrimaryFactory(type,naval,category);if(!factory)return;
    if(factory->QueuedObjects.Count&&index>=0&&category==BuildCat(0)){
        const bool removed=factory->RemoveOneFromQueue(product);
        if(all)while(factory->RemoveOneFromQueue(product)){}
        if(removed&&this==CurrentPlayer)SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(type,index,0));
        if(removed&&!all)return;
    }
    if(index!=-1&&(!factory->Object||factory->Object->GetTechnoType()->GetArrayIndex()!=index))return;
    if(this==CurrentPlayer)SidebarClass::UnlinkFactory(type,index,factory);
    factory->AbandonProduction();
    if(factory->QueuedObjects.Count){factory->StartProduction();return;}
    SetPrimaryFactory(nullptr,type,naval,category);GameDelete(factory);
}
bool HouseClass::PlaceObject(AbstractType type,int index,bool naval,CellStruct cell){
    auto* factory=GetPrimaryFactory(type,naval,BuildCat(0));
    if(!factory||!factory->IsDone()||!factory->Object)return false;
    auto* product=factory->Object;
    // Auto-exit path (infantry and vehicles). Player building placement remains
    // owned by the separate construction-yard command chain.
    if(cell!=CellStruct::Empty&&cell!=CellStruct{-1,-1})return false;
    auto* building=product->GetTechnoType()->FindFactory(false,true,false,this);
    if(!building)return false;
    const auto result=building->KickOutUnit(product,CellStruct::Empty);
    if(result!=KickOutResult::Succeeded&&!(result==KickOutResult::Busy&&building->Factory)){AbandonProduction(type,index,naval,false);return false;}
    if(IsControlledByCurrentPlayer()&&!game::type_resources().audio_unavailable)VoxClass::Play("EVA_UnitReady");
    factory->CompletedProduction();
    if(this==CurrentPlayer)SidebarClass::UnlinkFactory(type,index,factory);
    // CompletedProduction clears Object, so do not use Abandon's index match.
    if(factory->QueuedObjects.Count)factory->StartProduction();
    else {SetPrimaryFactory(nullptr,type,naval,BuildCat(0));GameDelete(factory);}
    JustBuilt(product);
    if(!game::type_resources().audio_unavailable&&building->Type->CreateUnitSound!=-1)VocClass::PlayIndexAtPos(building->Type->CreateUnitSound,product->GetCoords(),0);
    return true;
}
void HouseClass::JustBuilt(TechnoClass* object){
    const int index=object->GetTechnoType()->GetArrayIndex();
    switch(object->WhatAmI()){
    case AbstractType::Infantry:FactoryProducedInfantryTypes.Increment(index);break;
    case AbstractType::Unit:FactoryProducedUnitTypes.Increment(index);break;
    case AbstractType::Aircraft:FactoryProducedAircraftTypes.Increment(index);break;
    case AbstractType::Building:FactoryProducedBuildingTypes.Increment(index);break;
    default:break;
    }
}
