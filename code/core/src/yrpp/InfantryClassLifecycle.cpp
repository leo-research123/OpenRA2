// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp constructor/Init, YR 0x00517A50 / 0x00517CC0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Actual original class and registries; native presentation is not full combat initialization.
#include "yrpp/InfantryClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Unsorted.h"
#include "target_registry.hpp"
#include "map_world.hpp"
#include <cstring>
#include <cstdlib>
namespace { DynamicVectorClass<InfantryClass*> infantry; }
DynamicVectorClass<InfantryClass*>& InfantryClass::Array=infantry;
bool InfantryClass::Limbo() {
 Locomotor->Stop_Movement_Animation();
 unknown_int_6E8=2;Crawling=false;SequenceAnim=Sequence::Ready;
 return FootClass::Limbo();
}
bool InfantryClass::InitializeLocomotor() noexcept {
 if(Locomotor)return true;
 if(!Type || std::memcmp(&Type->Locomotor,&LocomotionClass::CLSIDs::Walk,sizeof(GUID)))return false;
 try {
  auto* driver=GameCreate<WalkLocomotionClass>();if(!driver)return false;
  // One retained ILocomotion reference, matching the constructor's COM query.
  if(driver->Link_To_Object(this)<0){GameDelete(driver);return false;}
#if defined(_MSC_VER)
  Locomotor=static_cast<ILocomotion*>(driver);
#else
  driver->AddRef();Locomotor=driver;
#endif
  return true;
 }catch(...){return false;}
}
InfantryClass::InfantryClass(InfantryTypeClass* type,HouseClass* owner) noexcept
 : FootClass(owner),Type{type},SequenceAnim{Sequence::Nothing},unknown_Timer_6C8{},PanicDurationLeft{},PermanentBerzerk{},
 Technician{},unknown_bool_6DA{},Crawling{},unknown_bool_6DC{},unknown_bool_6DD{},unknown_6E0{},ShouldDeploy{},
 unknown_int_6E8{2},unused_6EC{} {
 if(type){Health=EstimatedHealth=type->Strength;Ammo=type->InitialAmmo==-1?type->Ammo:type->InitialAmmo;Cloakable=type->Cloakable;}
 PrimaryFacing.SetROT(127);
 // Weapon manager arms of Techno initialization 0x6F3F40, called by the
 // original Infantry.Init 0x517CC0 after Type has been assigned.
 if(type)if(auto* weapon=GetWeapon(0)->WeaponType)if(weapon->Warhead){
  if(weapon->Warhead->MindControl)CaptureManager=GameCreate<CaptureManagerClass>(this,weapon->Damage,weapon->InfiniteMindControl);
  if(weapon->Warhead->Parasite)ParasiteImUsing=GameCreate<ParasiteClass>(this);
 }
 if(!Array.AddItem(this))std::abort();
 game::register_target_identity(*this);
 if(Type&&Owner)Owner->AddTracking(this);
}
InfantryClass::~InfantryClass(){
 if(Team)Team->LiberateMember(this);
 if(CountedAsOwned&&Owner&&Type)Owner->RemoveTracking(this);
 // The native lifecycle has no transport/team finalizer yet. Mark the final
 // removal before notification so Capture cannot retain a dying destination.
 IsAlive=false;NotifyObjectExpired(true);
 game::detach_map_object(*this);Array.Remove(this);game::unregister_target_identity(*this);
}
HRESULT YRPP_STDCALL InfantryClass::GetClassID(CLSID* out) {
 if(!out)return static_cast<HRESULT>(0x80004003u);
 constexpr std::uint32_t words[]{0x0E272DC4,0x11D19C0F,0xA00009B7,0xD1AFDD24};
 std::memcpy(out,words,sizeof(words));return 0;
}
AbstractType InfantryClass::WhatAmI() const{return AbsID;}
int InfantryClass::Size() const{return sizeof(*this);}
bool InfantryClass::Unlimbo(const CoordStruct& where,DirType facing){
 const int floor=MapClass::Instance.GetCellFloorHeight(where);auto at=where;
 if(at.Z==floor){
  const bool ignoreContents=Unsorted::ScenarioInit||!MapClass::Instance.IsWithinUsableArea(CellClass::Coord2Cell(where),true);
  at=MapClass::Instance.GetCellAt(where)->FindInfantrySubposition(where,ignoreContents,false,false);
  if(at==CoordStruct::Empty)return false;at.Z=floor;
 }
 if(!FootClass::Unlimbo(at,facing))return false;
 if(!Type->Sight)DiscoveredByCurrentPlayer=false;
 // 0xA8F234 is initialized at 0x5179B0 to four level heights.
 if(at.Z<=floor+4*Unsorted::LevelHeight)MarkAllOccupationBits(at);
 unknown_int_6E8=2;return true;
}
ObjectTypeClass* InfantryClass::GetType() const{return Type;}
bool InfantryClass::IsDeployed() const {
 return Type&&Type->Deployer&&SequenceAnim>=Sequence::Deploy&&SequenceAnim<=Sequence::DeployedIdle;
}
ObjectClass* InfantryTypeClass::CreateObject(HouseClass* owner) {
 return GameCreate<InfantryClass>(this,owner);
}

// OpenTS infantry.cpp Paradrop; YR 0x521760 adds the parachute sequence.
bool InfantryClass::SpawnParachuted(const CoordStruct& at){
 if(!ObjectClass::SpawnParachuted(at))return false;
 QueueMission(Owner->IsControlledByHuman()?Mission::Guard:Mission::Hunt,false);
 PlayAnim(Sequence::Paradrop,true,false);
 return true;
}
