// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Harvesting/Do_MISSION_HARVEST/Do_MISSION_UNLOAD;
// YR 0x73D450/0x73E5E0/0x73D630. YR unloads a complete resource slot at once.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "RulesClassReaders.hpp"
#include <cmath>

namespace {
int refinery_distance(const UnitClass& unit,const BuildingClass& refinery){
 const auto delta=unit.GetCoords()-refinery.GetCoords();
 return rule_integer(std::sqrt(double(delta.X)*delta.X+double(delta.Y)*delta.Y+double(delta.Z)*delta.Z));
}
}

bool UnitClass::Harvesting(){
 if(Destination)return true;
 auto* cell=GetCell();
 if(Type->Harvester&&GetStoragePercentage()<1.0&&cell->LandType==LandType::Tiberium){
  const int index=cell->GetContainedTiberiumIndex();
  const int amount=cell->ReduceTiberium(std::min(1,int(Type->Storage-Tiberium.GetTotalAmount())));
  // 0x0073D5A6: only the amount returned by the cell can start a load.
  if(amount>0){
   Tiberium.AddAmount(float(amount),index);
   // 0x73D450 explicitly resets the stage. StageClass::Start only restarts
   // the timer; retaining stage 9 would load another bail on every update.
   Animation.Value=0;Animation.Start(RulesClass::Instance->HarvesterLoadRate);return true;
  }
 }else {Animation.Value=0;Animation.Start(0);}
 return false;
}
int UnitClass::Mission_Harvest(){
 if(!Type->Harvester&&!Type->Weeder)return 450;
 bool haveRefinery=false;for(auto* type:Type->Dock)for(auto* building:Owner->Buildings)if(building->IsAlive&&!building->InLimbo&&building->Type==type)haveRefinery=true;
 if(!haveRefinery){QueueMission(Mission::Guard,false);return 1;}
 const auto& rules=*RulesClass::Instance;
 switch(MissionStatus){
 case 0:{
  if(GetStoragePercentage()>=1.0){MissionStatus=2;return 1;}
  bool close=true;if(ArchiveTarget){SetDestination(ArchiveTarget,true);SetArchiveTarget(nullptr);close=false;}
  IsHarvesting=false;
  if(MoveToTiberium(rules.TiberiumLongScan/256,close)){IsHarvesting=true;Animation.Value=0;Animation.Start(2);MissionStatus=1;return 1;}
  if(Destination)IsUseless=false;
  else{MissionStatus=4;IsUseless=true;Owner->IsTiberiumShort=true;return 105;}
  break;
 }
 case 1:
  if(!Animation.Rate){Animation.Value=0;Animation.Start(rules.HarvesterLoadRate);}
  if(Animation.Value<9)return 1;
  if(Harvesting())return 1;
  IsHarvesting=false;
  if(GetStoragePercentage()==1.0){
   MissionStatus=2;CellStruct at;ScanForTiberium(&at,rules.TiberiumShortScan/256,0);
   SetArchiveTarget(at==CellStruct::Empty?nullptr:MapClass::Instance.GetCellAt(at));
  }else if(MoveToTiberium(rules.TiberiumShortScan/256,false)||Destination){MissionStatus=1;IsHarvesting=true;}
  else{SetArchiveTarget(nullptr);MissionStatus=2;}
  return 1;
 case 2:
  if(!Destination){
   auto* refinery=TryNearestDockBuilding(&Type->Dock,0,0);
   const int range=(Type->Teleporter?rules.ChronoHarvTooFarDistance:rules.HarvesterTooFarDistance)*256;
   if(refinery&&refinery_distance(*this,*refinery)<=range&&SendCommand(RadioCommand::RequestLink,refinery)==RadioCommand::AnswerPositive)MissionStatus=3;
   else{
    ++Unsorted::ScenarioInit;refinery=TryNearestDockBuilding(&Type->Dock,0,1);--Unsorted::ScenarioInit;
    if(refinery&&(Type->Teleporter||refinery_distance(*this,*refinery)>768)){
     const auto anchor=refinery->GetMapCoords();
     const CellStruct corner{short(anchor.X+refinery->Type->GetFoundationWidth()),short(anchor.Y+refinery->Type->GetFoundationHeight(false))};
     const auto nearby=MapClass::Instance.NearByLocation(corner,SpeedType::Wheel,-1,MovementZone::Normal,false,1,1,false,false,false,true,CellStruct::Empty,false,false);
     SetDestination(nearby==CellStruct::Empty?nullptr:MapClass::Instance.GetCellAt(nearby),true);
    }
   }
  }break;
 case 3:QueueMission(Mission::Enter,false);return 1;
 case 4:
  if(auto* refinery=GetCell()->GetBuilding();refinery&&refinery->Type->Refinery){CellStruct nearby;NearbyLocation(&nearby,refinery);if(nearby!=CellStruct::Empty)SetDestination(MapClass::Instance.GetCellAt(nearby),true);}
  QueueMission(Mission::Guard,false);break;
 }
 return rule_integer(CurrentMissionControl()->Rate*900.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
}
int UnitClass::MissionHarvestUnload(){
 auto* contact=GetNthLink();
 auto* refinery=contact&&contact->WhatAmI()==AbstractType::Building?static_cast<BuildingClass*>(contact):nullptr;
 if(!refinery||!refinery->IsAlive){Unloading=false;EnterIdleMode(false,true);if(Locomotor->Is_Moving())StopMoving();if(ReadyToNextMission())NextMission();return 1;}
 if(PrimaryFacing.Current().GetValue<8>()!=64){if(!PrimaryFacing.IsRotating())Locomotor->Do_Turn(DirStruct(short(0x4000)));return 5;}
 if(!Unloading){Unloading=true;Animation.Value=0;Animation.Start(1);refinery->PlayNthAnim(BuildingAnimSlot::PreProduction,refinery->IsDamaged,false);MissionStatus=3;}
 else if(MissionStatus==3){
  if(Animation.Value>=RulesClass::Instance->HarvesterDumpRate*900.0){
   // 0x73E37E / 0x73E3BA: each dump interval starts chimney smoke and
   // the unloading animation before checking whether any resource remains.
   refinery->UpdateRefinerySmokeSystems();
   if(!refinery->Anims[static_cast<int>(BuildingAnimSlot::Special)])
    refinery->PlayNthAnim(BuildingAnimSlot::Special,refinery->IsDamaged,false);
   int slot=-1;for(int i=0;i<4;++i)if(Tiberium.GetAmount(i)>0){slot=i;break;}
   if(slot<0){if(refinery->Type->Refinery)refinery->PlayNthAnim(BuildingAnimSlot::Production,refinery->IsDamaged,false);MissionStatus=4;refinery->DestroyNthAnim(BuildingAnimSlot::Special);}
   else{
    const float amount=Tiberium.RemoveAmount(Tiberium.GetAmount(slot),slot);
    refinery->Owner->GiveTiberium(amount,slot);
    const float bonus=float(refinery->Owner->NumOrePurifiers)*RulesClass::Instance->PurifierBonus*amount;
    if(bonus>0)refinery->Owner->GiveTiberium(bonus,slot);
    Animation.Value=0;
   }
  }
  if(Destination&&QueuedMission!=Mission::None&&QueuedMission!=Mission::Harvest){
   if(refinery->Type->Refinery)refinery->PlayNthAnim(BuildingAnimSlot::Production,refinery->IsDamaged,false);
   MissionStatus=4;refinery->DestroyNthAnim(BuildingAnimSlot::Special);
  }
 }else if(MissionStatus==4&&!refinery->Anims[static_cast<int>(BuildingAnimSlot::Production)]){
  Unloading=false;SendToFirstLink(RadioCommand::NotifyUnlink);
  if(!Destination||QueuedMission==Mission::None||QueuedMission==Mission::Harvest)QueueMission(Mission::Harvest,false);
  if(ReadyToNextMission())NextMission();
 }
 return 1;
}
