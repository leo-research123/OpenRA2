// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp What_Action / Active_Click_With.
// YR 0x447210/0x447540/0x443410/0x4436F0: stationary and garrison input.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/SessionClass.h"
#include "scenario_runtime.hpp"

bool BuildingClass::IsUnitFactory() const {
 return Type->Factory==AbstractType::UnitType||Type->Factory==AbstractType::InfantryType||Type->Cloning||Type->UnitRepair;
}
bool BuildingClass::IsControllable() const {
 if(IsUnderEMP())return false;
 if(IsUnitFactory())return true;
 if(Type->ConstructionYard){
  const auto& runtime=game::scenario_runtime();const auto* session=runtime.houses?runtime.houses->session:nullptr;
  if(!Owner->IsHumanPlayer||!session||session->GameMode==GameMode::Campaign||!session->Config.MCVRedeploy||MindControlledBy)return false;
 }
 return Type->UndeploysInto!=nullptr;
}
Action BuildingClass::MouseOverObject(const ObjectClass* target,bool ignoreForce) const {
 if(!target||Type->InvisibleInGame||(target->WhatAmI()==AbstractType::Building&&static_cast<const BuildingClass*>(target)->Type->InvisibleInGame))return Action::None;
 auto action=TechnoClass::MouseOverObject(target,ignoreForce);
 if(action==Action::ToggleSelect)return action;
 if(action==Action::Self_Deploy){
  if(GetOccupantCount()>0||((Type->InfantryAbsorb||Type->UnitAbsorb)&&Passengers.NumPassengers>0))return Action::Self_Deploy;
  // Factory-primary/rally and selling inputs have no native producer yet.
  return Action::None;
 }
 if(action==Action::Attack){
  if(auto* weapon=GetWeapon(0)->WeaponType)
   if(!weapon->Projectile->AG||Type->EMPulseCannon||CurrentMission==Mission::Selling)return Action::None;
  if(target->GetType()->Immune)return Action::NoMove;
 }
 if((action==Action::Move||action==Action::NoMove)&&!IsControllable())return Action::Select;
 return action;
}
Action BuildingClass::MouseOverCell(const CellStruct* cell,bool checkFog,bool ignoreForce) const {
 if(Type->InvisibleInGame)return Action::None;
 const auto action=TechnoClass::MouseOverCell(cell,checkFog,ignoreForce);
 if(action==Action::Move&&!IsControllable())return Action::None;
 if(action==Action::Attack)if(auto* weapon=GetWeapon(0)->WeaponType)
  if(!weapon->Projectile->AG||Type->EMPulseCannon||CurrentMission==Mission::Selling)return Action::None;
 return action;
}
bool BuildingClass::ObjectClickedAction(Action action,ObjectClass* target,bool) {
 if(action==Action::Attack){if(target)ClickedMission(Mission::Attack,target,nullptr,nullptr);return true;}
 if(action==Action::Self_Deploy&&GetOccupantCount()>0){ClickedMission(Mission::Unload,target,nullptr,nullptr);return true;}
 return false;
}
bool BuildingClass::CellClickedAction(Action action,CellStruct* cell,CellStruct*,bool) {
 if(action==Action::Attack){ClickedMission(Mission::Attack,MapClass::Instance.GetCellAt(*cell),nullptr,nullptr);return true;}
 return false;
}
