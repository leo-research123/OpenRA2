// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 init.cpp DeployCommandClass/StopCommandClass/Init_Hotkeys
// and mainloop.cpp. YR 0x533D20, 0x55DEE0, 0x536C80, 0x730AF0,
// Stop.Execute 0x536B80 -> 0x730EA0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "player_commands.hpp"
#include "yrpp/CommandClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/VocClass.h"
#include "type_resources.hpp"
#include <cstring>

namespace game {
class StopCommandClass final : public CommandClass {
public:
 const char* GetName() const override {return "StopObject";}
 const wchar_t* GetUIName() const override {return StringTable::LoadString("TXT_STOP_OBJECT");}
 const wchar_t* GetUICategory() const override {return StringTable::LoadString("TXT_CONTROL");}
 const wchar_t* GetUIDescription() const override {return StringTable::LoadString("TXT_STOP_OBJECT_DESC");}
 void Execute(WWKey) const override {stop_selected_objects();}
};
class DeployCommandClass final : public CommandClass {
public:
 const char* GetName() const override {return "DeployObject";}
 const wchar_t* GetUIName() const override {return StringTable::LoadString("TXT_DEPLOY_OBJECT");}
 const wchar_t* GetUICategory() const override {return StringTable::LoadString("TXT_CONTROL");}
 const wchar_t* GetUIDescription() const override {return StringTable::LoadString("TXT_DEPLOY_OBJECT_DESC");}
 void Execute(WWKey) const override {deploy_selected_objects();}
};
bool load_player_hotkeys(CCINIClass& ini) noexcept {
 try {
  CommandClass::Hotkeys.Clear();
  if(auto* section=ini.GetSection("Hotkey"))for(auto* entry:section->Entries){
   const int key=ini.ReadInteger("Hotkey",entry->Key,0);
   for(auto* command:CommandClass::Array)if(!std::strcmp(command->GetName(),entry->Key)){
    if(key&&!CommandClass::Hotkeys.AddIndex(key,command))return false;
    break;
   }
  }
  return true;
 }catch(...){return false;}
}
bool initialize_player_commands() noexcept {
 try {
  static DeployCommandClass deploy;
  static StopCommandClass stop;
  if(CommandClass::Array.FindItemIndex(&deploy)<0&&!CommandClass::Array.AddItem(&deploy))return false;
  if(CommandClass::Array.FindItemIndex(&stop)<0&&!CommandClass::Array.AddItem(&stop))return false;
  CCFileClass file("KEYBOARDMD.INI");CCINIClass ini;
  if(!file.Exists()||ini.ReadCCFile(&file,false,false)<=0)return false;
  return load_player_hotkeys(ini);
 }catch(...){return false;}
}
bool dispatch_player_hotkey(WWKey input) noexcept {
 try {
  if(!int(input))return false;
  const int key=int(input)&~int(WWKey::Release);
  const int plain=key&~int(WWKey::Shift|WWKey::Ctrl|WWKey::Alt|WWKey::VirtualKey);
  CommandClass* command=nullptr;
  CommandClass::Hotkeys.TryGet(plain,command);
  if(!command||!command->PreventCombinationOverride(WWKey(key))){
   command=nullptr;CommandClass::Hotkeys.TryGet(key,command);
  }
  if(!command)return false;
  if(command->ExtraTriggerCondition(input))command->Execute(input);
  // Native device events arrive individually. 0x55DEE0's final virtual only
  // drains duplicate queued device events, and Deploy returns false there.
  command->CheckLoop55E020(input);
  return true;
 }catch(...){return false;}
}
void deploy_selected_objects() noexcept {
 try {
  auto& selected=ObjectClass::CurrentObjects;if(!selected.Count)return;
  bool done=false;
  auto* first=selected[0];
  if(first&&first->WhatAmI()==AbstractType::Building&&static_cast<BuildingClass*>(first)->GetOccupantCount()>0
     &&first->GetOwningHouse()->IsControlledByCurrentPlayer()){
   done=static_cast<BuildingClass*>(first)->ClickedEvent(EventType::Deploy);
  }else{
   bool deployed=false,undeployed=false;
   // YR's mixed selection deploys the undeployed members first; it does not
   // simultaneously undeploy the members that are already deployed.
   for(auto* object:selected)if(object){
    if(object->WhatAmI()==AbstractType::Infantry){
     auto* actor=static_cast<InfantryClass*>(object);if(!actor->Type->Deployer)continue;
     if(actor->IsDeployed())deployed=true;else undeployed=true;
    }else if(object->WhatAmI()==AbstractType::Unit){
     auto* unit=static_cast<UnitClass*>(object);if(!unit->Type->IsSimpleDeployer)continue;
     if(unit->Deployed||unit->Deploying)deployed=true;else undeployed=true;
    }
   }
   for(auto* object:selected)if(object&&(object->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None){
    auto* actor=static_cast<TechnoClass*>(object);
    if(!(actor->IsControllable()||actor->IsActive())||!actor->IsAlive||!actor->CanDeploySlashUnload()||actor->Berzerk)continue;
    // 0x700D10's vehicle gate is covered by CanDeploySlashUnload's stricter
    // tunnel/EMP rejection. Other original Techno kinds return true there.
    if(deployed&&undeployed){
     if(actor->WhatAmI()==AbstractType::Infantry&&static_cast<InfantryClass*>(actor)->IsDeployed())continue;
     if(actor->WhatAmI()==AbstractType::Unit){
      auto* unit=static_cast<UnitClass*>(actor);
      if(unit->Type->IsSimpleDeployer&&(unit->Deployed||unit->Deploying))continue;
     }
    }
    done=actor->ClickedEvent(EventType::Deploy)||done;
   }
  }
  if(done&&!type_resources().audio_unavailable&&RulesClass::Instance->DeploySound!=-1)
   VocClass::PlayGlobal(RulesClass::Instance->DeploySound,0x2000,1.0f,nullptr);
 }catch(...){/* Command callbacks must not unwind into the device boundary. */}
}
void stop_selected_objects() noexcept {
 try {
  auto& selected=ObjectClass::CurrentObjects;if(!selected.Count)return;
  // 0x730EA0 uses the same command admission as Deploy, without Deploy's
  // extra alive/deployer/berserk filters. Event execution checks live state.
  for(auto* object:selected)if(object&&(object->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None){
   auto* actor=static_cast<TechnoClass*>(object);
   if((actor->IsControllable()||actor->IsActive())&&!actor->ClickedEvent(EventType::Idle))return;
  }
  if(!type_resources().audio_unavailable&&RulesClass::Instance->StopSound!=-1)
   VocClass::PlayGlobal(RulesClass::Instance->StopSound,0x2000,1.0f,nullptr);
 }catch(...){/* Command callbacks must not unwind into the device boundary. */}
}
}
