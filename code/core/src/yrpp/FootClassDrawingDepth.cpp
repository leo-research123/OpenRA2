// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::Get_Z_Adjust and techno.cpp Z fudge helpers;
// calibrated to YR 0x4DAFC0, 0x703CC0, 0x703E70, 0x704000, 0x704240.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <algorithm>
namespace {
bool bridge_extension(const FootClass& actor,const CellClass& cell){
 if(actor.OnBridge)return false;
 if((static_cast<unsigned>(cell.Flags)&0x400u)!=0)return true;
 for(const int direction:{4,6,2,0})if(const auto* neighbor=cell.GetNeighbourCell(static_cast<FacingType>(direction))){
  const auto flags=static_cast<unsigned>(neighbor->Flags);
  if((flags&0x400u)&&bool(flags&0x800u)==(direction==4||direction==0))return true;
 }
 return false;
}
int column_fudge(const FootClass& actor,const CellClass& cell){
 if(!actor.IsUnderBridge()&&!bridge_extension(actor,cell))return 0;
 const CellClass* neighbors[]{cell.GetNeighbourCell(FacingType::South),cell.GetNeighbourCell(FacingType::East),cell.GetNeighbourCell(FacingType::SouthEast)};
 for(const auto* neighbor:neighbors)if(!neighbor)return 0;
 int result=0;
 for(int i=0;i<3;++i){const int tile=neighbors[i]->IsoTileTypeIndex;
  if(tile==0xFFFF||tile==0xFF)continue;
  const int relative=tile-IsometricTileTypeClass::BridgeSet+1;
  if(relative>=7&&relative<=16){if(i==2)++result;else result=1;}
 }
 return result;
}
int tunnel_fudge(const FootClass& actor,const CellClass& cell){
 if(actor.OnBridge)return 0;
 const auto* north=cell.GetNeighbourCell(FacingType::North);
 const auto* west=cell.GetNeighbourCell(FacingType::West);
 if(!north||!west)return 0;
 const CellClass* mouth=cell.Tile_Is_Tunnel()?&cell:north->Tile_Is_Tunnel()?north:west->Tile_Is_Tunnel()?west:nullptr;
 if(!mouth)return 0;
 north=mouth->GetNeighbourCell(FacingType::North);west=mouth->GetNeighbourCell(FacingType::West);
 if(!north||!west)return 0;
 north=north->GetNeighbourCell(FacingType::North);west=west->GetNeighbourCell(FacingType::West);
 return north&&west&&(north->Tile_Is_Tunnel()||west->Tile_Is_Tunnel())?1:0;
}
int cliff_fudge(const FootClass& actor,const CellClass& cell){
 if(actor.OnBridge)return 0;
 const auto* first=cell.GetNeighbourCell(FacingType::SouthEast);if(!first)return 0;
 int result=static_cast<signed char>(first->Level)-static_cast<signed char>(cell.Level)>=4?2:0;
 const auto* second=first->GetNeighbourCell(FacingType::SouthEast);
 if(second&&static_cast<signed char>(second->Level)-static_cast<signed char>(cell.Level)>=4)result=1;
 return result;
}
}
int FootClass::GetZAdjustment() const {
 const auto* cell=MapClass::Instance.TryGetCellAt(GetMapCoords());const auto* type=GetTechnoType();
 int fudge=0;
 if(cell&&type)fudge=std::max({column_fudge(*this,*cell)*type->ZFudgeColumn,
  tunnel_fudge(*this,*cell)*type->ZFudgeTunnel,cliff_fudge(*this,*cell)*type->ZFudgeCliff,
  IsUnderBridge()?type->ZFudgeBridge:0});
 return TechnoClass::GetZAdjustment()+(Locomotor?Locomotor->Z_Adjust():0)+fudge;
}
