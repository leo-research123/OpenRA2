// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp; Copyright 2025 Electronic Arts Inc.;
// Copyright 2026 OpenTS contributors; EA terms: third_party/opents/LICENSE.md.
// Original TechnoClass::DrawExtras 0x006F5190, building and ordinary infantry display.
// Wrench uses its own animation palette and remains visible when unselected.
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/StringTable.h"
#include "yrpp/Drawing.h"
#include "building_selection.hpp"
#include "type_drawing.hpp"
#include <algorithm>
#include <cwchar>
void TechnoClass::DrawExtras(Point2D* point,RectangleStruct* bounds) const {
 auto* frame=game::building_health_drawing();if(!frame)return;
 if(!point||!bounds){frame->status=game::DrawingStatus::invalid_argument;return;}
 if(WhatAmI()!=AbstractType::Building&&WhatAmI()!=AbstractType::Infantry&&WhatAmI()!=AbstractType::Unit&&WhatAmI()!=AbstractType::Aircraft){frame->status=game::DrawingStatus::unsupported;return;}
 if(IsSinking)return;
 const auto record=[&](game::DrawingStatus status){
  if(status!=game::DrawingStatus::skipped)frame->status=status;
  return status==game::DrawingStatus::drawn||status==game::DrawingStatus::skipped;
 };
 if(WhatAmI()==AbstractType::Building){
 const auto& building=*static_cast<const BuildingClass*>(this);
 const auto* cell=MapClass::Instance.TryGetCellAt(Location);
 if(building.IsBeingRepaired&&cell&&!cell->IsShrouded()){
  if(!frame->wrench||!frame->wrench_palette){frame->status=game::DrawingStatus::unavailable;return;}
  const int period=std::max(2,GameOptionsClass::Instance.GetAnimSpeed(14)/4);
  auto position=TacticalClass::CoordsToScreen(GetRenderCoords());position.X-=frame->camera.X;position.Y-=frame->camera.Y;
  game::ShapeDrawingRequest r;r.target=frame->drawing.target;r.palette=frame->wrench_palette;r.image=frame->wrench;
  r.position=position;r.frame=6*(Unsorted::CurrentFrame%period)/(period-1);r.flags=0xE00;r.clip=*bounds;
  if(!record(game::submit_type_shape(frame->drawing,r)))return;
 }

 }
 // 0x6F5372: rank insignia is visible even without selection or hover.
 if(!IsDisguisedAs(HouseClass::CurrentPlayer)&&VisualCharacter(false,nullptr)!=VisualType::Hidden)
  DrawVeterancyPips(point,bounds);
 if(!game::drawing_completed(frame->status))return;
 if(WhatAmI()==AbstractType::Building){
  const auto& building=*static_cast<const BuildingClass*>(this);
 if(IsSelected&&frame->selection_palette){
  game::BuildingSelectionGeometry geometry;
  if(game::building_selection_geometry(building,geometry))if(!record(game::draw_building_selection(frame->drawing,*bounds,frame->camera,geometry,*frame->selection_palette)))return;
 }
 }
 if(!frame->pips)return;
 if(IsSelected)DrawHealthBar(point,bounds,false);
 // 0x6F5E37: hover is a frame-local original object flag. The disguise
 // branch queries GetDisguiseHouse (vtable 0xD0), not GetDisguise (0xCC).
 else if(IsMouseHovering&&(!IsDisguisedAs(HouseClass::CurrentPlayer)||GetDisguiseHouse(true)))
  DrawHealthBar(point,bounds,true);
 // 跳过原 0x6F5E8D 的对话气泡：已核查的 YR 本体地图及 AI 配置无调用。
}

// YR 0x70A990: PIPS.SHP veteran/elite/negative-rank markers.
void TechnoClass::DrawVeterancyPips(Point2D* point,RectangleStruct* bounds) const {
 auto* frame=game::building_health_drawing();if(!frame||!point||!bounds)return;
 int index=Veterancy.IsElite()?15:Veterancy.IsVeteran()?14:Veterancy.IsNegative()?19:-1;
 if(index<0)return;
 if(!frame->pips||!frame->palette){frame->status=game::DrawingStatus::unavailable;return;}
 game::ShapeDrawingRequest request;request.target=frame->drawing.target;request.palette=frame->palette;
 request.image=frame->pips;request.frame=index;request.position={point->X+5,point->Y+2};
 if(WhatAmI()!=AbstractType::Infantry){request.position.X+=5;request.position.Y+=4;}
 request.clip=*bounds;request.flags=0xE00;request.depth_adjustment=-2;request.intensity=1000;
 const auto status=game::submit_type_shape(frame->drawing,request);if(status!=game::DrawingStatus::skipped)frame->status=status;
}

// OpenTS 44fac744 techno.cpp::Draw_Text_Overlay, calibrated to 0x70AA60.
// YR suppresses power text on primary factories and uses an owner-colored box.
// The native host submits the original BitFont glyphs through its own backend;
// the DDraw text wrapper (0x4A59E0 / 0x4A5EB0) is not replaced by this adapter.
void TechnoClass::DrawExtraInfo(const Point2D&,const Point2D& center,const RectangleStruct& bounds) const {
 auto* frame=game::building_health_drawing();if(!frame||!Owner)return;
 const bool building=WhatAmI()==AbstractType::Building;
 const auto* type=building?static_cast<const BuildingClass*>(this)->Type:nullptr;
 const wchar_t* label=nullptr;wchar_t buffer[128]{};
 if(IsPrimaryFactory)label=StringTable::LoadString(type&&type->GetFoundationWidth()==1?"TXT_PRI":"TXT_PRIMARY");
 else if(type&&type->PowerBonus>0){
  const auto* format=StringTable::LoadString("TXT_POWER_DRAIN2");
  if(!format)return;
  std::swprintf(buffer,128,format,int(Owner->Power_Output()),int(Owner->Power_Drain()));label=buffer;
 }
 if(!label||!*label)return;
 auto* font=BitFont::Instance;
 if(!font||!font->InternalPTR){frame->status=game::DrawingStatus::unavailable;return;}
 int width=0,height=0;if(!font->GetTextDimension(label,&width,&height,0))return;
 const RectangleStruct box=Drawing::Intersect({center.X-width/2-4,center.Y-2,width+8,height+4},bounds);
 const auto c=Owner->Color;const WORD color=WORD((c.R>>3)<<11|(c.G>>2)<<5|(c.B>>3));
 const auto fill=[&](RectangleStruct rect,WORD value){
  game::RasterDrawingRequest r;r.target=frame->drawing.target;r.position={rect.X,rect.Y};r.clip=bounds;
  r.width=rect.Width;r.height=rect.Height;r.color=value;
  const auto s=game::submit_type_raster(frame->drawing,r);
  if(s!=game::DrawingStatus::skipped)frame->status=s;
  return s==game::DrawingStatus::drawn||s==game::DrawingStatus::skipped;
 };
 if(box.Width<=0||box.Height<=0)return;
 if(!fill(box,0)||!fill({box.X,box.Y,box.Width,1},color)
   ||!fill({box.X,box.Y+box.Height-1,box.Width,1},color)
   ||!fill({box.X,box.Y,1,box.Height},color)||!fill({box.X+box.Width-1,box.Y,1,box.Height},color))return;
 int x=center.X-width/2;
 for(auto* ch=label;*ch;++ch){int next=x;
  const auto s=font->SubmitGlyph(frame->drawing,*ch,x,center.Y,bounds,color,next);
  if(s!=game::DrawingStatus::skipped)frame->status=s;
  if(s!=game::DrawingStatus::drawn&&s!=game::DrawingStatus::skipped)return;
  x=next;
 }
}
