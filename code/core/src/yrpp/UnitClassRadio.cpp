// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Receive_Message; YR 0x737430 passenger protocol.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CaptureManagerClass.h"

RadioCommand UnitClass::ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data){
 const auto fits=[&]{return double(Passengers.GetTotalSize())+sender->GetTechnoType()->Size<=Type->Passengers&&sender->GetTechnoType()->Size<=Type->SizeLimit;};
 switch(command){
 case RadioCommand::RequestDockRefinery:
  FootClass::ReceiveCommand(sender,command,data);
  if(!PrimaryFacing.IsRotating()&&PrimaryFacing.Current().Raw!=short(0x4000))Locomotor->Do_Turn(DirStruct(short(0x4000)));
  else if(!Locomotor->Is_Moving()&&IsTether&&GetNthLink()&&GetCurrentMission()==Mission::Enter)SendToFirstLink(RadioCommand::RequestCompleteEnter);
  return RadioCommand::AnswerPositive;
 case RadioCommand::NotifyUnlink:
  if(GetCurrentMission()==Mission::Return)QueueMission(Mission::Guard,false);
  FootClass::ReceiveCommand(sender,command,data);return RadioCommand::AnswerPositive;
 case RadioCommand::QueryCanEnter:
  if(!Type->Passengers||!sender||!Owner->IsAlliedWith(sender)||IsBeingWarpedOut()||GetCell()->GetBuilding())return RadioCommand::AnswerInvalid;
  if(sender->MindControlledBy||sender->MindControlledByAUnit)return RadioCommand::AnswerInvalid;
  if((sender->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None&&static_cast<FootClass*>(sender)->ParasiteEatingMe)return RadioCommand::AnswerInvalid;
  if(sender->CaptureManager&&sender->CaptureManager->IsControllingSomething())return RadioCommand::AnswerInvalid;
  return fits()?RadioCommand::AnswerPositive:RadioCommand::AnswerNegative;
 case RadioCommand::RequestLoading:{
  if((Locomotor->Is_Moving()||Destination)&&CurrentMission==Mission::Move)return RadioCommand::AnswerNegative;
  if(GetNthLink()!=sender){if(!fits())return RadioCommand::AnswerNegative;if(!HasAnyLink())SendCommand(RadioCommand::RequestLink,sender);}
  if(!Destination){
   const auto movement=sender->GetTechnoType()->MovementZone;
   const int zone=MapClass::Instance.GetMovementZoneType(LastMapCoords,movement,false);
   const int other=(sender->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None?MapClass::Instance.GetMovementZoneType(static_cast<FootClass*>(sender)->LastMapCoords,movement,false):-1;
   if(zone!=other||GetCell()->LandType==LandType::Water){SetDestination(sender,true);return RadioCommand::AnswerLoading;}
  }
  if(Type->Passengers<=0||!fits())break;
  FootClass::ReceiveCommand(sender,command,data);
  if(!Locomotor->Is_Moving()&&!unknown_bool_6AF&&!IsTether&&SendCommand(RadioCommand::QueryMoving,sender)==RadioCommand::AnswerPositive){
   data=this;if(SendCommandWithData(RadioCommand::RequestMoveTo,data,sender)!=RadioCommand::AnswerPositive)SendCommand(RadioCommand::NotifyUnlink,sender);
  }
  return RadioCommand::AnswerPositive;
 }
 case RadioCommand::RequestCompleteEnter:
  if(Passengers.GetTotalSize()==Type->Passengers)UnloadTimer.StartTimer10(Type->DeployTime);
  return RadioCommand::unknown_5;
 default:break;
 }
 return FootClass::ReceiveCommand(sender,command,data);
}
