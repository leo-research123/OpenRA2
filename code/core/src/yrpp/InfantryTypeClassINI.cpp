// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infatype.cpp::Read_INI / Read_Sequence_INI, all YR derived keys.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// YR 0x005240A0 / 0x00523D00; EA Section 7: third_party/opents/LICENSE.md.
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "RulesClassReaders.hpp"
#include "type_resources.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {
constexpr const char* names[]{"Ready","Guard","Prone","Walk","FireUp","Down","Crawl","Up","FireProne",
 "Idle1","Idle2","Die1","Die2","Die3","Die4","Die5","Tread","Swim","WetIdle1","WetIdle2","WetDie1",
 "WetDie2","WetAttack","Hover","Fly","Tumble","FireFly","Deploy","Deployed","DeployedFire","DeployedIdle",
 "Undeploy","Cheer","Paradrop","AirDeathStart","AirDeathFalling","AirDeathFinish","Panic","Shovel","Carry",
 "SecondaryFire","SecondaryProne"};
char* sound_token(char*& cursor) {
 cursor+=std::strspn(cursor," ,\t");if(!*cursor)return nullptr;
 char* out=cursor;cursor+=std::strcspn(cursor," ,\t");if(*cursor)*cursor++=0;return out;
}
void read_sequences(InfantryTypeClass& type) {
 auto& art=game::type_art_ini();char section[32]{};
 if(!art.ReadString(type.ImageFile,"Sequence","",section,sizeof(section)))return;
 if(!type.Sequence)throw std::runtime_error("Infantry sequence storage missing");
 constexpr const char* directions[]{"N","NE","E","SE","S","SW","W","NW"};
 for(unsigned i=0;i<std::size(names);++i) {
  auto& s=type.Sequence->Sequences[i];char text[32]{},facing[10]{};
  if(art.ReadString(section,names[i],"",text,sizeof(text))) {
   // Original sscanf keeps earlier assignments on an incomplete record. Bound
   // the text conversion locally; do not reproduce the target stack overwrite.
   std::sscanf(text,"%d,%d,%d,%9s",&s.StartFrame,&s.CountFrames,&s.FacingMultiplier,facing);
   for(unsigned d=0;d<8;++d)if(!std::strcmp(facing,directions[d]))s.Facing=static_cast<SequenceFacing>(d);
  }
  char key[64];std::snprintf(key,sizeof(key),"%sSounds",names[i]);
  if(!art.ReadString(section,key,"",text,sizeof(text))||game::type_resources().audio_unavailable)continue;
  char* cursor=text;
  while(char* frame=sound_token(cursor)) {
   char* name=sound_token(cursor);if(!name)break;int index=-1;
   if(!game::rules_sound_index(name,index))throw std::runtime_error("Infantry sequence sound dependency missing");
   if(index==-1||s.SoundCount>=2)continue;
   const int start=rule_decimal(frame);
   if(s.SoundCount==0){s.Sound1StartFrame=start;s.Sound1Index=index;}
   else{s.Sound2StartFrame=start;s.Sound2Index=index;}
   ++s.SoundCount;
  }
 }
}
}
void InfantryTypeClass::ReadSequence() {
 try {read_sequences(*this);}catch(...){std::abort();}
}
bool InfantryTypeClass::LoadFromINI(CCINIClass* ini) {
 try {
  DamageSparks=false;
  if(!TechnoTypeClass::LoadFromINI(ini))return false;
  Pip=static_cast<PipIndex>(ini->ReadPip(ID,"Pip",static_cast<int>(Pip)));
  OccupyPip=static_cast<PipIndex>(ini->ReadPip(ID,"OccupyPip",static_cast<int>(OccupyPip)));
  read_rule_type(*ini,ID,"OccupyWeapon",AbstractType::WeaponType,OccupyWeapon.WeaponType);
  read_rule_type(*ini,ID,"EliteOccupyWeapon",AbstractType::WeaponType,EliteOccupyWeapon.WeaponType);
  if(!game::type_resources().audio_unavailable)read_rule_sound_list(*ini,ID,"VoiceComment",VoiceComment);
  read_rule_type_list(*ini,ID,"DeadBodies",AbstractType::AnimType,DeadBodies);
  read_rule_type_list(*ini,ID,"DeathAnims",AbstractType::AnimType,DeathAnims);
#define B(f) f=ini->ReadBool(ID,#f,f)
  B(Cyborg);B(NotHuman);if(Cyborg)DamageSparks=true;
  if(!game::type_resources().audio_unavailable){read_rule_sound(*ini,ID,"EnterWaterSound",EnterWaterSound);read_rule_sound(*ini,ID,"LeaveWaterSound",LeaveWaterSound);}
  B(Fearless);B(Fraidycat);B(Infiltrate);B(Ivan);B(Occupier);B(Assaulter);
  DirectionDistance=ini->ReadInteger(ID,"DetectionDistance",DirectionDistance);
  HarvestRate=ini->ReadInteger(ID,"HarvestRate",HarvestRate);
  B(C4);B(Civilian);B(Engineer);B(TiberiumProof);B(Agent);B(Thief);B(VehicleThief);B(Doggie);
  B(Deployer);B(DeployedCrushable);B(UseOwnName);B(JumpJetTurn);
#undef B
  if(C4||Engineer||Agent)Infiltrate=true;
  auto& art=game::type_art_ini();Crawls=art.ReadBool(ImageFile,"Crawls",Crawls);
#define I(f) f=art.ReadInteger(ImageFile,#f,f)
  I(FireUp);I(FireProne);I(SecondaryFire);I(SecondaryProne);
#undef I
  read_sequences(*this);return true;
 }catch(...){return false;}
}
