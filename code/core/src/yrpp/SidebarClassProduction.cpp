// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 sidebar.cpp Strip/Select production; YR 0x6A6300/0x6A8420.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/SidebarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <cwchar>

bool YRPP_STDCALL BuildType::SortsBefore(AbstractType left,int li,AbstractType right,int ri){
 if(right==AbstractType::None)return true;
 const auto* a=TechnoTypeClass::GetByTypeAndIndex(left,li);const auto* b=TechnoTypeClass::GetByTypeAndIndex(right,ri);
 if(!a||!b)return false;
 auto* owner=HouseClass::CurrentPlayer;
 if(owner){const bool ownA=a->AIBasePlanningSide==owner->Type->SideIndex,ownB=b->AIBasePlanningSide==owner->Type->SideIndex;if(ownA!=ownB)return ownA;}
 // Original tab grouping: ground, aircraft, naval; then tech, price, name.
 const auto group=[](const TechnoTypeClass* t){return t->Naval?2:t->ConsideredAircraft?1:0;};
 if(group(a)!=group(b))return group(a)<group(b);
 if(a->TechLevel!=b->TechLevel)return a->TechLevel<b->TechLevel;
 const int ac=a->GetActualCost(owner),bc=b->GetActualCost(owner);if(ac!=bc)return ac<bc;
 return std::wcscmp(a->UIName?a->UIName:L"",b->UIName?b->UIName:L"")<=0;
}
bool SidebarClass::AddCameo(AbstractType kind,int index){
 const int tab=GetObjectTabIdx(kind,index,0);if(tab<0)return false;
 auto& strip=Tabs[tab];for(int i=0;i<strip.CameoCount;++i)if(strip.Cameos[i]==BuildType(index,kind))return false;
 if(strip.CameoCount==75)return false;
 int at=strip.CameoCount++;while(at>0&&BuildType::SortsBefore(kind,index,strip.Cameos[at-1].ItemType,strip.Cameos[at-1].ItemIndex)){strip.Cameos[at]=strip.Cameos[at-1];--at;}
 strip.Cameos[at]=BuildType(index,kind);strip.NeedsRedraw=true;TabButtons[tab].Enable();
 if(!Tabs[ActiveTabIndex].CameoCount)SetTab(tab);
 return true;
}
bool SidebarClass::LinkFactory(FactoryClass* factory,AbstractType kind,int index){
 const int tab=GetObjectTabIdx(kind,index,0);if(tab<0)return false;
 auto& strip=Tabs[tab];for(int i=0;i<strip.CameoCount;++i){auto& cameo=strip.Cameos[i];if(cameo.ItemType==kind&&cameo.ItemIndex==index){cameo.CurrentFactory=factory;strip.IsBuilding=strip.NeedsRedraw=true;return true;}}
 return false;
}
bool YRPP_STDCALL SidebarClass::UnlinkFactory(AbstractType kind,int index,FactoryClass* factory){
 const int tab=GetObjectTabIdx(kind,index,0);if(tab<0)return false;
 auto& strip=Instance.Tabs[tab];bool changed=false,building=false;
 for(int i=0;i<strip.CameoCount;++i){auto& cameo=strip.Cameos[i];
  if(cameo.CurrentFactory==factory){factory->AbandonProduction();cameo.CurrentFactory=nullptr;cameo.unknown_10=0;changed=true;}
  else if(cameo.CurrentFactory)building=true;
 }
 if(changed){strip.NeedsRedraw=true;Instance.SidebarNeedsRedraw=true;}
 if(!building)strip.IsBuilding=false;return changed;
}
void SidebarClass::RefreshBuildables(){
 auto* owner=HouseClass::CurrentPlayer;if(!owner)return;
 for(auto& strip:Tabs)for(int i=strip.CameoCount-1;i>=0;--i){
  const auto& cameo=strip.Cameos[i];auto* type=TechnoTypeClass::GetByTypeAndIndex(cameo.ItemType,cameo.ItemIndex);
  if(type&&(!type->FindFactory(true,false,false,owner)||owner->CanBuild(type,false,true)==CanBuildResult::Unbuildable)){
   for(int n=i;n+1<strip.CameoCount;++n)strip.Cameos[n]=strip.Cameos[n+1];--strip.CameoCount;strip.NeedsRedraw=true;
  }
 }
 const auto add=[&](auto& array){for(auto* type:array)if(type->TechLevel>=0&&type->FindFactory(true,false,false,owner)&&owner->CanBuild(type,false,true)!=CanBuildResult::Unbuildable)AddCameo(type->WhatAmI(),type->GetArrayIndex());};
 add(InfantryTypeClass::Array);add(UnitTypeClass::Array);
 for(int i=0;i<4;++i){if(Tabs[i].CameoCount)TabButtons[i].Enable();else TabButtons[i].Disable();}
}
void SidebarClass::UpdateProduction(){
 auto* owner=HouseClass::CurrentPlayer;if(!owner)return;
 RefreshBuildables();
 // A factory can be linked to several queued cameos; consume its change once.
 for(auto* factory:{owner->Primary_ForInfantry,owner->Primary_ForVehicles}){
  if(!factory||!factory->HasProgressChanged())continue;
  SidebarNeedsRedraw=true;
  if(factory->IsDone()&&factory->Object){
   auto* type=factory->Object->GetTechnoType();
   auto* building=type->FindFactory(false,true,false,owner);
   if(!building||building->HasAnyLink()||building->GetCurrentMission()==Mission::Unload||building->QueuedMission==Mission::Unload){factory->IsDifferent=true;continue;}
   EventClass event;
   event.Type=EventType::Place;event.HouseIndex=static_cast<char>(owner->ArrayIndex);event.Frame=Unsorted::CurrentFrame;
   event.Place={type->WhatAmI(),type->GetArrayIndex(),type->Naval,CellStruct::Empty};
   if(!EventClass::OutList.Add(event))factory->IsDifferent=true;
  }
 }
}
bool SelectClass::Action(GadgetFlag flags,DWORD* key,KeyModifier modifier){
 if(!Strip)return false;const int slot=Index+Strip->TopRowIndex*2;
 if(slot<0||slot>=Strip->CameoCount)return false;
 auto* owner=HouseClass::CurrentPlayer;const auto& cameo=Strip->Cameos[slot];
 auto* type=TechnoTypeClass::GetByTypeAndIndex(cameo.ItemType,cameo.ItemIndex);if(!owner||!type)return false;
 const auto bits=static_cast<unsigned>(flags);
 if(bits&(static_cast<unsigned>(GadgetFlag::LeftPress)|static_cast<unsigned>(GadgetFlag::RightPress))){
  auto* factory=owner->GetPrimaryFactory(cameo.ItemType,type->Naval,BuildCat(0));EventClass event;
  event.HouseIndex=static_cast<char>(owner->ArrayIndex);event.Frame=Unsorted::CurrentFrame;
  if(bits&static_cast<unsigned>(GadgetFlag::LeftPress)){
   if(owner->CanBuild(type,false,true)!=CanBuildResult::Buildable)return true;
   event.Type=EventType::Produce;event.Produce={cameo.ItemType,cameo.ItemIndex,type->Naval};
  }else{
   if(!factory||!factory->CountTotal(type))return true;
   const bool cancel=factory->IsSuspended||factory->IsQueued(type);
   event.Type=cancel?EventType::Abandon:EventType::Suspend;
   if(cancel)event.Abandon={cameo.ItemType,cameo.ItemIndex,type->Naval};else event.Suspend={cameo.ItemType,cameo.ItemIndex,type->Naval};
  }
  EventClass::OutList.Add(event);
 }
 return ControlClass::Action(flags,key,modifier);
}

// 0x6A5030 and Strip::Init_Clear 0x6A81B0: discard the previous scenario's
// cameos/factory links and scrolling state before reconnecting its buttons.
void SidebarClass::Init_Clear(){
 PowerClass::Init_Clear();RepairMode=SellMode=false;
 ToggleRepairButton.IsOn=ToggleSellButton.IsOn=false;ToggleRepairButton.IsPressed=ToggleSellButton.IsPressed=false;
 SidebarNeedsRedraw=true;unknown_bool_5514=unknown_bool_5515=false;
 for(auto& strip:Tabs){
  strip.IsScrollingDown=strip.IsScrolling=strip.IsBuilding=false;strip.Flasher=-1;
  strip.TopRowIndex=strip.Slid=strip.CameoCount=0;strip.NeedsRedraw=true;
  for(auto& cameo:strip.Cameos){cameo=BuildType(0,AbstractType::None);cameo.Progress.Start(0);}
 }
 ActiveTabIndex=0;unknown_53A0=0;
}
