// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 partsys.cpp.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x62E840 / 0x62FD60; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/GameOptionsClass.h"
#include <bit>
#include <cstdlib>
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

void ParticleSystemClass::SparkAI(){
 auto& random=ScenarioClass::Instance->Random;
 const auto absolute=[](int value){const auto bits=unsigned(value);return std::bit_cast<int>(value<0?0u-bits:bits);};
 const auto randomDouble=[&]{return double(random.RandomRanged(0,0x7FFFFFFE))*4.656612877414201e-10;};
 if(SparkSpawnFrames>0){
  if(SparkSpawnFrames==1||randomDouble()<=Type->SpawnSparkPercentage){
   const int half=Type->ParticleCap/2;
   auto* held=ParticleTypeClass::Array.GetItemOrDefault(Type->HoldsWhat);
   if(half<=0||!held||!held->XVelocity||!held->YVelocity||!held->ZVelocityRange)std::abort();
   int count=half+absolute(random.Random())%half;
   const int a=random.Random(),b=random.Random(),c=random.Random();
   Vector3D<float> spark{float(b%held->XVelocity),float(a%held->YVelocity),float(c%held->ZVelocityRange)};
   while(count--){
    auto* particle=SpawnParticle(Location,Location);if(!particle)std::abort();
    auto& v=particle->MovementDirection;const auto* type=particle->Type;
    v.X=float(random.Random()%type->XVelocity);v.Y=float(random.Random()%type->YVelocity);
    v.Z=float(type->MinZVelocity+absolute(random.Random())%type->ZVelocityRange);
    const float speed=float(Math::sqrt(double(v.X)*v.X+double(v.Y)*v.Y+double(v.Z)*v.Z));
    const auto direction=unknown_bool_F9?spark:Type->SpawnDirection;
    v.X=float(double(direction.X)+v.X);v.Y=float(double(direction.Y)+v.Y);v.Z=float(double(direction.Z)+v.Z);
    const double length=Math::sqrt(double(v.X)*v.X+double(v.Y)*v.Y+double(v.Z)*v.Z);
    if(length!=0){v.X=float(v.X/length);v.Y=float(v.Y/length);v.Z=float(v.Z/length);}
    v.X=float(double(speed)*v.X);v.Y=float(double(speed)*v.Y);v.Z=float(double(speed)*v.Z);
   }
   if(GameOptionsClass::Instance.DetailLevel==2&&SparkSpawnFrames==Type->SparkSpawnFrames&&Type->LightSize>0&&!Type->OneFrameLight)
    new SpotlightClass(Location,Type->LightSize);
  }
  if(--SparkSpawnFrames<=0)TimeToDie=true;
  const double chance=randomDouble();
  if(chance<0.3)SpotlightRadius=std::max(17,SpotlightRadius-3);
  else if(chance<0.6)SpotlightRadius=std::min(41,SpotlightRadius+3);
 }
 for(int i=0;i<Particles.Count;++i)Particles[i]->BehaviorAI();
 for(int i=Particles.Count-1;i>=0;--i)if(Particles[i]->IsToDie)Particles[i]->UnInit();
}
void ParticleSystemClass::Update(){
 switch(Type->BehavesLike){
  case BehavesLike::Spark:SparkAI();break;
  case BehavesLike::Smoke:SmokeAI();break;
  case BehavesLike::Gas:JMP_THIS(0x62E6D0);break;
  case BehavesLike::Fire:JMP_THIS(0x62F9A0);break;
  case BehavesLike::Railgun:JMP_THIS(0x62F230);break;
  default:break;
 }
 Lifetime=std::bit_cast<int>(unsigned(Lifetime)-1u);if(!Lifetime)UnInit();
 if(IsAlive&&TimeToDie&&!Particles.Count){ObjectClass::Limbo();IsAlive=false;if(!PendingDeletes.AddItem(this))std::abort();}
}
