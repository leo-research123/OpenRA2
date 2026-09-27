// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 unit.cpp::Draw_It / Unit_Draw_Shape;
// YR 0x0073CEC0 / 0x0073C5F0.
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "Matrix3DArithmetic.hpp"
#include "techno_drawing.hpp"
#include <algorithm>
#include <bit>

void UnitClass::DrawIt(Point2D* location,RectangleStruct* bounds) const {
 auto* d=game::techno_drawing();if(!d)return;
 if(!location||!bounds||!Type||!Locomotor){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
 auto& self=*const_cast<UnitClass*>(this);
 auto point=*location;point.Y-=DrawingYOffset;const auto raw=point;if(IsOnCarryall)point.Y+=14;
 if(VisualCharacter(false,nullptr)==VisualType::Hidden||Deploying||Undeploying)return;
 if(d->selectable)d->selectable(d->context,self,point);
 auto* cell=game::techno_drawing_cell(*d,*this);
 auto* ground=d->cell_at_world?d->cell_at_world(d->context,Location):cell;
 if(!cell||!ground){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
 const int height=GetHeight();const auto flags=unsigned(ground->Flags);
 bool bridge=false;
 if((!OnBridge&&height>=d->bridge_height)||(OnBridge&&height>=0))if(flags&0x100u){
  const auto* adjacent=ground->GetNeighbourCell((flags&0x800u)?FacingType::North:FacingType::West);
  bridge=adjacent&&(unsigned(adjacent->Flags)&0x100u);
 }
 const int intensity=std::bit_cast<short>(cell->Intensity_Normal)+(bridge?4*d->level_light:(unsigned(cell->Flags)&0x10000u)?-500:0)+d->extra_unit_light;
 if(Type->Harvester&&IsHarvesting&&!Locomotor->Is_Moving_Now()&&!IsBeingWarpedOut()&&!IsWarpingIn()){
  auto* image=d->load_shape?d->load_shape(d->context,"OREGATH.SHP"):nullptr;
  if(image){const auto facing=PrimaryFacing.Current().GetValue<3>();
   const double angle=(std::bit_cast<short>(std::uint16_t(facing<<13))-0x3FFF)*-0.00009587672516830327;
   auto at=GetRenderCoords();at.Y=int(double(at.Y)-Math::sin(angle)*30.0);at.X=int(Math::cos(angle)*30.0+at.X);
   game::ShapeDrawingRequest r;r.image=image;r.frame=15*(7-facing)+(d->frame+WalkedFramesSoFar)%15;
   r.position=d->project?d->project(d->context,at):point;r.clip=*bounds;r.flags=0x2A00;r.gradient=0;r.depth_adjustment=GetZAdjustment()-2;r.intensity=intensity;
   if(!game::techno_submit_shape(*d,*this,game::TechnoPalette::animation,cell,nullptr,r))return;
  }
 }
 auto* original=Type;
 struct Restore {UnitClass& object;UnitTypeClass* type;~Restore(){object.Type=type;}} restore{self,original};
 if(Unloading&&Type->Harvester&&Type->UnloadingClass)self.Type=Type->UnloadingClass;
 ObjectTypeClass* apparent=self.Type;
 if(!IsClearlyVisibleTo(d->player))if(auto* disguise=GetDisguise(true))apparent=disguise;
 if(TubeIndex==-1){
  if(apparent->WhatAmI()==AbstractType::TerrainType){
   self.Draw_A_SHP(apparent->GetImage(),0,&point,bounds,0,256,0,ZGradient::Deg90,0,intensity,0,nullptr,0,0,0,0);
  }else if(apparent->Voxel){self.DrawAsVXL(point,*bounds,intensity,0);}
  else self.DrawAsSHP(point,*bounds,intensity,0);
 }
 self.Type=original;
 if(!game::techno_drawing_complete(d->status))return;
 if(FlagHouseIndex!=-1){
  auto* image=d->load_shape?d->load_shape(d->context,"FLAGFLY.SHP"):nullptr;
  auto* owner=HouseClass::Array.GetItemOrDefault(FlagHouseIndex);
  if(image&&owner){game::ShapeDrawingRequest r;r.image=image;r.frame=d->frame%14;r.position=raw;r.clip=*bounds;r.flags=0xA00;r.gradient=0;r.intensity=intensity;
   game::techno_submit_shape(*d,*this,game::TechnoPalette::house,cell,owner,r);
  }
 }
}

void UnitClass::DrawAsSHP(Point2D point,RectangleStruct clip,int intensity,int tint) {
 auto* d=game::techno_drawing();if(!d||!Type)return;
 auto* image=GetImage();if(!image)return;
 int facing=0;
 if(Type->Facings==8&&!IsDisguised())facing=(int(PrimaryFacing.Current().GetValue<3>())+1)&7;
 int frame=facing;
 if(CurrentFiringFrame>=0)frame=Type->StartFiringFrame+CurrentFiringFrame/2+facing*int(Type->FiringFrames);
 else if((Locomotor&&Locomotor->Is_Moving())||Type->IdleRate){
  if(Type->WalkFrames<=0){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
  frame=Type->StartWalkFrame+facing*int(Type->WalkFrames)+WalkedFramesSoFar%int(Type->WalkFrames);
 }else if(DeathFrameCounter>=0){
  if(Type->DeathFrameRate<=0){game::record_techno_drawing(*d,game::DrawingStatus::invalid_argument);return;}
  frame=Type->StartDeathFrame+std::min(DeathFrameCounter/Type->DeathFrameRate,Type->DeathFrames-1);
 }else if(FrozenStill)frame=Type->StandingFrames?Type->StartStandFrame+facing*Type->StandingFrames:Type->StartWalkFrame+facing*int(Type->WalkFrames);
 if(!Type->Turret){
  Draw_A_SHP(image,frame,&point,&clip,0,256,0,GetZGradient(),0,intensity,unsigned(tint),nullptr,0,0,0,0);return;
 }
 // Native hosts compose the ordered parts at their final position instead
 // of using the EXE's shared 256x256 EightBitSurface. Frame and matrix choices
 // belong here; the host consumes the resulting requests synchronously.
 auto* shape=d->shape_data?d->shape_data(image):image;
 if(!shape){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
 game::ShapeDrawingRequest shadow;shadow.image=image;shadow.frame=frame+shape->Frames/2;
 shadow.position=point;if(IsOnCarryall)shadow.position.Y-=14;
 shadow.clip=clip;shadow.flags=0x2E01;shadow.depth_adjustment=GetZAdjustment()-2;shadow.intensity=1000;
 if(!game::techno_submit_shape(*d,*this,game::TechnoPalette::normal,game::techno_drawing_cell(*d,*this),nullptr,shadow))return;
 const bool shadows=d->draw_shadows;
 struct Restore {game::TechnoDrawing& d;bool shadows;~Restore(){d.draw_shadows=shadows;}} restore{*d,shadows};
 d->draw_shadows=false;
 auto barrel=Matrix3D::GetIdentity();
 const bool has_barrel=Type->BarrelVoxel.VXL&&Type->BarrelVoxel.HVA;
 bool above=true;
 if(has_barrel){
  VoxelIndexKey key(-1);auto body=Locomotor->Draw_Matrix(&key);
  drawing_translate_axis(body,0,matrix_store_float(Type->TurretOffset/8));
  const double secondary=(int(SecondaryFacing.Current().GetValue<5>())-8)*-0.1963495408493621;
  const double primary=(int(PrimaryFacing.Current().GetValue<5>())-8)*-0.1963495408493621;
  body.RotateZ(matrix_store_float(secondary-primary));barrel=body;
  const Vector3D<float> pivot{body.row[0][3],body.row[1][3],body.row[2][3]};drawing_translate(barrel,{-pivot.X,-pivot.Y,-pivot.Z});
  auto* weapon=GetWeapon(0);
  if(!weapon||!weapon->WeaponType||!weapon->WeaponType->Projectile){game::record_techno_drawing(*d,game::DrawingStatus::unavailable);return;}
  const Vector3D<float> flh{float(weapon->FLH.X/-8),0,float(weapon->FLH.Z/-8)};
  drawing_translate(barrel,{-flh.X,0,-flh.Z});auto facing=BarrelFacing.Current();
  if(weapon->WeaponType->Projectile->Inviso){int direction=facing.GetValue<8>();direction=direction>64&&direction<=128?(direction-64)/3+64:(64-direction)/3+64;facing.Raw=WORD(direction<<8);}
  if(BarrelRecoil.State!=RecoilData::RecoilState::Inactive)drawing_translate_axis(barrel,0,-BarrelRecoil.TravelSoFar);
  barrel.RotateY(matrix_store_float(-(int(facing.GetValue<5>())-8)*-0.1963495408493621));drawing_translate(barrel,flh);drawing_translate(barrel,pivot);
  const auto quadrant=SecondaryFacing.Current().GetValue<2>();above=quadrant==1||quadrant==2;
 }
 const auto draw_barrel=[&]{game::VoxelMatrixScope scope(*d,barrel);auto matrix=drawing_matrix_product(d->camera,barrel);
  Draw_A_VXL(&Type->BarrelVoxel,0,-1,&Type->VoxelTurretBarrelCache,&clip,&point,&matrix,intensity,BlitterFlags(0),unsigned(tint));};
 Draw_A_SHP(image,frame,&point,&clip,0,256,0,ZGradient::Ground,0,intensity,unsigned(tint),nullptr,0,0,0,0);
 if(has_barrel&&!above)draw_barrel();
 const int turret=8*int(Type->WalkFrames)+((int(SecondaryFacing.Current().GetValue<5>())+4)&31);
 Draw_A_SHP(image,turret,&point,&clip,0,256,0,ZGradient::Ground,0,intensity,unsigned(tint),nullptr,0,0,0,0);
 if(has_barrel&&above)draw_barrel();
}
