// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp constructor; YR 0x7353C0 native object fields.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "map_world.hpp"
#include "target_registry.hpp"
namespace {DynamicVectorClass<UnitClass*> units;}
DynamicVectorClass<UnitClass*>& UnitClass::Array=units;
UnitClass::UnitClass(UnitTypeClass* type,HouseClass* owner) noexcept
 :FootClass(owner),CurrentFiringFrame(-1),Type(type),FollowerCar(nullptr),FlagHouseIndex(-1),IsFollowerCar(false),
 Unloading(false),IsHarvesting(false),TerrainPalette(false),unknown_int_6D4(-1),DeathFrameCounter(-1),ElectricBolt(nullptr),
 Deployed(false),Deploying(false),Undeploying(false),NonPassengerCount(0),ToolTipText{}{
 Array.AddItem(this);SecondaryFacing.SetCurrent(PrimaryFacing.Current());
 if(Type){
  PrimaryFacing.SetROT(Type->ROT);SecondaryFacing.SetROT(Type->ROT);
  Ammo=Type->InitialAmmo==-1?Type->Ammo:Type->InitialAmmo;Cloakable=Type->Cloakable;Health=EstimatedHealth=Type->Strength;
  if(Owner){Owner->AddTracking(this);if(Owner->Type->VeteranUnits.FindItemIndex(Type)>=0||(Owner->WarFactoryInfiltrated&&!Type->Naval&&Type->Trainable))Veterancy.SetVeteran();}
  // Techno.Init 0x6F40F8..0x6F4154 also arms vehicle parasites after Type
  // is assigned. GameCreate uses the checked game allocator; allocation failure
  // terminates locally rather than publishing an unusable actor or an exception.
  if(auto* weapon=GetWeapon(0)->WeaponType)if(weapon->Warhead&&weapon->Warhead->Parasite)
   ParasiteImUsing=GameCreate<ParasiteClass>(this);
 }
 ReloadTimer.Start(0);game::register_target_identity(*this);
}
UnitClass::~UnitClass(){
 if(Type&&Owner&&CountedAsOwned)Owner->RemoveTracking(this);
 IsAlive=false;NotifyObjectExpired(true);game::detach_map_object(*this);Array.Remove(this);game::unregister_target_identity(*this);
}
