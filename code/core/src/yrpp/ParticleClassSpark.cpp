// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 particle.cpp.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x62CE40 / 0x62C6E0; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"

void ParticleClass::BehaviorAI(){
 switch(int(Type->BehavesLike)){
  case 3:SparkAI();break;
  // Preserve explicit original dependencies for the other particle behaviors.
  case 0:JMP_THIS(0x62BD50);break;
  case 1:SmokeAI();break;
  case 2:JMP_THIS(0x62CB10);break;
  case 4:JMP_THIS(0x62C3A0);break;
  default:break;
 }
 if(!--RemainingEC)IsToDie=true;
}
void ParticleClass::SparkAI(){
 const auto old=Location;const auto gravity=RulesClass::Instance->Gravity;
 MovementDirection.Z=float(double(MovementDirection.Z)-gravity);
 const float dx=MovementDirection.X,dy=MovementDirection.Y,dz=float(double(MovementDirection.Z)-gravity);
 const float x=float(float(old.X)+double(dx)),y=float(float(old.Y)+double(dy));
 float z=float(float(old.Z)+double(dz));
 CoordStruct at{rule_integer(x),rule_integer(y),rule_integer(z)};
 const int floor=MapClass::Instance.GetCellFloorHeight(at),bridge=floor+CellClass::BridgeHeight;
 auto* cell=MapClass::Instance.GetCellAt(at);
 const bool bridgeCell=(unsigned(cell->Flags)&0x100u)||(unsigned(MapClass::Instance.GetCellAt(old)->Flags)&0x100u);
 const bool fromAbove=bridgeCell&&at.Z<bridge&&old.Z>=bridge;
 const bool fromBelow=bridgeCell&&at.Z>=bridge&&old.Z<bridge;
 bool blocked=false;
 if(!fromAbove&&!fromBelow&&z>=double(floor)&&double(z)-150.0<floor){
  auto* building=cell->GetBuilding();
  blocked=building||cell->ConnectsToOverlay(-1,-1);
  if(building){
   if(building->Type->LaserFence)blocked=building->LaserFenceFrame<8;
   if(building->IsStrange())blocked=false;
  }
 }
 if(z<float(floor)||fromAbove||fromBelow||blocked){
  if(fromAbove)z=float(bridge);
  else if(fromBelow)z=float(bridge-20);
  else if(double(floor-100)<z)z=float(floor);
  // YR reflects a local velocity through the slope matrices, then discards
  // it. Only the contact position and death flag are persistent spark state.
  IsToDie=true;
 }
 SetLocation({rule_integer(x),rule_integer(y),rule_integer(z)});
 const double random=double(ScenarioClass::Instance->Random.RandomRanged(0,0x7FFFFFFE))*4.656612877414201e-10;
 ColorAccum=random*0.05+Type->ColorSpeed+ColorAccum;
 if(ColorAccum>1.0){
  if(ColorIndex<Type->ColorList.Count-2){++ColorIndex;ColorAccum=0;}
  else ColorAccum=1.0;
 }
}
