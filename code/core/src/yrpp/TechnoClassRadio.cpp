// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Receive_Message; YR 0x6F4AB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>

RadioCommand TechnoClass::ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data){
 switch(command){
 case RadioCommand::NotifyUnlink:
  if(IsTether&&sender->IsTether)SendCommand(RadioCommand::RequestUntether,sender);
  RadioClass::ReceiveCommand(sender,command,data);return RadioCommand::AnswerPositive;
 case RadioCommand::NotifyBeginLoad:case RadioCommand::RequestUnload:case RadioCommand::RequestDockRefinery:
  SendCommand(RadioCommand::RequestTether,sender);RadioClass::ReceiveCommand(sender,command,data);return RadioCommand::AnswerPositive;
 case RadioCommand::NotifyUnloaded:
  SendCommand(RadioCommand::RequestUntether,sender);return SendCommand(RadioCommand::NotifyUnlink,sender);
 case RadioCommand::RequestTether:
  if(WhatAmI()==AbstractType::Aircraft&&static_cast<AircraftClass*>(this)->Type->Carryall)break;
  if(!IsTether){IsTether=true;SendCommand(command,sender);return RadioCommand::AnswerPositive;}break;
 case RadioCommand::RequestUntether:
  if(IsTether){IsTether=false;SendCommand(command,sender);return RadioCommand::AnswerPositive;}break;
 case RadioCommand::RequestAlternativeTether:
  if(!IsAlternativeTether){IsAlternativeTether=true;SendCommand(command,sender);return RadioCommand::AnswerPositive;}break;
 case RadioCommand::RequestAlternativeUntether:
  if(IsAlternativeTether){IsAlternativeTether=false;SendCommand(command,sender);return RadioCommand::AnswerPositive;}break;
 case RadioCommand::RequestAttack:
  if(auto* weapon=GetTurretWeapon();weapon&&weapon->WeaponType){SetTarget(data);QueueMission(Mission::Attack,false);return RadioCommand::AnswerPositive;}break;
 case RadioCommand::RequestReload:
  if(Ammo==GetTechnoType()->Ammo)return RadioCommand::AnswerNegative;
  ++Ammo;return RadioCommand::AnswerPositive;
 case RadioCommand::RequestRepair:{
  if(GetHealthPercentage()>=RulesClass::Instance->ConditionGreen)return RadioCommand::AnswerNegative;
  const int cost=GetTechnoType()->GetRepairStepCost(),step=std::max(GetTechnoType()->GetRepairStep(),1);
  if(Owner->Available_Money()<cost)return RadioCommand::AnswerBlocked;
 if(cost)Owner->TakeMoney(cost);Health+=step;EstimatedHealth+=step;
  if((AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None)if(auto* parasite=static_cast<FootClass*>(this)->ParasiteEatingMe){parasite->ParasiteImUsing->SuppressionTimer.Start(50);parasite->ParasiteImUsing->ExitUnit();}
  if((GetHealthPercentage()>RulesClass::Instance->ConditionYellow||GetHeight()<-10)&&DamageParticleSystem)DamageParticleSystem->UnInit();
  if(GetHealthPercentage()>=RulesClass::Instance->ConditionGreen){Health=EstimatedHealth=GetTechnoType()->Strength;return RadioCommand::AnswerDone;}
  return RadioCommand::AnswerPositive;
 }
 default:break;
 }
 return RadioClass::ReceiveCommand(sender,command,data);
}

CellStruct* TechnoClass::NearbyLocation(CellStruct* output,AbstractClass* destination){
 auto speed=GetTechnoType()->SpeedType;if(speed==SpeedType::Float)speed=SpeedType::Track;
 const auto at=CellClass::Coord2Cell(destination?destination->GetCoords():GetCoords());
 const auto movement=GetTechnoType()->MovementZone;
 const int zone=movement==MovementZone(-1)?-1:MapClass::Instance.GetMovementZoneType(at,movement,OnBridge);
 return MapClass::Instance.NearByLocation(*output,at,speed,zone,movement,OnBridge,1,1,false,false,false,true,CellStruct::Empty,false,false);
}
bool TechnoClass::ApproachEnterQueue(){
 if(!QueueUpToEnter)return false;
 if(!QueueUpToEnter->IsAlive){QueueUpToEnter=nullptr;return false;}
 CellStruct at;NearbyLocation(&at,QueueUpToEnter);
 if(at==CellStruct::Empty)return false;
 SetDestination(MapClass::Instance.GetCellAt(at),true);QueueMission(Mission::Move,false);return true;
}
bool TechnoClass::UpdateEnterQueue(){
 auto* transport=QueueUpToEnter;if(!transport)return false;
 if(!transport->IsAlive){QueueUpToEnter=nullptr;return false;}
 if(SendCommand(RadioCommand::RequestLink,transport)!=RadioCommand::AnswerPositive){
  if(transport->WhatAmI()==AbstractType::Building&&WhatAmI()==AbstractType::Aircraft){
   auto* type=static_cast<BuildingClass*>(transport)->Type;
   if(type->UnitReload&&!type->UnitRepair)SendCommand(RadioCommand::RequestLoading,transport);
  }
  return false;
 }
 if(SendCommand(RadioCommand::QueryCanEnter,transport)==RadioCommand::AnswerPositive){
  QueueMission(Mission::Enter,true);SetDestination(transport,true);QueueUpToEnter=nullptr;
 }else{
  QueueMission(Mission::None,false);SetDestination(nullptr,true);QueueUpToEnter=nullptr;SendCommand(RadioCommand::NotifyUnlink,transport);
 }
 return true;
}
