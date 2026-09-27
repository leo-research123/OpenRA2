// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::Shape_Number; normal YR 0x00518D80 branch.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LocomotionClass.h"
#include "techno_drawing.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
int InfantryClass::GetCurrentFrame() const {
 if(!Type)return -1;
 int action=static_cast<int>(SequenceAnim);
 if(action==-1){auto*cell=GetCell();action=cell&&cell->LandType==LandType::Water&&!OnBridge?16:0;}
 if(action<0||action>=42)return -1;
 auto* apparent=Type;
 auto* drawing=game::techno_drawing();
 if(!IsClearlyVisibleTo(drawing?drawing->player:HouseClass::CurrentPlayer)){
  auto* disguise=GetDisguise(true);
  if(disguise&&disguise->WhatAmI()==AbstractType::InfantryType)apparent=static_cast<InfantryTypeClass*>(disguise);
 }
 if(!apparent->Sequence)return -1;
 const auto& sequence=apparent->Sequence->Sequences[action];
 constexpr unsigned human[]{7,7,6,6,6,6,5,5,5,5,4,4,4,4,3,3,3,3,2,2,2,2,1,1,1,1,0,0,0,0,7,7};
 std::int64_t frame=Animation.Value%std::max(sequence.CountFrames,1);
 auto direction=PrimaryFacing.Current();
 if(Type->JumpJet&&Locomotor){
  constexpr GUID iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
  IPersist* persist=nullptr;
  const auto result=Locomotor->QueryInterface(iid,reinterpret_cast<void**>(&persist));
  if(result<0||!persist)return -1;
  CLSID clsid{};persist->GetClassID(&clsid);persist->Release();
  if(!std::memcmp(&clsid,&LocomotionClass::CLSIDs::Jumpjet,sizeof(clsid))&&Target&&!Type->JumpJetTurn)
   GetDirectionTo(&direction,Target);
 }
 if(sequence.FacingMultiplier>0)frame+=std::int64_t(human[direction.GetFacing<32>()])*sequence.FacingMultiplier;
 frame+=sequence.StartFrame;
 return frame>=0 && frame<=INT32_MAX?static_cast<int>(frame):-1;
}

// OpenTS 44fac744 Draw_It, calibrated to YR 0x00518F90. The airborne
// shadow is INFSHDW frame 1 in YR, not another copy of the infantry SHP.
void InfantryClass::DrawIt(Point2D* location,RectangleStruct* bounds) const {
 auto* d=game::techno_drawing();if(!d)return;
 if(!location||!bounds||!Type||!d->height){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
 auto& self=*const_cast<InfantryClass*>(this);auto point=*location;
 auto* cell=game::techno_drawing_cell(*d,*this);
 if(!cell){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
 unsigned tint=Berzerk?d->berserk_tint:0;if(d->shrouded&&d->shrouded(*cell))tint=0;
 if(TubeIndex!=-1){if(d->selectable)d->selectable(d->context,self,point);return;}
 if(!Locomotor){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
 constexpr GUID iid{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
 struct Held { IPersist* pointer=nullptr; ~Held(){if(pointer)pointer->Release();} } persist;
 const auto result=Locomotor->QueryInterface(iid,reinterpret_cast<void**>(&persist.pointer));
 if(result<0||!persist.pointer){game::record_techno_drawing(*d,game::DrawingStatus::backend_failure);return;}
 CLSID clsid{};persist.pointer->GetClassID(&clsid);
 if(GetHeight()>0&&!std::memcmp(&clsid,&LocomotionClass::CLSIDs::Droppod,sizeof(clsid))){
  auto* image=d->load_shape?d->load_shape(d->context,"POD.SHP"):nullptr;
  if(!image){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
  const auto delta=Locomotor->Shadow_Point();
  game::ShapeDrawingRequest r;r.image=image;r.frame=Locomotor->Drawing_Code();r.position={point.X+delta.X,point.Y+delta.Y};r.clip=*bounds;
  r.flags=0x2E01;r.depth_adjustment=GetZAdjustment()-2;r.gradient=0;r.intensity=1000;r.tint=tint;
  if(!game::techno_submit_shape(*d,*this,game::TechnoPalette::normal,cell,nullptr,r))return;
  self.Draw_A_SHP(image,Locomotor->Drawing_Code(),location,bounds,0,256,0,ZGradient::Deg90,0,1000,tint,nullptr,0,0,0,0);return;
 }
 auto* image=GetImage();if(!image)return;
 if(d->selectable)d->selectable(d->context,self,point);
 auto* ground=d->cell_at_world?d->cell_at_world(d->context,Location):cell;
 if(!ground){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
 const bool overshadowed=(unsigned(ground->Flags)&0x10000u)!=0;
 const int floor=d->ground_height?d->ground_height(d->context,Location):Location.Z-GetHeight();
 int intensity=std::bit_cast<short>(cell->Intensity_Normal);
 if(OnBridge||(overshadowed&&Location.Z>floor+d->bridge_height/2))intensity+=d->level_light*(GetHeight()/(2*d->level_height));
 else if(overshadowed)intensity-=500;
 intensity+=d->extra_infantry_light;
 int height=GetHeight();const auto flags=unsigned(ground->Flags);
 if((flags&0x100u)&&height>=d->bridge_height){
  const auto* neighbor=ground->GetNeighbourCell((flags&0x800u)?FacingType::North:FacingType::West);
  if(neighbor&&(unsigned(neighbor->Flags)&0x100u))height-=d->bridge_height;
 }
 if(height>0){
  auto* shadow=d->load_shape?d->load_shape(d->context,"INFSHDW.SHP"):nullptr;
  if(shadow){game::ShapeDrawingRequest r;r.image=shadow;r.frame=1;r.position={point.X,point.Y-DrawingYOffset+d->height(height)};r.clip=*bounds;
   r.flags=0x2601;r.depth_adjustment=-5-d->height(Location.Z-height);r.gradient=0;r.intensity=1000;
   if(!game::techno_submit_shape(*d,*this,game::TechnoPalette::normal,cell,nullptr,r))return;
  }
 }
 point.Y-=DrawingYOffset;
 const int frame=GetCurrentFrame();if(frame<0)return;
 self.Draw_A_SHP(image,frame,&point,bounds,0,256,DWORD(-10),ZGradient::Deg90,0,intensity,tint,nullptr,0,0,0,0);
}
