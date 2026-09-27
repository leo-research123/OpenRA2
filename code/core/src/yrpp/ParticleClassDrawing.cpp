// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 particle.cpp::Draw_It.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x62CEC0 spark/railgun pixel branch; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "type_drawing.hpp"
#include "RulesClassReaders.hpp"

void ParticleClass::DrawIt(Point2D* position,RectangleStruct* bounds) const {
 const auto* context=game::active_type_drawing();if(!context||!Type||!bounds)return;
 if(!GameOptionsClass::Instance.DetailLevel&&(int(Type->BehavesLike)==1||int(Type->BehavesLike)==3))return;
 if(ScenarioClass::Instance->SpecialFlags.FogOfWar&&GetCell()->IsFogged())return;
 if(int(Type->BehavesLike)==1){
  auto* image=GetImage();if(!image||!position)return;
  // 0x62CFB0: ANIM.PAL, absolute state frame, 50% blend at high detail.
  game::ShapeDrawingRequest r;r.target=context->target;r.palette=context->palette;
  r.image=image;r.frame=static_cast<signed char>(StartStateAI);r.position=*position;r.clip=*bounds;
  r.flags=0x2E00;r.gradient=2;r.intensity=1000;
  if(GameOptionsClass::Instance.DetailLevel==2){
   if(Translucency==25)r.flags|=2;else if(Translucency==50)r.flags|=4;else if(Translucency>=75)r.flags|=6;
  }
  r.depth_adjustment=-15-TacticalClass::AdjustForZ(GetHeight());
  game::record_type_drawing_result(game::submit_type_shape(*context,r));return;
 }
 if(int(Type->BehavesLike)!=3&&int(Type->BehavesLike)!=4){game::record_type_drawing_result(game::DrawingStatus::unsupported);return;}
 if(ColorIndex<0||ColorIndex+1>=Type->ColorList.Count){game::record_type_drawing_result(game::DrawingStatus::invalid_argument);return;}
 Point2D point{};if(!TacticalClass::CoordsToClient(Location,TacticalClass::Instance->TacticalPos,*bounds,point))return;
 const auto a=ColorIndex?Type->ColorList[ColorIndex]:Color,b=Type->ColorList[ColorIndex+1];
 const int red=rule_integer(a.Red*(1.0-ColorAccum)+b.Red*ColorAccum);
 const int green=rule_integer(a.Green*(1.0-ColorAccum)+b.Green*ColorAccum);
 const int blue=rule_integer(a.Blue*(1.0-ColorAccum)+b.Blue*ColorAccum);
 game::RasterDrawingRequest r;r.target=context->target;r.position={point.X+bounds->X,point.Y+bounds->Y};r.clip=*bounds;
 r.width=r.height=1;r.blend_mode=game::RasterBlendMode::particle;
 r.line_rgb=unsigned(red)|(unsigned(green)<<8)|(unsigned(blue)<<16);
 r.line_z=-TacticalClass::AdjustForZ(Location.Z)-50;
 game::record_type_drawing_result(game::submit_type_raster(*context,r));
}
