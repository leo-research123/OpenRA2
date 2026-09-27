// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 bounce.cpp / voxel.cpp; YR 0x004397E0..0x0043A010.
// Copyright 2026 OpenTS contributors. See third_party/opents/LICENSE.md.
#include "yrpp/BounceClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/YRMath.h"
#include "Matrix3DArithmetic.hpp"
#include <cmath>

namespace {
CoordStruct integer_coords(const Vector3D<float>& p){return {int(p.X),int(p.Y),int(p.Z)};}
bool bridge(const CellClass* cell){return cell&&(cell->Flags&CellFlags::Bridge)!=CellFlags{};}
int level(const CellClass* cell){return cell?static_cast<signed char>(cell->Level):0;}
Quaternion multiply(const Quaternion& a,const Quaternion& b){
 return {a.W*b.X+a.X*b.W+a.Y*b.Z-a.Z*b.Y,
         a.W*b.Y-a.X*b.Z+a.Y*b.W+a.Z*b.X,
         a.W*b.Z+a.X*b.Y-a.Y*b.X+a.Z*b.W,
         a.W*b.W-a.X*b.X-a.Y*b.Y-a.Z*b.Z};
}
int cliff_direction(const CellClass& cell,const CellClass& previous){
 // OpenTS CellClass::Bounce_Direction; YR table 0x0081CD4C.
 const int dx=cell.MapCoords.X-previous.MapCoords.X,dy=cell.MapCoords.Y-previous.MapCoords.Y;
 if(dx<-1||dx>1||dy<-1||dy>1)return -1;
 constexpr int directions[]={7,0,1,6,-1,2,5,4,3};
 int direction=directions[4+dx+3*dy];
 if(dx&&dy){auto& map=MapClass::Instance;
  const bool left=level(map.TryGetCellAt(CellStruct{short(previous.MapCoords.X+(dx==dy?dx:dy)),previous.MapCoords.Y}))>=level(&previous)+2;
  const bool right=level(map.TryGetCellAt(CellStruct{previous.MapCoords.X,short(previous.MapCoords.Y+(dx==dy?dy:dx))}))>=level(&previous)+2;
  if(left&&!right)direction=(direction-1)&7;else if(!left&&right)++direction;
 }
 return direction;
}
}

void BounceClass::Initialize(const CoordStruct& coords,double elasticity,double gravity,
        double maxVelocity,const Vector3D<float>& velocity,double angularVelocity){
 Elasticity=elasticity;Gravity=gravity;MaxVelocity=maxVelocity;
 Coords={float(coords.X),float(coords.Y),float(coords.Z)};Velocity=velocity;
 auto& random=ScenarioClass::Instance->Random;
 const int z=random.RandomRanged(-65535,65535),y=random.RandomRanged(-65535,65535),x=random.RandomRanged(-65535,65535);
 Vector3D<float> axis{float(x/65535.0),float(y/65535.0),float(z/65535.0)};
 const double length=std::sqrt(double(axis.X)*axis.X+double(axis.Y)*axis.Y+double(axis.Z)*axis.Z);
 const float sine=float(Math::sin(float(angularVelocity)*0.5));
 AngularVelocity.X=length?float(axis.X/length)*sine:0;
 AngularVelocity.Y=length?float(axis.Y/length)*sine:0;
 AngularVelocity.Z=length?float(axis.Z/length)*sine:0;
 AngularVelocity.W=float(Math::cos(float(angularVelocity)*0.5));
 CurrentAngle.X=CurrentAngle.Y=CurrentAngle.Z=0;CurrentAngle.W=1;
}
CoordStruct* BounceClass::GetCoords(CoordStruct* output) const{*output=integer_coords(Coords);return output;}
Matrix3D* BounceClass::GetDrawingMatrix(Matrix3D* output) const{
 return Matrix3D::FromQuaternion(output,&CurrentAngle);
}
BounceClass::Status BounceClass::Update(){
 auto& map=MapClass::Instance;const auto oldPosition=Coords,oldVelocity=Velocity;
 Velocity.Z=float(Velocity.Z-Gravity);
 // YR 0x00439B9A and OpenTS both scale by speed/speed when over
 // MaxVelocity. Preserve that no-op rather than inventing a speed clamp.
 const auto previous=integer_coords(Coords);Coords+=Velocity;
 auto current=integer_coords(Coords);auto* cell=map.TryGetCellAt(current);
 auto* previousCell=map.TryGetCellAt(previous);
 if(!cell)return Status::Impact;
 const int floor=map.GetCellFloorHeight(current),deck=floor+CellClass::BridgeHeight;
 const bool crossedBridge=bridge(cell)||bridge(previousCell);
 const bool top=crossedBridge&&current.Z<deck&&previous.Z>=deck;
 const bool bottom=crossedBridge&&current.Z>=deck&&previous.Z<deck;
 bool obstacle=false;
 if(!top&&!bottom&&floor<=Coords.Z&&Coords.Z-150.0<floor){
  auto* building=cell->GetBuilding();obstacle=building||cell->ConnectsToOverlay();
  if(building&&((building->Type->LaserFence&&building->LaserFenceFrame>=8)||building->IsStrange()))obstacle=false;
 }
 auto result=Status::None;
 if(Coords.Z<floor||top||bottom||obstacle){
  if(top)Coords.Z=float(deck);else if(bottom)Coords.Z=float(deck-20);else if(floor-100<Coords.Z)Coords.Z=float(floor);
  const auto& slope=Matrix3D::VoxelRampMatrix[cell->SlopeIndex];
  const auto inverse=Matrix3D::TransposeMatrix(slope);
  auto velocity=inverse.RotateVector({Velocity.X,-Velocity.Y,Velocity.Z});
  const float elasticity=matrix_store_float(Elasticity);
  velocity.X=matrix_store_float(double(velocity.X)*elasticity);
  velocity.Y=matrix_store_float(double(velocity.Y)*elasticity);
  velocity.Z=-matrix_store_float(double(velocity.Z)*elasticity);
  velocity=slope.RotateVector(velocity);Velocity={velocity.X,-velocity.Y,velocity.Z};
  AngularVelocity.X=-AngularVelocity.X;AngularVelocity.Y=-AngularVelocity.Y;AngularVelocity.Z=-AngularVelocity.Z;
  if(previousCell&&level(cell)-level(previousCell)>=2&&
      ((oldVelocity.Z<-0.0002&&Coords.Z>oldPosition.Z)||(oldVelocity.Z>=-0.0003&&oldVelocity.Z+oldPosition.Z+1.0<Coords.Z))){
   Coords=oldPosition;Velocity=oldVelocity;
   const int facing=cliff_direction(*cell,*previousCell);
   if(facing!=-1){const float x=Velocity.X,y=Velocity.Y;
    switch(facing&3){case 0:Velocity.Y=-y;break;case 1:Velocity.X=y;Velocity.Y=x;break;
     case 2:Velocity.X=-x;break;case 3:Velocity.X=-y;Velocity.Y=-x;break;}
    Velocity.X=float(Velocity.X*Elasticity);Velocity.Y=float(Velocity.Y*Elasticity);Velocity.Z=float(Velocity.Z*Elasticity);
   }
  }
  result=Status::Bounce;
 }
 auto angle=multiply(CurrentAngle,AngularVelocity);CurrentAngle=angle;
 current=integer_coords(Coords);cell=map.TryGetCellAt(current);
 int height=int(Coords.Z-map.GetCellFloorHeight(current));if(bridge(cell)&&height>=CellClass::BridgeHeight)height-=CellClass::BridgeHeight;
 // 0x00439A10 converts the residual vector to integer leptons first.
 const double x=int(Velocity.X),y=int(Velocity.Y),z=int(height*Gravity+Velocity.Z);
 if(int(std::sqrt(x*x+y*y+z*z))<2.5)result=Status::Impact;
 return result;
}
