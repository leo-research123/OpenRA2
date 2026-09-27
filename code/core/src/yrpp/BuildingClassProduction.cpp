// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Exit_Object/Find_Exit_Cell/Do_MISSION_UNLOAD.
// YR 0x443C60/0x44EFB0/0x44D880/0x449540. YR uses exit-list index 10,
// an X - 1 adjustment, and the rules ExitCoord (0x44F640), unlike TS.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "RulesClassReaders.hpp"
#include "map_world.hpp"
#include <cstring>
#include <cstdlib>
namespace {
CellStruct add(CellStruct a,CellStruct b){return {short(a.X+b.X),short(a.Y+b.Y)};}
CellStruct vehicle_exit(const BuildingClass& building){
 const auto base=building.GetMapCoords();
 const auto delta=building.Type->FoundationOutside?building.Type->FoundationOutside[10]:CellStruct{3,0};
 return {short(base.X+delta.X-1),short(base.Y+delta.Y)};
}
void start_factory_track(FootClass& unit,const BuildingClass& building,const CoordStruct& exit){
 // YR 0x0044DE84..0x0044E163 queries the actual locomotor, not the type's
 // default CLSID: idle chrono miners may already have restored Teleport.
 constexpr GUID persistIID{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
 constexpr GUID piggyIID{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
 IPersist* persist=nullptr;GUID clsid{};
 if(!unit.Locomotor||unit.Locomotor->QueryInterface(persistIID,reinterpret_cast<void**>(&persist))<0||!persist)std::abort();
 const auto identified=persist->GetClassID(&clsid);persist->Release();if(identified<0)std::abort();
 const auto is=[&](const GUID& id){return !std::memcmp(&clsid,&id,sizeof(id));};
 if(is(LocomotionClass::CLSIDs::Teleport)||is(LocomotionClass::CLSIDs::Tunnel)){
  // The original tests Is_Piggybacking here, not Is_Ok_To_End. Unwrap any
  // existing wrapper before placing Drive over the underlying locomotor.
  IPiggyback* old=nullptr;
  const auto queried=unit.Locomotor->QueryInterface(piggyIID,reinterpret_cast<void**>(&old));
  if(queried<0&&queried!=static_cast<HRESULT>(0x80004002u))std::abort();
  if(old&&old->Is_Piggybacking()){
#if defined(_MSC_VER)
   unit.Locomotor=nullptr;ILocomotion* restored=nullptr;
   if(old->End_Piggyback(&restored)<0)std::abort();unit.Locomotor.Attach(restored);
#else
   unit.Locomotor->Release();unit.Locomotor=nullptr;
   if(old->End_Piggyback(&unit.Locomotor)<0)std::abort();
#endif
  }
  if(old)old->Release();
  auto* drive=GameCreate<DriveLocomotionClass>();if(!drive)std::abort();
  drive->AddRef();
  if(drive->Link_To_Object(&unit)<0||drive->Begin_Piggyback(unit.Locomotor)<0)std::abort();
#if defined(_MSC_VER)
  unit.Locomotor=static_cast<ILocomotion*>(drive);
#else
  drive->AddRef();unit.Locomotor->Release();unit.Locomotor=drive;
#endif
  drive->Release();
 }else if(!is(LocomotionClass::CLSIDs::Drive)){
  // 0x0044DF1C: locomotors without a drive track receive a normal exit order.
  const auto cell=building.GetMapCoords();
  unit.SetDestination(MapClass::Instance.GetCellAt(CellStruct{short(cell.X+4),short(cell.Y+1)}),true);
  return;
 }
 unit.Locomotor->Force_Track(66,exit);
}
}
CellStruct BuildingClass::FindExitCell(FootClass* product,CellStruct preferred) const {
 const auto origin=GetMapCoords();auto& map=MapClass::Instance;
 const auto free=[&](CellStruct at,bool loco){auto* cell=map.TryGetCellAt(at);return cell&&map.IsWithinUsableArea(at,true)&&product->IsCellOccupied(cell,FacingType(-1),-1,nullptr,loco)==Move::OK;};
 const CellStruct special=Type->GDIBarracks?CellStruct{1,2}:Type->NODBarracks?CellStruct{2,2}:Type->YuriBarracks?CellStruct{2,1}:CellStruct::Empty;
 if(special!=CellStruct::Empty&&free(add(origin,special),true))return add(origin,special);
 if(preferred!=CellStruct::Empty&&free(preferred,false))return preferred;
 if(Type->FoundationOutside&&!Type->Hospital){
  for(auto* p=Type->FoundationOutside;*p!=CellStruct{0x7FFF,0x7FFF};++p)if(free(add(origin,*p),false))return add(origin,*p);
 }else{
  const int w=Type->GetFoundationWidth(),h=Type->GetFoundationHeight(false);
  for(int x=-1;x<=w;++x){auto at=add(origin,{short(x),short(h)});if(free(at,true))return at;at=add(origin,{short(x),-1});if(free(at,true))return at;}
  for(int y=-1;y<=h;++y){auto at=add(origin,{short(w),short(y)});if(free(at,true))return at;at=add(origin,{-1,short(y)});if(free(at,true))return at;}
 }
 return CellStruct::Empty;
}
KickOutResult BuildingClass::KickOutUnit(TechnoClass* product,CellStruct preferred){
 if(!product||(product->AbstractFlags&::AbstractFlags::Foot)==::AbstractFlags::None)return KickOutResult::Failed;
 auto* foot=static_cast<FootClass*>(product);foot->SetArchiveTarget(ArchiveTarget);
 if(Type->WeaponsFactory&&!Type->Naval){
  if(GetCurrentMission()==Mission::Unload||QueuedMission==Mission::Unload){
   for(auto* other:Owner->Buildings)if(other!=this&&other->Type==Type&&other->GetCurrentMission()==Mission::Guard&&!other->Factory)return other->KickOutUnit(product,CellStruct::Empty);
   return KickOutResult::Busy;
  }
  // 0x44F640: nonzero ExitCoord is relative to Location, not GetCoords' center.
  const auto at=Type->ExitCoord!=CoordStruct::Empty?Location+Type->ExitCoord:GetCoords();
  ++Unsorted::ScenarioInit;const bool placed=foot->Unlimbo(at,DirType::East);--Unsorted::ScenarioInit;
  if(!placed)return KickOutResult::Failed;
  foot->Mark(MarkType::Up);foot->SetLocation(at);foot->Mark(MarkType::Down);
  SendCommand(RadioCommand::RequestLink,foot);SendCommand(RadioCommand::RequestTether,foot);
  QueueMission(Mission::Unload,false);NextMission();UpdateTimer.Start(0);
 }else{
  if(HasAnyLink())return KickOutResult::Busy;
  const auto exit=FindExitCell(foot,preferred);if(exit==CellStruct::Empty)return KickOutResult::Failed;
  const auto origin=GetMapCoords();auto inside=exit;
  if(inside.X>=origin.X+Type->GetFoundationWidth())--inside.X;else if(inside.X<origin.X)++inside.X;
  if(inside.Y>=origin.Y+Type->GetFoundationHeight(false))--inside.Y;else if(inside.Y<origin.Y)++inside.Y;
  CoordStruct at{inside.X*256+128,inside.Y*256+128,0};
  if((Type->GDIBarracks&&exit==add(origin,{1,2}))||(Type->NODBarracks&&exit==add(origin,{2,2}))||(Type->YuriBarracks&&exit==add(origin,{2,1})))at+=Type->ExitCoord;
  auto* cell=MapClass::Instance.GetCellAt(exit);DirStruct facing;GetDirectionTo(&facing,cell);
  ++Unsorted::ScenarioInit;const bool placed=foot->Unlimbo(at,static_cast<DirType>(facing.GetValue<8>()));--Unsorted::ScenarioInit;
  if(!placed)return KickOutResult::Failed;
  foot->QueueMission(Mission::Move,false);foot->SetDestination(cell,true);
  if(SendCommand(RadioCommand::RequestLink,foot)==RadioCommand::AnswerPositive)SendCommand(RadioCommand::RequestTether,foot);
 }
 if(LogicClass::Instance.FindItemIndex(foot)<0)LogicClass::Instance.AddObject(foot,false);
 game::map_object_changed();return KickOutResult::Succeeded;
}
bool BuildingClass::ClearFactoryBib(){
 if(!Type->WeaponsFactory)return false;
 auto* cell=MapClass::Instance.GetCellAt(vehicle_exit(*this));bool blocked=false;
 for(auto* object=cell->FirstObject;object;object=object->NextObject)if(object!=this&&object!=GetNthLink()){blocked=true;break;}
 if(!blocked)return false;
 cell->ScatterContent(CoordStruct::Empty,true,true,false);
 const auto from=cell->GetCoords();for(int i=0;i<8;++i)cell->GetNeighbourCell(static_cast<FacingType>(i))->ScatterContent(from,true,true,false);
 return true;
}
int BuildingClass::MissionFactoryUnload(){
 auto* unit=HasAnyLink()?static_cast<FootClass*>(GetNthLink()):nullptr;
 switch(MissionStatus){
 case 0:
  if(unit){unit->QueueMission(Mission::Guard,false);unit->NextMission();}
  UnloadTimer.StartTimer11(Type->DeployTime);MissionStatus=1;NeedsRedraw=true;
  PlayNthAnim(BuildingAnimSlot::Production,IsDamaged,false);break;
 case 1:if(!ClearFactoryBib())MissionStatus=2;break;
 case 2:
  if(UnloadTimer.AreStates01()){
   if(unit){
    unit->QueueMission(Mission::Move,false);unit->NextMission();
    const auto at=vehicle_exit(*this);CoordStruct coord{at.X*256+128,at.Y*256+128,0};
    start_factory_track(*unit,*this,coord);unit->SetSpeedPercentage(0.5);MissionStatus=3;
   }else{UnloadTimer.StartTimer10(Type->DeployTime);MissionStatus=4;}
  }break;
 case 3:if(!IsTether){UnloadTimer.StartTimer10(Type->DeployTime);MissionStatus=4;}break;
 case 4:if(UnloadTimer.AreStates00()){QueueMission(Mission::Guard,false);NextMission();NeedsRedraw=true;DestroyNthAnim(BuildingAnimSlot::Production);}break;
 default:break;
 }
 game::map_object_changed();return rule_integer(CurrentMissionControl()->Rate*900.0)+ScenarioClass::Instance->Random.RandomRanged(0,2);
}

// Ground factory arms of YR 0x43C2D0 / OpenTS Receive_Message.
RadioCommand BuildingClass::ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data){
 if(Type->DockUnload&&sender&&sender->WhatAmI()==AbstractType::Unit&&static_cast<UnitClass*>(sender)->Type->Harvester){
  switch(command){
  case RadioCommand::QueryCanEnter:
   if(!Owner->IsAlliedWith(sender))return RadioCommand::AnswerInvalid;
   if(InLimbo||!IsAlive||!HasPower||GetCurrentMission()==Mission::Construction||GetCurrentMission()==Mission::Selling||(!Unsorted::ScenarioInit&&!HasFreeLink(sender)))return RadioCommand::AnswerNegative;
   return RadioCommand::AnswerPositive;
  case RadioCommand::RequestLoading:{
   if(!HasPower||!HasFreeLink(sender))return RadioCommand::AnswerNegative;
   if(!ContainsLink(sender)&&SendCommand(RadioCommand::RequestLink,sender)!=RadioCommand::AnswerPositive)return RadioCommand::AnswerNegative;
   const auto at=GetMapCoords();data=MapClass::Instance.GetCellAt(CellStruct{short(at.X+3),short(at.Y+1)});
   if(SendCommandWithData(RadioCommand::RequestMoveTo,data,sender)==RadioCommand::AnswerAwaiting){
    SendCommand(RadioCommand::RequestTether,sender);SendCommand(RadioCommand::RequestDockRefinery,sender);
   }
   return RadioCommand::AnswerPositive;
  }
  case RadioCommand::RequestCompleteEnter:
   sender->QueueMission(Mission::Unload,false);return RadioCommand::AnswerPositive;
  default:break;
  }
 }
 if(command==RadioCommand::NotifyUnloaded&&Type->WeaponsFactory){
  TechnoClass::ReceiveCommand(sender,command,data);return RadioCommand::AnswerLeave;
 }
 return TechnoClass::ReceiveCommand(sender,command,data);
}
