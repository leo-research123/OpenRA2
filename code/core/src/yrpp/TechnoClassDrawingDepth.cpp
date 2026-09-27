// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Get_Z_Adjust; calibrated to YR 0x704350.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"

int TechnoClass::GetZAdjustment() const {
 const int z=-TacticalClass::AdjustForZ(GetZ());
 auto* cell=MapClass::Instance.TryGetCellAt(GetMapCoords());if(!cell)return z-1;
 if(WhatAmI()==AbstractType::Unit){
  const auto* unit=static_cast<const UnitClass*>(this);
  const auto* link=GetNthLink();
  if(IsTether&&link&&link->WhatAmI()==AbstractType::Building&&link->GetCurrentMission()==Mission::Unload)return z-3;
  if(unit->Type->Harvester)if(auto* building=cell->GetBuilding();building&&building->Type->Refinery)return z-14;
 }
 if(cell->SlopeIndex){
  const auto* south=cell->GetNeighbourCell(FacingType::South);
  const auto* east=cell->GetNeighbourCell(FacingType::East);
  const auto* southeast=cell->GetNeighbourCell(FacingType::SouthEast);
  if(south&&east&&southeast){
   // The original tests only the first present overlay in this order.
   for(const auto* neighbor:{south,east,southeast})if(neighbor->OverlayTypeIndex!=-1){
    const auto* overlay=OverlayTypeClass::Array.GetItemOrDefault(neighbor->OverlayTypeIndex);
    if(overlay&&overlay->IsARock)return z+2;
    break;
   }
  }
 }
 const int facing=PrimaryFacing.Current().GetValue<3>();
 const CellClass* neighbors[6];
 const int directions[]{facing+1,facing-1,facing,facing-4,facing+3,facing-3};
 for(int i=0;i<6;++i){neighbors[i]=cell->GetNeighbourCell(static_cast<FacingType>(directions[i]&7));if(!neighbors[i])return z-1;}
 for(const auto* neighbor:neighbors)if(neighbor->SlopeIndex)return z-1;
 if(cell->SlopeIndex)return z-1;
 const CellClass* const* candidates;
 if(facing==6||facing==0)candidates=neighbors+3;
 else if(facing==4||facing==2)candidates=neighbors;
 else return z-1;
 if((static_cast<unsigned>(cell->Flags)&0x10000u)!=0)return z-1;
 for(int i=0;i<3;++i)if((static_cast<unsigned>(candidates[i]->Flags)&0x10000u)!=0)return z-1;
 for(int i=0;i<3;++i){
  int index=candidates[i]->IsoTileTypeIndex,subtile=static_cast<unsigned char>(candidates[i]->Height);
  if(index==0xFFFF||index==0xFF){index=IsometricTileTypeClass::ClearTile;subtile=0;}
  const auto* tile=IsometricTileTypeClass::Array.GetItemOrDefault(index);int width=0,height=0;
  if(tile&&tile->GetTileDimensions(subtile,width,height)&&height>36)return z-2;
 }
 return z-1;
}
