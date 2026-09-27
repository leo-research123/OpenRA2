// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 alphashp.cpp Draw_All; YR 0x00421350.
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "tactical_drawing.hpp"
void YRPP_FASTCALL AlphaShapeClass::DrawAll(const RectangleStruct& clip){
 auto* frame=game::tactical_drawing();if(!frame||!TacticalClass::Instance)return;
 try{const auto camera=TacticalClass::Instance->TacticalPos;
 for(auto* shape:Array){
  auto rect=shape->Rect;rect.X+=frame->bounds.X-camera.X;rect.Y+=frame->bounds.Y-camera.Y;
  const auto intersection=Drawing::Intersect(rect,clip);
  if(intersection.Width<=0||intersection.Height<=0)continue;
  if(!shape->AlphaImage&&shape->AttachedTo){auto* type=shape->AttachedTo->GetType();shape->AlphaImage=type?type->AlphaImage:nullptr;}
  const auto status=game::draw_lighting_shape(shape->AlphaImage,0,{rect.X,rect.Y},clip,game::RasterBlendMode::alpha_shape);
  game::record_tactical_drawing(status);
  if(status!=game::DrawingStatus::drawn&&status!=game::DrawingStatus::skipped)return;
 }
 }catch(...){game::record_tactical_drawing(game::DrawingStatus::backend_failure);}
}
