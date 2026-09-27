// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Grand_Opening, calibrated to YR 0x445F80.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ParticleSystemClass.h"

bool BuildingClass::IsCellImpassable(CellClass* cell) const {
 if(!cell||cell->GetBuilding()!=this)return false;
 return Type->NumberImpassableRows==-1||(Type->UnitReload&&BunkerLinkedItem)
     ||GetMapCoords().X+Type->NumberImpassableRows>cell->MapCoords.X;
}

void BuildingClass::Place(bool captured){
 if(ActuallyPlacedOnMap&&!captured)return;
 if(!ActuallyPlacedOnMap)IsReadyToCommence=true;
 ActuallyPlacedOnMap=true;Owner->RecheckPower=Owner->RecheckRadar=Owner->RecheckTechTree=true;
 if(captured||Unsorted::ScenarioInit||!Type->FreeUnit||(Owner->IsControlledByHuman()&&Value&&Value<=Type->Cost))return;
 // Grant once at construction completion. Scenario loading and capture have
 // already been excluded; repeated activation cannot create more miners.
 auto* unit=GameCreate<UnitClass>(Type->FreeUnit,Owner);
 if(!unit){Owner->GiveMoney(Type->FreeUnit->GetActualCost(Owner));return;}
 if(!unit->InitializeLocomotor()){GameDelete(unit);return;}
 auto& map=MapClass::Instance;const auto center=CellClass::Coord2Cell(GetCoords());
 const auto delta=Unsorted::AdjacentCell[static_cast<int>(FacingType::South)];
 const CellStruct first{short(center.X+delta.X),short(center.Y+delta.Y)};
 const auto place=[&](CellStruct at,DirType facing){auto* cell=map.TryGetCellAt(at);return cell&&map.IsWithinUsableArea(at,true)&&unit->Unlimbo(cell->GetCoords(),facing);};
 bool placed=place(first,DirType::West);
 for(bool requireEmpty:{true,false})if(!placed){
  const int zone=map.GetMovementZoneType(GetMapCoords(),unit->Type->MovementZone,false);
  const auto at=map.NearByLocation(GetMapCoords(),SpeedType::Wheel,zone,unit->Type->MovementZone,false,1,1,requireEmpty,true,false,false,CellStruct::Empty,false,false);
  if(at!=CellStruct::Empty)placed=place(at,DirType::SouthWest);
 }
 if(!placed){Owner->GiveMoney(Type->FreeUnit->GetActualCost(Owner));GameDelete(unit);return;}
 unit->QueueMission(Mission::Harvest,false);unit->NextMission();unit->UpdateTimer.Start(0);
 if(LogicClass::Instance.FindItemIndex(unit)<0)LogicClass::Instance.AddObject(unit,false);
}

// YR adds this hook to the OpenTS harvester unloading sequence (0x73E37E).
void BuildingClass::UpdateRefinerySmokeSystems(){
 if(!Type->RefinerySmokeParticleSystem)return;
 for(const auto offset:{Type->RefinerySmokeOffsetOne,Type->RefinerySmokeOffsetTwo,
      Type->RefinerySmokeOffsetThree,Type->RefinerySmokeOffsetFour}){
  // Original skips both empty coordinates and cell-centre sentinel.
  if(offset==CoordStruct::Empty||offset==CoordStruct{128,128,0})continue;
  auto* system=GameCreate<ParticleSystemClass>(Type->RefinerySmokeParticleSystem,
      Location+offset,nullptr,this,CoordStruct::Empty,nullptr);
  if(system)system->Lifetime=Type->RefinerySmokeFrames; // 0x006301F0
 }
}
