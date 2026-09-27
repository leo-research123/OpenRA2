// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Goto_Tiberium/Search_For_Tiberium/Find_Docking_Bay;
// calibrated to YR 0x4DCE80..0x4DD0A0, 0x4DEE50..0x4DF040.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include <cmath>

CellStruct* FootClass::ScanForTiberium(CellStruct* output,int range,DWORD) const {
 auto& map=MapClass::Instance;const auto center=GetMapCoords();*output=CellStruct::Empty;
 if(GetCell()->LandType==LandType::Tiberium){*output=center;return output;}
 const int zone=map.GetMovementZoneType(CellClass::Coord2Cell(GetDestination()),GetTechnoType()->MovementZone,OnBridge);
 int value=-1;
 const auto inspect=[&](int dx,int dy){
  CellStruct at{short(center.X+dx),short(center.Y+dy)};auto* cell=map.TryGetCellAt(at);
  if(!cell||!map.IsWithinUsableArea(at,true)||cell->LandType!=LandType::Tiberium)return;
  if(map.GetMovementZoneType(at,GetTechnoType()->MovementZone,false)!=zone||IsCellOccupied(cell,FacingType(-1),-1,nullptr,true)!=Move::OK)return;
  const int candidate=cell->GetContainedTiberiumValue();if(candidate>value){value=candidate;*output=at;}
 };
 // Original examines a complete square ring, choosing the richest cell in
 // the first ring containing ore. Preserve edge order and strict tie-breaking.
 for(int radius=1;radius<range&&value==-1;++radius)for(int offset=-radius;offset<=radius;++offset){
  inspect(offset,-radius);inspect(offset,radius);inspect(-radius,offset);inspect(radius,offset);
 }
 return output;
}
bool FootClass::MoveToTiberium(int radius,bool scanClose){
 if(Destination)return false;
 CellStruct at;ScanForTiberium(&at,radius,scanClose);
 if(at==CellStruct::Empty)return false;
 if(at==GetMapCoords())return true;
 SetDestination(MapClass::Instance.GetCellAt(at),true);return false;
}
BuildingClass* FootClass::FindCloserDockBuilding(BuildingTypeClass* type,DWORD,DWORD allowBusy,int* distance) const {
 if(!type||!Owner||!distance)return nullptr;BuildingClass* best=nullptr;
 auto& map=MapClass::Instance;const auto origin=GetDestination();
 const int zone=map.GetMovementZoneType(CellClass::Coord2Cell(origin),GetTechnoType()->MovementZone,OnBridge);
 for(auto* building:Owner->Buildings){
  if(!building||building->InLimbo||!building->IsAlive||building->Type!=type||(!allowBusy&&!HasFreeLink()))continue;
  // The refinery's docking cell is passable even though its center is not.
  const auto anchor=building->GetMapCoords();const CellStruct dock{short(anchor.X+3),short(anchor.Y+1)};
  if(map.GetMovementZoneType(dock,GetTechnoType()->MovementZone,false)!=zone)continue;
  if(const_cast<FootClass*>(this)->SendCommand(RadioCommand::QueryCanEnter,building)!=RadioCommand::AnswerPositive)continue;
  const auto delta=building->GetCoords()-GetCoords();const int candidate=int(std::sqrt(double(delta.X)*delta.X+double(delta.Y)*delta.Y+double(delta.Z)*delta.Z));
  if(*distance==-1||candidate<*distance||building->IsPrimaryFactory){best=building;*distance=candidate;}
 }
 return best;
}
BuildingClass* FootClass::FindNearestDockBuilding(BuildingTypeClass* type,DWORD arg,DWORD busy) const {int distance=-1;return FindCloserDockBuilding(type,arg,busy,&distance);}
BuildingClass* FootClass::TryNearestDockBuilding(TypeList<BuildingTypeClass*>* types,DWORD arg,DWORD busy) const {
 BuildingClass* result=nullptr;int distance=-1;if(types)for(auto* type:*types)if(auto* found=FindCloserDockBuilding(type,arg,busy,&distance))result=found;return result;
}
