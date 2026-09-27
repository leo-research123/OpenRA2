// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp::Captured; YR 0x448260 ownership lists.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>
#include "map_world.hpp"
#include "type_resources.hpp"

namespace {
void transfer_list(DynamicVectorClass<BuildingClass*>& from,DynamicVectorClass<BuildingClass*>& to,BuildingClass* building,bool add){
 from.Remove(building);if(add&&to.FindItemIndex(building)<0)to.AddItem(building);
}
}
bool BuildingClass::SetOwningHouse(HouseClass* house,bool announce){
 if(house==Owner)return false;
 auto* old=Owner;
 if(old->Type->MultiplayPassive&&Type->ProduceCashStartup){house->GiveMoney(Type->ProduceCashStartup);CashProductionTimer.Start(Type->ProduceCashDelay);}
 DisplayProductionTo.Remove(house->Type->ArrayIndex);IsPrimaryFactory=false;
 if(announce&&!house->Type->MultiplayPassive&&!game::type_resources().audio_unavailable&&(old->IsControlledByCurrentPlayer()||house->IsControlledByCurrentPlayer())){
  if(Type->NeedsEngineer){if(old->IsControlledByCurrentPlayer())VoxClass::Play("EVA_TechBuildingLost",-1,-1);
   if(house->IsControlledByCurrentPlayer()&&Type->CaptureEvaEvent!=-1)VoxClass::PlayIndex(Type->CaptureEvaEvent,-1,-1);
  }else if(RadarEventClass::Create(RadarEventType::BuildingCaptured,GetMapCoords()))VoxClass::Play("EVA_BuildingCaptured",-1,-1);
 }
 NeedsRedraw=HasEngineer=HasBeenCaptured=true;
 if(Type->NeedsEngineer)StuffEnabled=true;
 if(Factory){Factory->AbandonProduction();GameDelete(Factory);Factory=nullptr;}
 // Lists are independent from the active/total type counters in Techno.Captured.
 transfer_list(old->ConYards,house->ConYards,this,RulesClass::Instance->BuildConst.FindItemIndex(Type)>=0);
 transfer_list(old->UnitRepairStations,house->UnitRepairStations,this,Type->UnitRepair);
 transfer_list(old->Grinders,house->Grinders,this,Type->Grinding);
 transfer_list(old->Absorbers,house->Absorbers,this,Type->InfantryAbsorb||Type->UnitAbsorb);
 transfer_list(old->Bunkers,house->Bunkers,this,Type->Bunker);
 transfer_list(old->Occupiables,house->Occupiables,this,Type->CanBeOccupied&&Type->TechLevel>-1);
 transfer_list(old->CloningVats,house->CloningVats,this,Type->Cloning);
 transfer_list(old->SecretLabs,house->SecretLabs,this,Type->SecretLab);
 transfer_list(old->PsychicDetectionBuildings,house->PsychicDetectionBuildings,this,Type->PsychicDetectionRadius>0);
 transfer_list(old->FactoryPlants,house->FactoryPlants,this,Type->FactoryPlant);
 transfer_list(old->Buildings,house->Buildings,this,true);
 if(Type->OrePurifier){--old->NumOrePurifiers;++house->NumOrePurifiers;}
 old->InfantrySelfHeal=std::max(0,old->InfantrySelfHeal-Type->InfantryGainSelfHeal);house->InfantrySelfHeal+=Type->InfantryGainSelfHeal;
 old->UnitsSelfHeal=std::max(0,old->UnitsSelfHeal-Type->UnitsGainSelfHeal);house->UnitsSelfHeal+=Type->UnitsGainSelfHeal;
 TechnoClass::SetOwningHouse(house,true);
 old->ToCapture=this;if(house->ToCapture==this)house->ToCapture=nullptr;
 IsBeingRepaired=false;
 old->RecheckTechTree=house->RecheckTechTree=true;
 old->RecheckPower=house->RecheckPower=true;
 old->RecheckRadar=house->RecheckRadar=true;
 UpdateAnimAppearance();Mark(MarkType::Change);game::map_object_changed();
 return true;
}

void BuildingClass::GotHijacked(){
 for(auto* foot:FootClass::Array)if(foot->IsAlive){
  const auto at=foot->GetDestination();
  if(MapClass::Instance.GetCellAt(at)->GetBuilding()==this&&(!foot->Destination||foot->Destination==this))
   foot->Scatter(CoordStruct::Empty,true,true);
 }
}
