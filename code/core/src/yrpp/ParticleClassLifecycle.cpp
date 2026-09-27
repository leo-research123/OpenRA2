// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 particle.cpp.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x62B5E0 / 0x62D9A0; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "RulesClassReaders.hpp"
#include <cstdlib>
#include <cmath>

namespace { DynamicVectorClass<ParticleClass*> particles; }
DynamicVectorClass<ParticleClass*>& ParticleClass::Array=particles;

ParticleClass::ParticleClass(ParticleTypeClass* type,CoordStruct* origin,CoordStruct* target,ParticleSystemClass* system) noexcept
 : ObjectClass(),Type(type),Color(),ColorIndex(0),ColorAccum(0),Velocity{},GasVelocity{0,0,-1},
 unknown_D8(0),unknown_DC(0),unknown_E0(0),Speed(type->Velocity),unknown_coords_E8(*target),unknown_coords_F4(*origin),
 unknown_coords_100(CoordStruct::Empty),MovementDirection{float(target->X-origin->X),float(target->Y-origin->Y),float(target->Z-origin->Z)},
 PrecisePosition{float(origin->X),float(origin->Y),float(origin->Z)},ParticleSystem(system),RemainingEC(0),RemainingDC(WORD(type->MaxDC)),
 StateAIAdvance(type->StateAIAdvance),unknown_12D(0),StartStateAI(type->StartStateAI),Translucency(BYTE(type->Translucency)),unknown_130(0),IsToDie(0),unused_134(0) {
 Create_ID();if(!Array.AddItem(this))std::abort();
 auto& random=ScenarioClass::Instance->Random;
 if(int(Type->BehavesLike)==1)Speed+=random.Random()%2;
 if(Type->MaxEC<=0)std::abort(); // Original modulo requires a positive lifetime.
 RemainingEC=WORD(Type->MaxEC+std::abs(random.Random()%(int(Type->BehavesLike)==4?10:Type->MaxEC)));
 auto at=*origin;const int floor=MapClass::Instance.GetCellFloorHeight(at);if(at.Z<=floor)at.Z=floor;SetLocation(at);
 auto& v=MovementDirection;
 const double length=Math::sqrt(double(v.X)*v.X+double(v.Y)*v.Y+double(v.Z)*v.Z);
 if(length!=0){v.X=float(v.X/length);v.Y=float(v.Y/length);v.Z=float(v.Z/length);}
 if(Type->Normalized){
  const float vx=float(std::abs(rule_integer(double(v.X)*Speed))),vy=float(std::abs(rule_integer(double(v.Y)*Speed)));
  double time=9999.0;
  if(vx>0.000001)time=double(std::abs(origin->X-target->X))/vx;
  if(vy>0.000001)time=std::min(time,double(float(double(std::abs(origin->Y-target->Y))/vy)));
  StateAIAdvance=BYTE(rule_integer(time/(static_cast<signed char>(Type->FinalDamageState)+1)+1.0));
 }
 if(Type->ColorList.Count){
  const auto a=Type->StartColor1,b=Type->StartColor2;
  if(!(a.R||a.G||a.B||b.R||b.G||b.B))Color=Type->ColorList[0];
  else{
   const float t=float(double(random.RandomRanged(0,0x7FFFFFFE))*4.656612877414201e-10);
   Color=RGBClass(rule_integer(a.R*(1.0-double(t))+b.R*double(t)),rule_integer(a.G*(1.0-double(t))+b.G*double(t)),rule_integer(a.B*(1.0-double(t))+b.B*double(t)));
  }
 }
 ObjectClass::Unlimbo(at,DirType::North);
}
ParticleClass::~ParticleClass(){
 ObjectClass::Limbo();NotifyObjectExpired(true);Array.Remove(this);
 while(PendingDeletes.Remove(this)){}Type=nullptr;
}
