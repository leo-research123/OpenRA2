// Copyright 2025 Electronic Arts Inc.
// Copyright 2026 OpenTS contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 additional terms and warranty disclaimers apply;
// see code/third_party/opents/LICENSE.md. Modified by RedAlert2Open, 2026.
// Adapted from OpenTS building.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR calibration: 0x0043D290 body/lower factory pass; 0x0043DA80
// factory exiting object, upper door and SHP/VXL turret/barrel ordering.
#include "yrpp/BuildingClass.h"
#include "yrpp/TacticalClass.h"
#include "building_drawing.hpp"
#include "techno_drawing.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/AnimClass.h"
#include "Matrix3DArithmetic.hpp"
#include <algorithm>
#include <cmath>
namespace {
SHPStruct* data(SHPStruct* image){
 if(!image)return nullptr;
 if(auto*reference=image->AsReference()){reference->Load();return reference->Data;}
 return image;
}
bool complete(game::DrawingStatus status){return status==game::DrawingStatus::drawn||status==game::DrawingStatus::skipped;}
int progress_frame(TransitionTimer& timer,int stages){
 const double value=timer.PercentageDone(),nearest=value*stages;
 const double residual=std::fma(value,double(stages),-nearest);
 const double product=(nearest>0&&residual<0)||(nearest<0&&residual>0)?std::nextafter(nearest,0.0):nearest;
 if(!std::isfinite(product)||product<-2147483648.0||product>=2147483648.0)return 0;
 return static_cast<int>(product);
}
bool roof_exit(const BuildingClass& building){
 const auto*link=building.RadioLinks.Capacity>0?building.RadioLinks[0]:nullptr;
 if(!link)return false;
 const auto*type=static_cast<const TechnoTypeClass*>(link->GetType());
 return type&&(type->JumpJet||type->ConsideredAircraft);
}
bool part(const BuildingClass& building,SHPStruct* image,int index,Point2D point,
        const RectangleStruct& clip,int z,int gradient,int intensity,bool write_depth,
        SHPStruct* depth=nullptr,Point2D depth_offset={}){
 auto*scope=game::building_drawing();
 if(!image||!complete(scope->status))return complete(scope->status);
 if(auto* drawing=game::techno_drawing()){
  auto bounds=clip;
  unsigned tint=building.Airstrike?drawing->laser_tint:0;
  if(building.IsIronCurtained()&&building.ForceShielded==1)tint|=drawing->shield_tint;
  const_cast<BuildingClass&>(building).DrawObject(image,index,&point,&bounds,0,256,z,ZGradient(gradient),write_depth,intensity,int(tint),depth,0,depth_offset.X,depth_offset.Y,0);
  scope->status=drawing->status;return complete(scope->status);
 }
 game::ShapeDrawingRequest r;r.target=scope->context.target;r.palette=scope->context.palette;
 r.image=image;r.frame=index;r.position=point;r.clip=clip;r.flags=write_depth?0x6E00:0x2E00;
    r.gradient=gradient;r.depth_adjustment=z-2;r.intensity=intensity;r.depth_image=depth;r.depth_offset=depth_offset;
 auto status=game::submit_type_shape(scope->context,r);
 if(status!=game::DrawingStatus::skipped)scope->status=status;
 if(!complete(status)||building.Type->NoShadow)return complete(status);
 auto*shape=data(image);
 if(!shape){scope->status=game::DrawingStatus::unavailable;return false;}
 r.frame+=shape->Frames/2;r.flags|=1;r.gradient=0;r.depth_adjustment=-TacticalClass::AdjustForZ(building.Location.Z)-4;
 r.intensity=1000;r.depth_image=nullptr;r.depth_offset={};
 status=game::submit_type_shape(scope->context,r);if(status!=game::DrawingStatus::skipped)scope->status=status;
 return complete(status);
}
}
void BuildingClass::DrawIt(Point2D* point,RectangleStruct* clip) const{
 auto*scope=game::building_drawing();if(!scope)return;
 if(!point||!clip){scope->status=game::DrawingStatus::invalid_argument;return;}
 if(!Type||Type->InvisibleInGame)return;
 auto*image=GetImage();if(!image)return;
 if(!BState&&GetCurrentMission()==Mission::Selling)for(auto* anim:Anims)if(anim)anim->Invisible=true;
 const auto mission=GetCurrentMission();const int height=TacticalClass::AdjustForZ(Location.Z);
 auto&timer=const_cast<TransitionTimer&>(UnloadTimer);
 if(mission==Mission::Open&&!timer.AreStates00()){
  int index=progress_frame(timer,Type->GateStages);
  if(timer.AreStates10())index=Type->GateStages-index;
  if(timer.AreStates00())index=0;
  if(timer.AreStates01())index=Type->GateStages-1;
  index=std::max(0,std::min(index,Type->GateStages-1));
  if(!IsGreenHP())index+=Type->GateStages+1;
  part(*this,image,index,*point,*clip,Type->NormalZAdjust-height,2,scope->intensity,true);return;
 }
 const bool roof=mission==Mission::Unload&&roof_exit(*this);
 int z=Type->NormalZAdjust;
 if(mission==Mission::Unload){auto*replacement=roof?Type->RoofDeployingAnim:Type->DeployingAnim;if(replacement){image=replacement;z=roof?-40:-20;}}
 auto*shape=data(image);if(!shape){scope->status=game::DrawingStatus::unavailable;return;}
 const int index=std::min(const_cast<BuildingClass*>(this)->GetCurrentFrame(),int(shape->Frames)/2);
 const auto extent=TacticalClass::AdjustForZShapeMove(Type->GetFoundationWidth()*256-256,Type->GetFoundationHeight(false)*256-256);
 const bool constructing=mission==Mission::Construction||mission==Mission::Selling;
 Point2D offset{198-extent.X,446-extent.Y};if(!constructing){offset.X+=Type->ZShapePointMove.X;offset.Y+=Type->ZShapePointMove.Y;}
 const int intensity=(game::techno_drawing()?scope->intensity:GetFlashingIntensity(scope->intensity))+static_cast<short>(Type->ExtraLight);
 if(clip->Height>0&&!part(*this,image,index,*point,*clip,z-height,2,intensity,true,Type->GetFoundationWidth()<8?scope->depth_image:nullptr,offset))return;
 if(BState!=static_cast<int>(BStateType::Construction))if(!part(*this,Type->BibShape,const_cast<BuildingClass*>(this)->GetCurrentFrame(),*point,*clip,-height-1,0,intensity,true))return;
 if(mission==Mission::Unload)part(*this,roof?Type->UnderRoofDoorAnim:Type->UnderDoorAnim,IsGreenHP()?0:1,*point,*clip,-height,0,intensity,true);
}
void BuildingClass::Draw(const Point2D& point,const RectangleStruct& clip){
 auto*scope=game::building_drawing();if(!Type||IsFogged||Type->InvisibleInGame)return;
 auto* d=game::techno_drawing();
 if(d&&CurrentMission==Mission::Unload&&IsTether&&RadioLinks.Capacity){
  auto* object=RadioLinks[0];
  if(object&&!object->InLimbo&&object->WhatAmI()!=AbstractType::Building){
   CoordStruct destination;object->GetDestination(&destination,nullptr);destination.Z=object->Location.Z;
   if(!d->window_active||d->debug_map||!d->fog_of_war||!d->fogged||
      (!d->fogged(d->context,object->Location)&&!d->fogged(d->context,destination))){
    auto at=d->project?d->project(d->context,object->GetRenderCoords()):point;auto bounds=clip;
    object->DrawIt(&at,&bounds);
   }
  }
 }
 if(scope&&GetCurrentMission()==Mission::Unload&&Type->DoorAnim&&Type->DoorStages>0){
 int index=progress_frame(UnloadTimer,Type->DoorStages);
 if(UnloadTimer.AreStates10())index=Type->DoorStages-index;
 if(UnloadTimer.AreStates00())index=0;
 index=std::max(0,std::min(index,Type->DoorStages-1));
 if(!IsGreenHP()&&Type->DamagedDoor)index+=Type->DoorStages;
 part(*this,Type->DoorAnim,index,point,clip,-5-TacticalClass::AdjustForZ(Location.Z),0,scope->intensity,false);
 if(UnloadTimer.AreStates10()&&!index)NeedsRedraw=true;
 }
 if(!d||!game::techno_drawing_complete(d->status))return;
 if(!Type->TurretAnimIsVoxel&&!Type->BarrelAnimIsVoxel)return;
 if(!Type->HasSpotlight||!Type->TurretAnimIsVoxel){
  if((CurrentMission==Mission::Construction||QueuedMission==Mission::Construction)&&Animation.Value<Type->BuildingAnimFrame[0].dwUnknown+Type->BuildingAnimFrame[0].FrameCount-1)return;
  if(CurrentMission==Mission::Selling&&Animation.Value>0)return;
 }
 const auto ready=[](const VoxelStruct& v){return v.VXL&&!v.VXL->Initialized&&v.VXL->CountHeaders&&v.VXL->BodyData&&v.HVA&&!v.HVA->LoadedFailed&&v.HVA->Matrixes&&v.HVA->FrameCount>0;};
 const auto rotation=[](unsigned axis,float angle){Matrix3D m;m.MakeIdentity();const float cosine=float(Math::cos(angle)),sine=float(Math::sin(angle));const int x=(axis+1)%3,y=(axis+2)%3;m.row[x][x]=cosine;m.row[x][y]=-sine;m.row[y][x]=sine;m.row[y][y]=cosine;return m;};
 const auto yaw=[](unsigned raw){const auto direction=((raw>>10)+1u)/2u&31u;return matrix_store_float((8-int(direction))*0.19634954084936207);};
 const auto& cfg=Type->BuildingAnim[9];const Point2D at{point.X+cfg.Position.X,point.Y+cfg.Position.Y};
 const int light=(scope?scope->intensity:1000)+static_cast<short>(Type->ExtraLight);
 const auto draw=[&](VoxelStruct& v,const Matrix3D& local,unsigned frame,IndexClass<VoxelIndexKey,VoxelCacheStruct*>& cache){
  if(!ready(v)||!game::techno_drawing_complete(d->status))return;
  const auto matrix=drawing_matrix_product(d->camera,local);
  game::VoxelMatrixScope scope(*d,local);
  DrawVoxel(v,frame%unsigned(v.HVA->FrameCount),-1,cache,clip,at,matrix,light,0,0);
 };
 auto z=rotation(2,yaw(PrimaryFacing.Current().Raw));const auto y=rotation(1,-yaw(BarrelFacing.Current().Raw));
 if(Type->TurretAnimIsVoxel){
  if(ready(Type->TurretVoxel)){
   drawing_translate_axis(z,0,matrix_store_float(Type->TurretOffset/8));auto barrel=z;
   const Vector3D<float> pivot{z.row[0][3],z.row[1][3],z.row[2][3]};drawing_translate(barrel,{-pivot.X,-pivot.Y,-pivot.Z});
   if(TurretRecoil.State!=RecoilData::RecoilState::Inactive)drawing_translate_axis(z,0,-TurretRecoil.TravelSoFar);
   if(BarrelRecoil.State!=RecoilData::RecoilState::Inactive)drawing_translate_axis(barrel,0,-BarrelRecoil.TravelSoFar);
   barrel=drawing_matrix_product(barrel,y);drawing_translate(barrel,pivot);
   const unsigned quadrant=(((unsigned(PrimaryFacing.Current().Raw)>>13)+1)/2)&3;
   if(quadrant==0||quadrant==3)draw(Type->BarrelVoxel,barrel,0,Type->VoxelTurretBarrelCache);
   draw(Type->TurretVoxel,z,unsigned(TurretAnimFrame),Type->VoxelTurretWeaponCache);
   if(quadrant==1||quadrant==2)draw(Type->BarrelVoxel,barrel,0,Type->VoxelTurretBarrelCache);
  }else draw(Type->BarrelVoxel,drawing_matrix_product(z,y),unsigned(TurretAnimFrame),Type->VoxelTurretBarrelCache);
 }else{
  const unsigned facing=((unsigned(PrimaryFacing.Current().Raw)>>10)+1u)/2u&31u;
  const bool after=((facing+28)%32)<=16;
  const auto animation=[&]{if(auto* anim=Anims[9];anim&&d->animation){anim->Invisible=false;const auto status=d->animation(d->context,*this,*anim,point,clip);anim->Invisible=true;game::record_techno_drawing(*d,status);}};
  Matrix3D matrix;GetVoxelBarrelOffsetMatrix(matrix);
  if(after)animation();draw(Type->BarrelVoxel,matrix,0,Type->VoxelTurretBarrelCache);if(!after)animation();
 }
}

int BuildingClass::GetZAdjustment() const {
 const int z=-TacticalClass::AdjustForZ(GetZ());
 return Type&&Type->TurretAnimIsVoxel?z+Type->BuildingAnim[9].ZAdjust:z;
}
