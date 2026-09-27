// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 particle.cpp Smoke_Behavior_AI / Gas_Motion_AI;
// YR 0x62C540 / 0x62D3F0: TS motion names differ from YR's behavior dispatch.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MapClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>

void ParticleClass::SmokeAI(){
 auto& random=ScenarioClass::Instance->Random;
 const auto absolute=[](int value){const auto bits=unsigned(value);return std::bit_cast<int>(value<0?0u-bits:bits);};
 if(Unsorted::CurrentFrame&1){
  if((absolute(random.Random())&3)==0){
   const bool y=absolute(random.Random())&1;
   const int delta=absolute(random.Random())%3-1;
   (y?Velocity.Y:Velocity.X)+=delta;
   Velocity.X=std::clamp(Velocity.X,-5,5);Velocity.Y=std::clamp(Velocity.Y,-5,5);
  }
  Velocity.Z=0;
 }
 if(StartStateAI<Type->EndStateAI){
  const int interval=static_cast<signed char>(Type->StateAIAdvance)+(UniqueID&1);
  if(interval&&(Type->MaxEC-int(RemainingEC)+static_cast<int>(UniqueID))%interval==0)++StartStateAI;
  if(StartStateAI==Type->EndStateAI&&Type->DeleteOnStateLimit)IsToDie=true;
 }
 if(Speed>3.0)Speed=float(double(Speed)-Type->Deacc);
}

void ParticleClass::SmokeMotionAI(){
 // Original tables 0x8366A4 / 0x8366C4 (YR fixes TS southeast X).
 constexpr int windX[]{0,2,2,2,0,-2,-2,-2},windY[]{-2,-2,0,2,2,2,0,-2};
 const int wind=RulesClass::Instance->WindDirection&7;
 auto at=Location;
 at.X+=Type->WindEffect*windX[wind]+Velocity.X;
 at.Y+=Type->WindEffect*windY[wind]+Velocity.Y;
 at.Z+=rule_integer(Speed)+Velocity.Z;
 auto& map=MapClass::Instance;auto* cell=map.GetCellAt(Location);
 if((unsigned(cell->Flags)&0x100u)&&Location.Z<map.GetCellFloorHeight(Location)+CellClass::BridgeHeight
    &&at.Z>=map.GetCellFloorHeight(Location)+CellClass::BridgeHeight-Unsorted::LevelHeight*5/2){
  // 0x62D4DF: bridge underside is 2.5 height levels below its deck.
  IsToDie=true;
 }else SetLocation(at);
}
