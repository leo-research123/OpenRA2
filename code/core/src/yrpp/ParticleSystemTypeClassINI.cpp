// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 psystype.cpp::Read_INI.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x6442D0; EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/ParticleTypeClass.h"
#include "RulesClassReaders.hpp"
#include <cstdlib>

bool ParticleSystemTypeClass::LoadFromINI(CCINIClass* ini) {
 try {
  if(!ini||!ObjectTypeClass::LoadFromINI(ini))return false;
  char name[512]{};ini->ReadString(ID,"HoldsWhat","",name,64);
  const auto& runtime=game::rules_runtime();
  if(!runtime.find_index||!runtime.find_index(runtime.context,AbstractType::ParticleType,name,HoldsWhat))return false;
#define I(f) f=ini->ReadInteger(ID,#f,f)
#define F(f) f=float(ini->ReadDouble(ID,#f,f))
#define D(f) f=ini->ReadDouble(ID,#f,f)
  Spawns=ini->ReadBool(ID,"Spawns",Spawns);
  I(SpawnFrames);I(ParticleCap);I(SpawnRadius);F(Slowdown);F(SpawnCutoff);F(SpawnTranslucencyCutoff);I(Lifetime);
  ini->ReadString(ID,"BehavesLike","",name,64);
  constexpr const char* behaviors[]={"Smoke","Gas","Fire","Spark","Railgun"};
  BehavesLike=::BehavesLike(-1);for(int i=0;i<5;++i)if(!_strcmpi(name,behaviors[i]))BehavesLike=::BehavesLike(i);
  if(ini->ReadString(ID,"SpawnDirection","",name,sizeof(name))){
   char* cursor=name;float v[3]{};
   for(auto& value:v){auto* token=next_rule_token(cursor);if(!token)return false;value=float(std::strtod(token,nullptr));}
   SpawnDirection={v[0],v[1],v[2]};
  }
  D(ParticlesPerCoord);D(SpiralDeltaPerCoord);D(SpiralRadius);
  D(PositionPerturbationCoefficient);D(MovementPerturbationCoefficient);D(VelocityPerturbationCoefficient);
  Laser=ini->ReadBool(ID,"Laser",Laser);LaserColor=ini->ReadColor(ID,"LaserColor",LaserColor);
  I(SparkSpawnFrames);I(LightSize);OneFrameLight=ini->ReadBool(ID,"OneFrameLight",OneFrameLight);D(SpawnSparkPercentage);
#undef I
#undef F
#undef D
  return true;
 }catch(...){return false;}
}
