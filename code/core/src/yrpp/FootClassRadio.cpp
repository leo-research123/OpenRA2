// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Receive_Message; YR 0x4D8FB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"

RadioCommand FootClass::ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data){
 switch(command){
 case RadioCommand::QueryWantEnter:
  if(GetCurrentMission()==Mission::Enter||QueuedMission==Mission::Enter){TechnoClass::ReceiveCommand(sender,command,data);return RadioCommand::AnswerPositive;}break;
 case RadioCommand::RequestMoveTo:
  if(data&&CellClass::Coord2Cell(data->GetCoords())==GetMapCoords())return RadioCommand::AnswerAwaiting;
  if(GetCurrentMission()==Mission::Guard&&QueuedMission==Mission::None)QueueMission(Mission::Move,false);
  if(QueuedMission==Mission::Enter&&ReadyToNextMission())NextMission();
  SetDestination(data,true);UpdateTimer.Start(0);return RadioCommand::AnswerPositive;
 case RadioCommand::QueryMoving:
  data=Destination;return !Destination||!Locomotor->Is_Moving()?RadioCommand::AnswerPositive:RadioCommand::AnswerNegative;
 case RadioCommand::NotifyLeave:
  if(HasAnyLink()&&Destination==GetNthLink())SetDestination(nullptr,true);
  if(GetCurrentMission()==Mission::Sleep){QueueMission(Mission::Guard,false);if(ReadyToNextMission())NextMission();}
  if(GetCurrentMission()==Mission::Enter)QueueMission(Mission::Guard,false);
  if(!unknown_bool_6AF&&!Destination)Scatter(CoordStruct::Empty,true,true);break;
 case RadioCommand::RequestRepair:if(Destination)return RadioCommand::AnswerNegative;break;
 case RadioCommand::QueryOnBuilding:return GetCell()->GetBuilding()==sender?RadioCommand::AnswerPositive:RadioCommand::AnswerNegative;
 default:break;
 }
 return TechnoClass::ReceiveCommand(sender,command,data);
}
