// Native map building lifecycle; original 0x0043B740.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/FactoryClass.h"
#include "map_world.hpp"
#include "target_registry.hpp"
#include <algorithm>
namespace {DynamicVectorClass<BuildingClass*> objects;}
// OpenTS Detach_All; YR 0x44EBF0. Production dependencies are only reached
// when a real FactoryClass owns a product, never synthesized by the map host.
void BuildingClass::Disappear(bool permanently) {
 if(permanently) {
  if(Factory){Factory->AbandonProduction();GameDelete(Factory);Factory=nullptr;}
  if(Owner) {
   const auto abandon=[&](FactoryClass* factory) {
    if(!factory)return;auto* product=factory->Object;const bool saved=InLimbo;InLimbo=true;
    if(product&&!product->GetTechnoType()->FindFactory(true,false,false,Owner))
     Owner->AbandonProduction(Type->Factory,product->GetTechnoType()->GetArrayIndex(),false,false);
    InLimbo=saved;
   };
   if(Type->Factory==AbstractType::BuildingType) {
    abandon(Owner->Primary_ForBuildings);abandon(Owner->Primary_Unused1);abandon(Owner->Primary_Unused2);
    abandon(Owner->Primary_Unused3);abandon(Owner->Primary_ForDefenses);
   }else abandon(Owner->GetPrimaryFactory(Type->Factory,Type->Naval,BuildCat(0)));
  }
  SendToEachLink(RadioCommand::NotifyUnlink);
 }else for(int i=0;i<RadioLinks.Capacity;++i)if(auto* link=GetNthLink(i);link&&!Owner->IsAlliedWith(link))SendCommand(RadioCommand::NotifyUnlink,link);
 ObjectClass::Disappear(permanently);
}
DynamicVectorClass<BuildingClass*>& BuildingClass::Array=objects;
BuildingClass::BuildingClass(BuildingTypeClass*pType,HouseClass*pOwner) noexcept
 : TechnoClass(pOwner),
 Type{},
 Factory{},
 C4Timer{},
 BState{},
 QueueBState{},
 OwnerCountryIndex{},
 C4AppliedBy{},
 unknown_544{},
 FirestormAnim{},
 PsiWarnAnim{},
 FactoryRetryTimer{},
 Anims{},
 AnimStates{},
 align_5C5{},
 DamageFireAnims{},
 RequiresDamageFires{},
 Upgrades{},
 FiringSWType{},
 unknown_5FC{},
 Spotlight{},
 GateTimer{},
 LightSource{},
 LaserFenceFrame{},
 FirestormWallFrame{},
 RepairProgress{},
 unknown_rect_63C{},
 unknown_coord_64C{},
 unknown_int_658{},
 unknown_65C{},
 HasPower{},
 IsOverpowered{},
 RegisteredAsPoweredUnitSource{},
 SupportingPrisms{},
 HasExtraPowerBonus{},
 HasExtraPowerDrain{},
 Overpowerers{},
 Occupants{},
 FiringOccupantIndex{},
 Audio7{},
 Audio8{},
 WasOnline{},
 ShowRealName{},
 BeingProduced{},
 ShouldRebuild{},
 HasEngineer{},
 CashProductionTimer{},
 AI_Sellable{},
 IsReadyToCommence{},
 NeedsRepairs{},
 C4Applied{},
 NoCrew{},
 unknown_bool_6E1{},
 unknown_bool_6E2{},
 HasBeenCaptured{},
 ActuallyPlacedOnMap{},
 unknown_bool_6E5{},
 IsDamaged{},
 IsFogged{},
 IsBeingRepaired{},
 HasBuildUp{},
 StuffEnabled{},
 HasCloakingData{},
 CloakRadius{},
 Translucency{},
 StorageFilledSlots{},
 SecretProduction{},
 ColorAdd{},
 unknown_int_6FC{},
 unknown_short_700{},
 UpgradeLevel{},
 GateStage{},
 PrismStage{},
 PrismTargetCoords{},
 DelayBeforeFiring{},
 TankBunkerState{} {
 Type=pType;Health=EstimatedHealth=pType?std::max(pType->Strength,1):1;BState=static_cast<int>(BStateType::Idle);QueueBState=static_cast<int>(BStateType::None);
 if(Type)Ammo=Type->InitialAmmo==-1?Type->Ammo:Type->InitialAmmo;
 HasBuildUp=Type&&Type->Buildup;
 HasPower=WasOnline=StuffEnabled=true;ShowRealName=true;ActuallyPlacedOnMap=false;OwnerCountryIndex=0xFFFFFFFFu;
 if(pType){DirStruct pitch;pitch.Raw=WORD((pType->BarrelStartPitch&255)<<8);BarrelFacing.SetCurrent(pitch);}
 Array.AddItem(this);if(Owner)Owner->Buildings.AddItem(this);
 game::register_target_identity(*this);
 if(Type&&Owner)Owner->AddTracking(this);
}
BuildingClass::~BuildingClass(){if(Owner&&CountedAsOwned)Owner->RemoveTracking(this);IsAlive=false;NotifyObjectExpired(true);game::detach_map_object(*this);delete Spotlight;Spotlight=nullptr;delete LightSource;LightSource=nullptr;for(int i=0;i<21;++i)DestroyNthAnim(static_cast<BuildingAnimSlot>(i));for(auto&a:DamageFireAnims){delete a;a=nullptr;}if(Owner)Owner->Buildings.Remove(this);Array.Remove(this);
 game::unregister_target_identity(*this);
}
AbstractType BuildingClass::WhatAmI() const{return AbsID;}
int BuildingClass::Size() const{return sizeof(*this);}
ObjectTypeClass* BuildingClass::GetType() const{return Type;}
