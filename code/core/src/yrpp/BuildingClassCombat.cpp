// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Guard/Attack/Unload; YR garrison extensions.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Unsorted.h"
#include "RulesClassReaders.hpp"
#include "type_resources.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>

bool BuildingClass::ReadyToNextMission() const {return IsReadyToCommence;}

bool BuildingClass::EnterIdleMode(bool initial,bool) {
 const bool constructing=initial&&!Unsorted::ScenarioInit&&!Unsorted::ArmageddonMode;
 BeginMode(constructing?BStateType::Construction:BStateType::Idle);
 QueueMission(constructing?Mission::Construction:Mission::Guard,false);
 return false;
}
FireError BuildingClass::GetFireError(AbstractClass* target,int weapon,bool checkRange) const {
 if((Type->CanBeOccupied&&(!Type->CanOccupyFire||!GetOccupantCount()))||IsWarpingIn())return FireError::ILLEGAL;
 if(Type->EMPulseCannon)return FireError::CANT;
 if(GetCurrentMission()==Mission::Selling||GetCurrentMission()==Mission::Construction)return FireError::ILLEGAL;
 if(!IsPowerOnline())return FireError::CANT;
 if(DelayBeforeFiring)return FireError::REARM;
 const auto result=TechnoClass::GetFireError(target,weapon,checkRange);
 if(result!=FireError::OK||!HasTurret())return result;
 const auto direction=FireAngleTo(target);
 const auto delta=std::bit_cast<short>(static_cast<unsigned short>(PrimaryFacing.Current().Raw-direction.Raw));
 return std::abs(int(delta))>(Type->TurretAnimIsVoxel?0:0x800)?FireError::FACING:result;
}

bool BuildingClass::IsArmed() const {return CanOccupyFire()||TechnoClass::IsArmed();}
int BuildingClass::GetOccupyRangeBonus() const {return std::min(Type->GetFoundationWidth(),Type->GetFoundationHeight(false))/2;}
AbstractClass* BuildingClass::GreatestThreat(ThreatType threat,CoordStruct* origin,bool onlyEnemy){
 unsigned flags=unsigned(threat)|1u;
 for(int i=0;i<2;++i)if(auto* weapon=GetWeapon(i)->WeaponType)flags|=unsigned(weapon->AllowedThreats());
 return TechnoClass::GreatestThreat(ThreatType(flags),origin,onlyEnemy);
}
WeaponStruct* BuildingClass::GetWeapon(int index) const {
 for(int i=0;i<UpgradeLevel;++i)if(auto* upgrade=Upgrades[i])if(upgrade->Weapon[index].WeaponType)return &upgrade->Weapon[index];
 if(CanOccupyFire()&&FiringOccupantIndex<Occupants.Count){
  auto* occupant=Occupants[FiringOccupantIndex];
  auto* weapon=occupant->Veterancy.IsElite()?&occupant->Type->EliteOccupyWeapon:&occupant->Type->OccupyWeapon;
  return weapon->WeaponType?weapon:occupant->GetWeapon(0);
 }
 return TechnoClass::GetWeapon(index);
}
void BuildingClass::UpdateGarrisonAnimations(){
 const bool damaged=GetHealthPercentage()<=RulesClass::Instance->ConditionYellow;
 const bool garrisoned=GetOccupantCount()>0;
 for(int slot:{18,3,4,5,6})if(Anims[slot]){
  const auto& anim=Type->BuildingAnim[slot];const char* name=damaged?anim.Damaged:garrisoned?anim.Garrisoned:anim.Anim;
  if(*name)PlayAnim(name,static_cast<BuildingAnimSlot>(slot),damaged,garrisoned,0);
 }
}
void BuildingClass::UpdateGarrison(){
 if(Type->TechLevel!=-1)return;
 if(IsRedHP())UnloadOccupants(false,false);
 HouseClass* neutral=nullptr;const int side=SideClass::FindIndex("Civilian");
 for(auto* house:HouseClass::Array)if(house->Type->SideIndex==side){neutral=house;break;}
 if(!neutral)return; // Detached fixtures may omit the scenario's neutral house.
 if(!GetOccupantCount()&&Owner!=neutral){
  if(Owner->IsControlledByCurrentPlayer()&&!game::type_resources().audio_unavailable){
   VocClass::PlayGlobal(RulesClass::Instance->BuildingAbandonedSound,0x2000,1.0f,nullptr);
   if(RadarEventClass::Create(RadarEventType::BuildingCaptured,GetMapCoords()))VoxClass::Play("EVA_StructureAbandoned",-1,-1);
  }
  UpdateGarrisonAnimations();SetOwningHouse(neutral,false);
 }
 if(GetOccupantCount()>0&&Owner==neutral){UpdateGarrisonAnimations();SetOwningHouse(Occupants[0]->Owner,false);}
}
int BuildingClass::Mission_Guard(){
 if(Type->IsGattling){GattlingRateDown(MissionAccumulateTime);MissionAccumulateTime=0;}
 if(IsArmed()){
  IsReadyToCommence=true;
  if(!Type->Artillary&&(!Type->CanBeOccupied||GetOccupantCount()>0)&&Target){QueueMission(Mission::Attack,false);NextMission();return 1;}
  return rule_integer(CurrentMissionControl()->AARate*900.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
 }
 return rule_integer(CurrentMissionControl()->Rate*2700.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
}
int BuildingClass::Mission_Attack(){
 if(!Target){SupportingPrisms=0;QueueMission(Mission::Guard,false);NextMission();return 1;}
 const int index=SelectWeapon(Target);IsReadyToCommence=true;
 auto error=GetFireError(Target,index,true);
 if(error==FireError::FACING&&HasTurret()&&Type->TurretAnimIsVoxel){
  const auto direction=FireAngleTo(Target);
  const auto difference=std::bit_cast<short>(static_cast<unsigned short>(PrimaryFacing.Current().Raw-direction.Raw));
  const auto turn=std::bit_cast<short>(static_cast<unsigned short>(unsigned(Type->ROT)<<8));
  if(!Type->ROT||std::abs(int(turn))>=std::abs(int(difference))){PrimaryFacing.SetCurrent(direction);error=GetFireError(Target,index,true);}
 }
 switch(error){
 case FireError::OK:
  Fire(Target,index);
  if(Type->IsGattling){GattlingRateUp(MissionAccumulateTime);MissionAccumulateTime=0;}else ++TurretAnimFrame;
  return 1;
 case FireError::REARM:case FireError::FACING:{
  PrimaryFacing.SetDesired(FireAngleTo(Target));
  if(Type->IsGattling){GattlingRateUp(MissionAccumulateTime);MissionAccumulateTime=0;}
  else if(error==FireError::REARM)++TurretAnimFrame;
  return 2;}
 case FireError::AMMO:case FireError::ILLEGAL:case FireError::CANT:case FireError::RANGE:
  SetTarget(nullptr);SupportingPrisms=0;
  if(Type->IsGattling){GattlingRateDown(MissionAccumulateTime);MissionAccumulateTime=0;}
  QueueMission(Mission::Guard,false);NextMission();return 1;
 case FireError::CLOAKED:
  Uncloak(false);
  if(Type->IsGattling)GattlingRateDown(MissionAccumulateTime);
  break;
 case FireError::BUSY:
  if(Type->IsGattling){GattlingRateDown(MissionAccumulateTime);MissionAccumulateTime=0;}
  return 1;
 default:break;
 }
 if(Target)PrimaryFacing.SetDesired(FireAngleTo(Target));
 MissionAccumulateTime=0;return 1;
}
int BuildingClass::Mission_Unload(){
 if(Type->WeaponsFactory)return MissionFactoryUnload();
 if(GetOccupantCount()>0)UnloadOccupants(false,false);
 QueueMission(Mission::Guard,false);return 1;
}
