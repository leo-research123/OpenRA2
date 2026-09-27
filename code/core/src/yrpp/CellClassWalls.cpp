// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 cell.cpp Reduce_Wall / Wall_Update; YR 0x480CB0/0x480630.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_world.hpp"

namespace {
void release_adjacent_count(CellClass& cell){
 for(int direction=0;direction<8;++direction){auto* adjacent=cell.GetNeighbourCell(static_cast<FacingType>(direction));
  if(adjacent!=&MapClass::InvalidCell)--adjacent->BlockedNeighbours;
 }
}
}

void CellClass::DamageWall(int damage){
 auto* wall=OverlayTypeClass::Array.GetItemOrDefault(OverlayTypeIndex);
 if(!wall||!wall->Wall)return;
 if(damage!=-1&&damage<wall->Strength&&!Unsorted::ScenarioInit
     &&ScenarioClass::Instance->Random.RandomRanged(0,wall->Strength)>=damage)return;
 OverlayData=BYTE(OverlayData+0x10);game::map_resource_changed(*this);
 if((OverlayData>>4)==wall->DamageLevels-1&&wall->DamageLevels>2){
  for(int direction=0;direction<8;direction+=2){auto* adjacent=GetNeighbourCell(static_cast<FacingType>(direction));
   if(adjacent->OverlayTypeIndex==OverlayTypeIndex&&adjacent->OverlayData<0x10)adjacent->DamageWall(200);
  }
 }
 const int stage=OverlayData>>4;
 if(damage!=-1&&stage<wall->DamageLevels&&(stage!=wall->DamageLevels-1||(OverlayData&0xF)))return;
 WallOwnerIndex=-1;OverlayTypeIndex=-1;OverlayData=0;
 RecalcAttributes(-1);auto& map=MapClass::Instance;
 map.ResetZones(MapCoords);map.RecalculateSubZones(MapCoords);game::map_resource_changed(*this);
 for(int direction:{0,6,4,2})GetNeighbourCell(static_cast<FacingType>(direction))->UpdateWall();
 NotifyObjectExpired(true);release_adjacent_count(*this);
}

void CellClass::UpdateWall(bool){
 // Original visits N/E/S/W/self. Native presentation invalidates complete
 // cell/overlay packets instead of the EXE's old/new rectangle union.
 for(int direction:{0,2,4,6,-1}){
  auto* cell=direction<0?this:GetNeighbourCell(static_cast<FacingType>(direction));
  if(cell==&MapClass::InvalidCell)continue;
  game::map_resource_changed(*cell);
  const int overlay=cell->OverlayTypeIndex;auto* type=OverlayTypeClass::Array.GetItemOrDefault(overlay);
  if(!type||!type->Wall)continue;
  unsigned connections=0;
  for(int side=0;side<4;++side)if(cell->GetNeighbourCell(static_cast<FacingType>(side*2))->ConnectsToOverlay(overlay,side*2))connections|=1u<<side;
  cell->OverlayData=BYTE((cell->OverlayData&0xF0)|connections);
  const int frame=cell->OverlayData;const auto oldPassability=cell->Passability;
  const bool removed=((overlay==2||overlay==26)&&(frame==0x30||frame==0x20))
    ||((overlay==0||overlay==22)&&(frame==0x10||frame==0x20))
    ||(overlay==1&&frame==0x20)||(overlay==3&&frame==0x10);
  if(removed){
   cell->OverlayTypeIndex=-1;cell->OverlayData=0;
   // The YR brick/Nod branch retains owner; the other original arms clear it.
   if(overlay!=2&&overlay!=26)cell->WallOwnerIndex=-1;
   cell->NotifyObjectExpired(true);
  }
  cell->RecalcAttributes(-1);
  if(cell->Passability!=oldPassability){auto& map=MapClass::Instance;
   if(removed)map.ResetZones(cell->MapCoords);else map.RecalculateZones(cell->MapCoords);
   map.RecalculateSubZones(cell->MapCoords);
   if(removed)release_adjacent_count(*cell);
  }
 }
}
