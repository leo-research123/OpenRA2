// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors; adaptations Copyright 2026 RedAlert2Open.
// OpenTS ovrlight.cpp/xsurface.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR 0x005FF850 pool lookup, clipping and RGB565 destination brightening.
#include "yrpp/SpotlightClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "type_drawing.hpp"
#include <array>
#include <algorithm>
namespace {
DynamicVectorClass<SpotlightClass*> lights;
void circle(std::array<std::uint16_t,256*128>& pixels,int radius,int color){
 if(radius<0)return;
 auto row=[&](int y,int x1,int x2){y+=128;if(y<0||y>=255||(y&1))return;
  for(int x=std::max(0,128+x1);x<=std::min(254,128+x2);++x)pixels[(y/2)*256+x]=std::uint16_t(color);
 };
 int x=radius,y=0,d=3-2*radius;
 do{row(y,-x,x);row(x,-y,y);row(-y,-x,x);row(-x,-y,y);
  if(d<0)d+=4*y+6;else{d+=4*(y-x)+10;--x;}++y;
 }while(x>=y);
}
}
DynamicVectorClass<SpotlightClass*>& SpotlightClass::Array=lights;
SpotlightClass::SpotlightClass(CoordStruct coords,int size):Coords(coords),MovementRadius(0),Size(size),DisableFlags(SpotlightFlags::None){Array.AddItem(this);}
SpotlightClass::~SpotlightClass(){Array.Remove(this);}
void SpotlightClass::Update(){MovementRadius+=8;if(MovementRadius>=80)delete this;}
void YRPP_FASTCALL SpotlightClass::DrawAll(){for(auto*light:Array)light->Draw();}
void SpotlightClass::Draw(){
 const auto*context=game::active_type_drawing();auto*tactical=TacticalClass::Instance;
 if(!context||!tactical||!RulesClass::Instance)return;
 if(ScenarioClass::Instance&&ScenarioClass::Instance->SpecialFlags.FogOfWar){auto*cell=MapClass::Instance.TryGetCellAt(Coords);if(!cell||cell->IsFogged())return;}
 Point2D point{};if(!TacticalClass::CoordsToClient(Coords,tactical->TacticalPos,TacticalClass::ViewBounds,point))return;
 static constexpr int indices[90]={5,10,15,20,25,30,35,40,45,50,55,60,61,62,63,63,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,64,65,66,67,68,69,70,71,72,73};
 if(MovementRadius<0||MovementRadius>=90)return;
 int index=indices[MovementRadius];if(index<64)index=index*Size/64;
 if(index<0||index>=74)return;
 std::array<std::uint16_t,256*128> pixels{};
 if(index<64){for(int radius=2*index+1,color=-2;radius>0;radius-=2,color+=4)circle(pixels,radius,std::max(0,color));}
 else {const int extra=index-64;circle(pixels,extra-int((30*RulesClass::Instance->SpotlightRadius)/-358.4),128-extra*6);}
 const auto bounds=TacticalClass::ViewBounds;
 game::RasterDrawingRequest r;r.target=context->target;r.position={point.X+bounds.X-128,point.Y+bounds.Y-64};r.clip=bounds;
 r.width=255;r.height=127;r.pitch=256;r.pixels=pixels.data();r.pixel_count=pixels.size();r.blend_mode=game::RasterBlendMode::spotlight;r.spotlight_flags=unsigned(DisableFlags);
 game::record_type_drawing_result(game::submit_type_raster(*context,r));
}
