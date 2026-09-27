// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 isotype.cpp Draw_Shadow_Caster; YR 0x00547230.
// YR cliff offsets use 60x30 units (initializer 0x00543F10); slope offsets
// retain 48x24 units (0x00544690). These tables must not share one scale.
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/TacticalClass.h"
#include "tactical_drawing.hpp"
#include "map_world_internal.hpp"
namespace {
struct Caster{int frame,index,x,y;};
constexpr Caster cliffs[40]={
 {0,0,0,0}, // 0
 {0,0,0,0}, // 1
 {0,0,0,0}, // 2
 {0,0,0,0}, // 3
 {0,0,0,0}, // 4
 {0,0,0,0}, // 5
 {0,0,0,0}, // 6
 {0,0,0,0}, // 7
 {0,0,0,0}, // 8
 {0,0,0,0}, // 9
 {0,0,0,0}, // 10
 {0,0,0,0}, // 11
 {0,0,0,0}, // 12
 {0,0,0,0}, // 13
 {0,0,0,0}, // 14
 {0,0,0,0}, // 15
 {0,0,0,0}, // 16
 {0,0,30,30}, // 17
 {0,0,30,30}, // 18
 {0,0,30,30}, // 19
 {1,0,30,30}, // 20
 {1,0,30,30}, // 21
 {2,1,60,15}, // 22
 {3,1,60,15}, // 23
 {4,1,60,15}, // 24
 {5,0,90,30}, // 25
 {6,0,60,15}, // 26
 {7,1,30,0}, // 27
 {8,0,60,15}, // 28
 {9,0,60,15}, // 29
 {10,1,0,-15}, // 30
 {11,1,0,-15}, // 31
 {12,0,60,15}, // 32
 {0,0,0,0}, // 33
 {0,0,0,0}, // 34
 {0,0,0,0}, // 35
 {0,0,0,0}, // 36
 {0,0,0,0}, // 37
 {0,0,0,0}, // 38
 {0,0,0,0}, // 39
};
constexpr Caster slopes[10]={{},{},{},{},{13,6,48,12},{},{14,1,48,12},{},{},{}};
}
void IsometricTileTypeClass::DrawShadowCaster(int sub,Surface*,Point2D point,RectangleStruct clip,int depth){
 auto* frame=game::tactical_drawing();if(!frame||!frame->world)return;
 const Caster* caster=nullptr;
 if(ArrayIndex>=SlopeSetPieces&&ArrayIndex<SlopeSetPieces+10)caster=&slopes[ArrayIndex-SlopeSetPieces];
 else if(ArrayIndex>=SlopeSetPieces2&&ArrayIndex<SlopeSetPieces2+10)caster=&slopes[ArrayIndex-SlopeSetPieces2];
 else if(ShadowCaster)for(int first:ShadowTileSets)if(unsigned(ArrayIndex-first)<40u){caster=&cliffs[ArrayIndex-first];break;}
 if(!caster||!caster->frame||caster->index!=sub)return;
 try{
  auto* image=static_cast<SHPStruct*>(FileSystem::LoadFile("C_SHADOW.SHP",true));
  const auto bounds=frame->bounds.Width>0?frame->bounds:TacticalClass::ViewBounds;
  point.X+=bounds.X+30+caster->x-clip.X;point.Y+=bounds.Y+15+caster->y-clip.Y;
  auto* sprite=game::append_world_sprite(*frame->world,image,caster->frame-1,nullptr,nullptr,point,&FileSystem::ISOx_PAL,0,true,true);
  if(!sprite){if(!image)game::record_tactical_drawing(game::DrawingStatus::unavailable);return;}
  sprite->original_depth=true;sprite->flags=0x4601;sprite->gradient=0;sprite->depth_adjustment=depth;
  sprite->cell_tint=false;sprite->intensity=1000;
 }catch(...){game::record_tactical_drawing(game::DrawingStatus::backend_failure);}
}
