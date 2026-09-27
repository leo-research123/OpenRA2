// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 ptype.cpp::Read_INI.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x644F50 / RGB list 0x476B20; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "RulesClassReaders.hpp"

bool ParticleTypeClass::LoadFromINI(CCINIClass* ini) {
 try {
  if(!ini||!ObjectTypeClass::LoadFromINI(ini))return false;
  TypeList<RGBClass> colors;char text[512]{};
  if(ini->ReadString(ID,"ColorList","",text,sizeof(text))){
   char* cursor=text;
   while(auto* red=next_rule_token(cursor)){
    auto* green=next_rule_token(cursor);auto* blue=next_rule_token(cursor);
    if(!green||!blue)break;
    // Original syntax is (R,G,B),(R,G,B). It discards the first and last
    // character, truncating each parsed component to a byte.
    const auto length=std::strlen(blue);if(!length)continue;blue[length-1]=0;
    if(!colors.AddItem(RGBClass(rule_decimal(red+1),rule_decimal(green),rule_decimal(blue))))throw std::bad_alloc();
   }
  }
  ColorList=colors;
#define I(f) f=ini->ReadInteger(ID,#f,f)
#define S(f) f=BYTE(ini->ReadInteger(ID,#f,static_cast<signed char>(f)))
  I(MaxDC);I(MaxEC);I(Damage);read_rule_type(*ini,ID,"Warhead",AbstractType::WarheadType,Warhead);
  I(StartFrame);I(NumLoopFrames);I(Translucency);I(WindEffect);
  Velocity=float(ini->ReadDouble(ID,"Velocity",Velocity));Deacc=float(ini->ReadDouble(ID,"Deacc",Deacc));I(Radius);
  DeleteOnStateLimit=ini->ReadBool(ID,"DeleteOnStateLimit",DeleteOnStateLimit);
  S(EndStateAI);S(StartStateAI);S(StateAIAdvance);
  Translucent50State=BYTE(ini->ReadInteger(ID,"Translucent50State",Translucent50State));
  Translucent25State=BYTE(ini->ReadInteger(ID,"Translucent25State",Translucent25State));
  Normalized=ini->ReadBool(ID,"Normalized",Normalized);ColorSpeed=ini->ReadDouble(ID,"ColorSpeed",ColorSpeed);
  I(XVelocity);I(YVelocity);I(MinZVelocity);I(ZVelocityRange);
  auto offset=NextParticleOffset;ini->ReadPoint3D(NextParticleOffset,ID,"NextParticleOffset",offset);
  StartColor1=ini->ReadColor(ID,"StartColor1",StartColor1);StartColor2=ini->ReadColor(ID,"StartColor2",StartColor2);
  FinalDamageState=BYTE(ini->ReadInteger(ID,"FinalDamageState",static_cast<signed char>(EndStateAI)));
  if(ini->ReadString(ID,"NextParticle","",text,32)){
   const auto& runtime=game::rules_runtime();
   if(!runtime.find_index||!runtime.find_index(runtime.context,AbstractType::ParticleType,text,NextParticle))return false;
  }
  ini->ReadString(ID,"BehavesLike","",text,32);
  // Particle and particle-system enums differ in their first two entries.
  constexpr const char* behaviors[]={"Gas","Smoke","Fire","Spark","Railgun"};
  BehavesLike=::BehavesLike(-1);for(int i=0;i<5;++i)if(!_strcmpi(text,behaviors[i]))BehavesLike=::BehavesLike(i);
#undef I
#undef S
  return true;
 }catch(...){return false;}
}
