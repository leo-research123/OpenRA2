// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Per_Cell_Process factory departure; YR 0x739EC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
void UnitClass::UpdatePosition(PCPType reason){
 if(reason==PCPType::End&&IsTether&&HasAnyLink()&&GetNthLink()->WhatAmI()==AbstractType::Building
    &&static_cast<BuildingClass*>(GetNthLink())->Type->WeaponsFactory&&GetCurrentMission()!=Mission::Unload
    &&(GetCurrentMission()!=Mission::Enter||(Destination&&Destination!=GetNthLink()))){
  const bool arrived=!Destination||(Destination->WhatAmI()==AbstractType::Cell&&Destination==GetCell());
  if(arrived||GetCell()->GetBuilding()!=GetNthLink()){
   if(SendToFirstLink(RadioCommand::NotifyUnloaded)==RadioCommand::AnswerLeave){
    if(!Destination||Destination==GetCell()){
     if(ArchiveTarget&&ArchiveTarget!=Destination)SetDestination(ArchiveTarget,true);
     else {Destination=nullptr;Scatter(CoordStruct::Empty,true,true);}
    }
    QueueMission(Mission::Move,false);
   }
  }
 }
 // Original 0x739EC0 end-of-cell sight pair precedes the Foot parent.
 if(IsAlive && reason==PCPType::End) {
  vt_entry_48C(false,0,false,nullptr);UpdateSight(false,0,false,nullptr,0);
  auto at=Location;MapClass::Instance.RevealArea3(&at,0,LastSightRange+3,false);
 }
 if(IsAlive)FootClass::UpdatePosition(reason);
}
