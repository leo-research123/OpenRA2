// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 tactical.cpp background passes, calibrated to YR entry points.
// Native hosts redraw a complete target. DDraw strip/dirty-surface replay is not
// required here; original cell/object decisions and ordering remain in the core.
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/AlphaShapeClass.h"
#include "tactical_drawing.hpp"
#include "map_runtime.hpp"
#include <algorithm>

namespace {
bool complete(){auto* f=game::tactical_drawing();return f&&(f->status==game::DrawingStatus::drawn||f->status==game::DrawingStatus::skipped);}
RectangleStruct viewport(){auto* f=game::tactical_drawing();return f&&f->bounds.Width>0?f->bounds:TacticalClass::ViewBounds;}
bool overlaps(const RectangleStruct& a,const RectangleStruct& b){
 return a.Width>0&&a.Height>0&&b.Width>0&&b.Height>0&&a.X<b.X+b.Width&&a.Y<b.Y+b.Height&&a.X+a.Width>b.X&&a.Y+a.Height>b.Y;
}
template<class F> void cells(TacticalClass& tactical,const RectangleStruct& area,int extraRows,int extraColumns,int dy,bool clampOrigin,F&& draw){
 if(area.Width<=0||area.Height<=0)return;
 const auto bounds=viewport();
 auto at=tactical.ApplyMatrix_Pixel({tactical.TacticalPos.X+area.X-bounds.X,tactical.TacticalPos.Y+area.Y-bounds.Y});
 if(clampOrigin){at.X=std::max(0,at.X);at.Y=std::max(0,at.Y);}
 const CellStruct base{short(at.X/256-2),short(at.Y/256+dy)};
 for(int row=0;row<area.Height/15+extraRows&&complete();++row){
  CellStruct cell{short(base.X+row/2),short(base.Y+(row+1)/2)};
  for(int col=0;col<area.Width/60+extraColumns&&complete();++col,++cell.X,--cell.Y){
   if(!MapClass::Instance.CoordinatesLegal(cell))continue;
   auto* item=MapClass::Instance.TryGetCellAt(cell);if(!item)continue;
   const auto flat=TacticalClass::CoordsToScreen({cell.X*256,cell.Y*256,0});
   draw(*item,Point2D{flat.X-tactical.TacticalPos.X-30,flat.Y-tactical.TacticalPos.Y});
  }
 }
}
}

void TacticalClass::DrawTiles(const RectangleStruct& area,const RectangleStruct& clip){
 auto* frame=game::tactical_drawing();if(!frame||!frame->drawing)return;
 try{cells(*this,area,17,4,0,false,[&](CellClass& cell,const Point2D& at){
  // Cell/TMP perform artwork and multi-cell-smudge clipping; do not discard a
  // smudge merely because the owning TMP's diamond is outside the rectangle.
  const auto status=game::draw_cell_terrain(cell,*frame->drawing,at,clip);
  game::record_tactical_drawing(status);
  if(frame->statistics){++frame->statistics->visited;if(status==game::DrawingStatus::drawn)++frame->statistics->drawn;else if(status==game::DrawingStatus::skipped)++frame->statistics->skipped;}
 });}catch(...){game::record_tactical_drawing(game::DrawingStatus::backend_failure);}
}

void TacticalClass::DrawShroud(const RectangleStruct& area){
 try{const auto bounds=viewport();
 cells(*this,area,17,4,0,true,[&](CellClass& cell,const Point2D& at){
  const RectangleStruct footprint{at.X,bounds.Y+at.Y,60,30};
  const auto clip=Drawing::Intersect(bounds,Drawing::Intersect(area,footprint));
  if(clip.Width>0&&clip.Height>0)cell.DrawShroudAndFog({at.X+bounds.X,at.Y+bounds.Y},clip);
 });
 if(complete())AlphaShapeClass::DrawAll(area);
 }catch(...){game::record_tactical_drawing(game::DrawingStatus::backend_failure);}
}

void TacticalClass::DrawTileShadows(const RectangleStruct& area,const RectangleStruct& clip){
 try{cells(*this,area,20,7,3,false,[&](CellClass& cell,const Point2D& at){cell.DrawShadowCast(at,clip);});}
 catch(...){game::record_tactical_drawing(game::DrawingStatus::backend_failure);}
}

void YRPP_STDCALL TacticalClass::DrawTerrain(bool forced,const RectangleStruct& area,const RectangleStruct& clip){
 try{const auto intersection=Drawing::Intersect(area,clip),bounds=viewport();
 auto& layer=MapClass::ObjectsInLayers[int(Layer::Ground)];
 for(int i=layer.Count-1;i>=0&&complete();--i){
  auto* object=layer[i];if(object->WhatAmI()!=AbstractType::Terrain||object->IsDead())continue;
  auto* terrain=static_cast<TerrainClass*>(object);
  if(!terrain->Type||terrain->Type->IsAnimated||terrain->IsCrumbling)continue;
  RectangleStruct dimensions;object->GetRenderDimensions(&dimensions);dimensions.X+=bounds.X;dimensions.Y+=bounds.Y;
  if(!overlaps(dimensions,intersection))continue;
  game::record_tactical_drawing(game::draw_tactical_object(*object,forced,clip,false));
  object->NeedsRedraw=false;
 }
 }catch(...){game::record_tactical_drawing(game::DrawingStatus::backend_failure);}
}

void YRPP_STDCALL TacticalClass::DrawBuildings(bool forced,const RectangleStruct& area,const RectangleStruct& clip){
 try{const auto intersection=Drawing::Intersect(area,clip),bounds=viewport();
 const auto& runtime=game::map_runtime();const bool debug=runtime.debug_map&&*runtime.debug_map;
 auto& layer=MapClass::ObjectsInLayers[int(Layer::Ground)];
 for(int i=0;i<layer.Count&&complete();++i){
  auto* object=layer[i];if(object->WhatAmI()!=AbstractType::Building)continue;
  auto* building=static_cast<BuildingClass*>(object);if(!building->Type||(building->IsFogged&&!debug))continue;
  RectangleStruct dimensions;object->GetRenderDimensions(&dimensions);dimensions.X+=bounds.X;dimensions.Y+=bounds.Y;
  if(!overlaps(dimensions,intersection))continue;
  game::record_tactical_drawing(game::draw_tactical_object(*object,forced,clip,false));
  object->NeedsRedraw=false;
 }
 }catch(...){game::record_tactical_drawing(game::DrawingStatus::backend_failure);}
}
