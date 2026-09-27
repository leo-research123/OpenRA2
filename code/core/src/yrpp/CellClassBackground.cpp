// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 cell.cpp Draw_Shroud_And_Fog / Draw_Shadow_Cast.
// YR 0x004801F0 / 0x004802A0. Actual ABuffer writes use the typed backend.
#include "yrpp/CellClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/HouseClass.h"
#include "tactical_drawing.hpp"

void CellClass::DrawShroudAndFog(const Point2D& point,const RectangleStruct& clip){
 if(!TacticalClass::Instance)return;
 Visibility=TacticalClass::Instance->GetOcclusion(MapCoords,false);
 Foggedness=TacticalClass::Instance->GetOcclusion(MapCoords,true);
 const bool fog=ScenarioClass::Instance&&ScenarioClass::Instance->SpecialFlags.FogOfWar;
 const auto index=[](char state){return state==-2?15:state==-1?0:int(state);};
 // YR also submits frame zero: shroud zero pixels are valid ABuffer writes.
 {
  auto* image=static_cast<SHPStruct*>(FileSystem::LoadFile(fog?"FOG.SHP":"SHROUD.SHP",true));
  game::record_tactical_drawing(game::draw_lighting_shape(image,index(Visibility),point,clip,game::RasterBlendMode::shroud));
 }
 // YR checks CurrentPlayer->Defeated (+0x1F5), unlike OpenTS Session.ObiWan.
 if(fog&&(!HouseClass::CurrentPlayer||!HouseClass::CurrentPlayer->Defeated)){
  auto* image=static_cast<SHPStruct*>(FileSystem::LoadFile("FOG.SHP",true));
  game::record_tactical_drawing(game::draw_lighting_shape(image,index(Foggedness),point,clip,game::RasterBlendMode::fog));
 }
}
void CellClass::DrawShadowCast(const Point2D& point,const RectangleStruct& clip){
 auto* type=IsometricTileTypeClass::Array.GetItemOrDefault(IsoTileTypeIndex==0xFFFF?IsometricTileTypeClass::ClearTile:IsoTileTypeIndex);
 if(!type){game::record_tactical_drawing(game::DrawingStatus::unavailable);return;}
 const int sub=IsoTileTypeIndex==0xFFFF?0:static_cast<unsigned char>(Height);
 if(type->ShadowCaster){
  const int level=static_cast<signed char>(Level);
  type->DrawShadowCaster(sub,nullptr,{point.X,point.Y-15*level},clip,58-15*level);
 }
}
