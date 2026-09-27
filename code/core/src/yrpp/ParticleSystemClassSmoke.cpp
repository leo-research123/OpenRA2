// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 partsys.cpp Smoke_AI; calibrated to YR 0x62ED40.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>

void ParticleSystemClass::SmokeAI(){
 if(Owner&&(Owner->AbstractFlags&AbstractFlags::Object)!=AbstractFlags::None&&Owner->WhatAmI()!=AbstractType::Building)
  SetLocation(Owner->GetCoords()+SpawnDistanceToOwner);
 for(int i=0;i<Particles.Count;++i)Particles[i]->BehaviorAI();
 auto& random=ScenarioClass::Instance->Random;
 for(int i=Particles.Count-1;i>=0;--i){
  auto* particle=Particles[i];
  if(!particle->IsToDie){particle->SmokeMotionAI();continue;}
  if(particle->Type->NextParticle!=-1){
   auto* next=ParticleTypeClass::Array.GetItemOrDefault(particle->Type->NextParticle);
   const int spread=particle->Type->Radius>>3;
   if(next&&spread>0){
    int x=random.Random()%spread;x=x>0?spread+x:x-spread;
    int y=random.Random()%spread;y=y>0?spread+y:y-spread;
    for(int sign:{1,-1}){
     auto at=particle->Location+CoordStruct{sign*x,sign*y,0},target=CoordStruct::Empty;
     auto* child=GameCreate<ParticleClass>(next,&at,&target,this);
     if(child){
      if(!Particles.AddItem(child)){delete child;continue;}
      child->Speed=particle->Speed;
      child->Translucency=BYTE(particle->Translucency+(random.Random()%6!=0?25:0));
     }
    }
   }
  }
  particle->UnInit();
 }
 const int interval=rule_integer(SpawnFrames);
 if(!TimeToDie&&IsAlive&&interval>0&&Unsorted::CurrentFrame%interval==0){
  const bool inTube=Owner&&(Owner->AbstractFlags&AbstractFlags::Foot)!=AbstractFlags::None&&static_cast<FootClass*>(Owner)->TubeIndex>=0;
  if(!inTube&&Type->SpawnRadius>=0){
   const int x=random.Random(),y=random.Random(),range=Type->SpawnRadius+1;
   if(auto* particle=SpawnParticle(Location+CoordStruct{x%range,y%range,10},TargetCoords)){
    if(SpawnFrames>Type->SpawnTranslucencyCutoff)particle->Translucency+=25;
    particle->Speed=float(double(particle->Speed)-(double(SpawnFrames)-Type->SpawnFrames)*0.34999999);
    if(particle->Speed<2.0)particle->Speed=2.0f;
   }
  }
 }
 SpawnFrames=float(double(Type->Slowdown)+SpawnFrames);
 if(SpawnFrames>Type->SpawnCutoff)TimeToDie=true;
}
