// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; additional terms in third_party/opents/LICENSE.md.
// OpenTS smudtype.cpp 44fac744f70235e0d5ddca107364a68f95132ce9; YR 0x006B59A0–0x006B6080.
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_world.hpp"
#include <vector>
bool SmudgeTypeClass::CanPlaceHere(const CellStruct& origin,bool underBuildings) const{
 if(!MapClass::Instance.IsWithinUsableArea2D(origin))return false;
 for(int y=0;y<Height;++y)for(int x=0;x<Width;++x){
  auto*cell=MapClass::Instance.TryGetCellAt(CellStruct{short(origin.X+x),short(origin.Y+y)});
  if(!cell||cell->SlopeIndex||cell->SmudgeTypeIndex!=-1||cell->OverlayTypeIndex!=-1)return false;
  if(!underBuildings)for(auto*object=cell->FirstObject;object;object=object->NextObject)if(object->WhatAmI()==AbstractType::Building)return false;
  auto*tile=IsometricTileTypeClass::Array.GetItemOrDefault(cell->IsoTileTypeIndex);
  if(!tile)tile=IsometricTileTypeClass::Array.GetItemOrDefault(0);
  if(!tile||!tile->Morphable)return false;
 }
 return true;
}
void SmudgeTypeClass::Place(const CellStruct& origin) const{
 for(int y=0;y<Height;++y)for(int x=0;x<Width;++x)
  if(auto*cell=MapClass::Instance.TryGetCellAt(CellStruct{short(origin.X+x),short(origin.Y+y)})){
   cell->SmudgeTypeIndex=ArrayIndex;cell->SmudgeData=BYTE(x+y*Width);game::map_resource_changed(*cell);
  }
}
namespace {
bool mark(const CoordStruct& coordinate,int width,int height,bool large,bool crater){
 const CellStruct origin{short(coordinate.X/256),short(coordinate.Y/256)};
 if(origin==CellStruct{0,0}||!ScenarioClass::Instance)return false;
 std::vector<SmudgeTypeClass*> candidates,preferred;
 for(auto*type:SmudgeTypeClass::Array)if((crater?type->Crater:type->Burn)&&type->CanPlaceHere(origin,large)){
  candidates.push_back(type);
  if(large?(type->Width>1&&type->Height>1):((type->Width==1&&type->Height==1)||(width>60&&height>50)))preferred.push_back(type);
 }
 auto&list=preferred.empty()?candidates:preferred;
 if(!list.empty())list[ScenarioClass::Instance->Random.RandomRanged(0,int(list.size())-1)]->Place(origin);
 return false; // Original return value, including successful placement.
}
}
bool SmudgeTypeClass::ScorchTheGround(const CoordStruct& p,int w,int h,bool large){return mark(p,w,h,large,false);}
bool SmudgeTypeClass::CraterTheGround(const CoordStruct& p,int w,int h,bool large){return mark(p,w,h,large,true);}
