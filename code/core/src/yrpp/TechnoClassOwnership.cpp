// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Captured; calibrated to YR 0x7014A0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/MapClass.h"

void TechnoClass::RemoveFromTargeting() {
 for(int i=Array.Count-1;i>=0;--i){
  auto* actor=Array[i];
  const bool keep=actor->TemporalImUsing&&actor->TemporalImUsing->Target==this&&Health>0&&IsAlive;
  if(actor->Target==this&&!keep){
   actor->Mission_Revert();
   if(actor->WhatAmI()==AbstractType::Aircraft&&actor->CurrentMission==Mission::Patrol){actor->MissionStatus=0;static_cast<AircraftClass*>(actor)->IsLocked=false;}
   if(actor->Target==this)actor->SetTarget(nullptr);
  }
 }
 // Every live spawn manager belongs to a Techno. Iterate that native registry
 // instead of dereferencing the original executable's manager-array address.
 for(auto* actor:Array)if(auto* manager=actor->SpawnManager){
  if(manager->Target==this)manager->Target=nullptr;
  if(manager->NewTarget==this)manager->NewTarget=nullptr;
 }
}

bool TechnoClass::SetOwningHouse(HouseClass* house,bool) {
 if(house==Owner)return false;
 auto* old=Owner;auto* type=GetTechnoType();
 if(old==HouseClass::CurrentPlayer&&IsSelected)Deselect();
 SetTarget(nullptr);SetDestination(nullptr,true);
 const auto* unit=WhatAmI()==AbstractType::Unit?static_cast<UnitClass*>(this):nullptr;
 if(!unit||!unit->IsDeployingOrUndeploying())ArchiveTarget=nullptr;
 if(CurrentMission!=Mission::Selling&&(!unit||!unit->Type->Harvester||CurrentMission!=Mission::Unload))QueueMission(Mission::Guard,true);
 if(SpawnManager)SpawnManager->ResetTarget();
 if(PlanningToken)ClearPlanningTokens(nullptr);
 if(!InLimbo)old->RegisterLoss(this,false);
 RegisterDestruction(nullptr);
 house->PointTotal+=type->GetActualCost(old);
 old->RemoveTracking(this);house->AddTracking(this);
 if(!type->DontScore&&old->ArrayIndex>=0&&old->ArrayIndex<20){
  if(WhatAmI()==AbstractType::Building)++house->KilledBuildingsOfHouses[old->ArrayIndex];
  else if(WhatAmI()==AbstractType::Infantry||WhatAmI()==AbstractType::Unit||WhatAmI()==AbstractType::Aircraft)++house->KilledUnitsOfHouses[old->ArrayIndex];
 }
 old->WhoLastHurtMe=house->Type->ArrayIndex;
 if(!InLimbo)Mark(MarkType::Up);
 RemoveFromTargeting();
 if(!InLimbo)Mark(MarkType::ChangeRedraw);
 CellClass* cell=nullptr;
 if(!InLimbo){
  cell=(AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None?MapClass::Instance.GetCellAt(static_cast<FootClass*>(this)->LastMapCoords):GetCell();
  if(cell){cell->UpdateThreat(old->ArrayIndex,-int(ThreatPosed));ThreatPosed=0;}
  vt_entry_48C(false,0,false,nullptr);
 }
 Owner=house;IsOwnedByCurrentPlayer=house==HouseClass::CurrentPlayer&&HouseClass::CurrentPlayer;
 if(!InLimbo){if(cell)AddThreatToCell(cell);house->RegisterGain(this,true);}
 MindControlledByHouse=nullptr;
 if(CurrentMission==Mission::Rescue||QueuedMission==Mission::Rescue)ForceMission(Mission::Guard);
 if(!InLimbo){
  const auto* building=WhatAmI()==AbstractType::Building?static_cast<BuildingClass*>(this):nullptr;
  auto* link=GetNthLink();
  if((!building||CurrentMission!=Mission::Unload||!building->Type->WeaponsFactory)
     &&(!link||link->WhatAmI()!=AbstractType::Building||!static_cast<BuildingClass*>(link)->Type->WeaponsFactory)){
   SetDestination(nullptr,true);SetTarget(nullptr);EnterIdleMode(false,true);
  }
  if(IsRadarTracked){RadarTrackingStop();RadarTrackingStart();}
  UpdateSight(0,0,0,0,0);
 }
 return true;
}
