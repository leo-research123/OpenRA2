// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Repair / Repair_AI / Sell_Back /
// Can_Demolish / Do_MISSION_DECONSTRUCTION, calibrated to the YR entries
// declared in BuildingClass.h. Copyright Electronic Arts Inc. / OpenTS
// contributors; EA Section 7 terms: code/third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "RulesClassReaders.hpp"
#include "map_world.hpp"
#include "type_resources.hpp"
#include <algorithm>
#include <new>

bool BuildingClass::CanBeRepaired() const {
 return Health && Type->ClickRepairable && !IsStrange() && Type->Repairable && Health!=Type->Strength;
}
bool BuildingClass::CanBeSold() const {
 if(ForceShielded || Type->Unsellable)return false;
 if(HasBuildUp && BState && GetCurrentMission()!=Mission::Selling && GetCurrentMission()!=Mission::Construction)return true;
 return Type->ToTile && !Owner->Defeated;
}
void BuildingClass::SetRepairState(int state) {
 if(state==-1)IsBeingRepaired=!IsBeingRepaired;
 else if(state==0){if(!IsBeingRepaired)return;IsBeingRepaired=false;}
 else if(state==1){if(IsBeingRepaired)return;IsBeingRepaired=true;}
 auto& rules=*RulesClass::Instance;int sound=rules.GenericClick;
 if(IsBeingRepaired) {
  if(Health==Type->Strength)sound=rules.ScoldSound;
  else {
   if(Owner->IsControlledByCurrentPlayer())Flash(7);
   NeedsRepairs=true;
   if(Owner->IsControlledByCurrentPlayer()&&!game::type_resources().audio_unavailable)VoxClass::Play("EVA_Repairing",-1,-1);
  }
 }
 if(Owner->IsControlledByCurrentPlayer()&&!game::type_resources().audio_unavailable)VocClass::PlayAt(sound,Location);
 NeedsRedraw=true;game::map_object_changed();
}
void BuildingClass::UpdateRepair() {
 // Player repair arm of 0x450630. AI deciding when to start repair/sell is a
 // separate decision; a manual order uses this same original repair state.
 if(!IsBeingRepaired)return;
 const int interval=rule_integer(RulesClass::Instance->RepairRate*900.0);
 if(interval<=0 || Unsorted::CurrentFrame%interval)return;
 NeedsRepairs=!NeedsRepairs;
 const int cost=Type->GetRepairStepCost(),step=Type->GetRepairStep();
 if(Owner->Available_Money()<cost)IsBeingRepaired=false;
 else {
  Owner->TakeMoney(cost);Health+=step;EstimatedHealth+=step;
  if(Health>=Type->Strength){Health=EstimatedHealth=Type->Strength;IsBeingRepaired=false;}
  ToggleDamagedAnims(GetHealthPercentage()<=RulesClass::Instance->ConditionYellow);
  UpdateDamageFires();Owner->RecheckPower=Owner->RecheckRadar=true;
 }
 NeedsRedraw=true;game::map_object_changed();
}
void BuildingClass::Sell(int control) {
 if(HasBuildUp) {
  const auto mission=GetCurrentMission();
  if((control==0 && mission!=Mission::Selling)||(control==1 && (mission==Mission::Selling || C4Applied)))return;
  if((control==-1 || control==1) && mission!=Mission::Selling) {
   QueueMission(Mission::Selling,false);NextMission();
   LogicClass::Instance.AddObject(this,false);
  }
  if(Owner->IsControlledByCurrentPlayer())VocClass::PlayGlobal(RulesClass::Instance->GenericClick,0x2000,1.0f);
 } else if(Type->ToTile) {
  // Pavement conversion is separate from the buildup/refund mission.
  (void)Type->GetActualCost(Owner);(void)Owner->IsControlledByHuman();Limbo();delete this;
 }
}
#if !defined(RA2_YRPP_GAME)
int BuildingClass::GetCrewCount() const {
 if(NoCrew || !Type->Crewed)return 0;
 const auto& rules=*RulesClass::Instance;
 int divisor=Owner->SideIndex==0?rules.AlliedSurvivorDivisor:Owner->SideIndex==1?rules.SovietSurvivorDivisor:
     Owner->SideIndex==2?rules.ThirdSurvivorDivisor:0;
 if(!divisor)return 0;
 if(HasBeenCaptured)divisor*=2;
 return std::clamp(Type->GetRefund(Owner,false)/divisor,1,5);
}
#endif
int BuildingClass::Mission_Selling() {
 // TODO(BUILDING-SELL-SPECIAL): complete the 0x449C30 undeploy conversion,
 // specialized upgrade/passenger/kill bookkeeping and crew-selection arms.
 // This implementation covers ordinary building demolition through this
 // original mission state, including reverse buildup and final retirement.
 SetRepairState(0);
 if(MissionStatus==0) {
  if(UpgradeLevel>0) {
   const int slot=UpgradeLevel-1;auto* upgrade=Upgrades[slot];
   if(upgrade)Owner->GiveMoney(upgrade->GetRefund(Owner,false));
   DestroyNthAnim(static_cast<BuildingAnimSlot>(slot));Upgrades[slot]=nullptr;--UpgradeLevel;
   Owner->RecheckPower=Owner->RecheckRadar=true;
   QueueMission(Mission::Guard,false);NextMission();UpdateAnimations();
   return 1;
  }
  IsReadyToCommence=false;SendToEachLink(RadioCommand::NotifyLeave);
  for(auto*& fire:DamageFireAnims){delete fire;fire=nullptr;}
  MissionStatus=1;
 } else if(MissionStatus==1) {
  SendToEachLink(RadioCommand::NotifyUnlink);
  if(IsTether)return 1;
  if(Occupants.Count)UnloadOccupants(true,true);
  if(!ArchiveTarget || !Type->UndeploysInto) {
   const int count=GetCrewCount();const auto* foundation=GetFoundationData(false);
   int cells=0;while(foundation && foundation[cells]!=CellStruct{0x7FFF,0x7FFF})++cells;
   bool engineer=false;
   for(int i=0;i<count && cells;++i) {
    auto* crew=GetCrew();if(!crew)continue;
    if(crew->Engineer && engineer)continue;
    engineer=engineer||crew->Engineer;
    auto* infantry=new(std::nothrow) InfantryClass(crew,Owner);if(!infantry)continue;
#if !defined(RA2_YRPP_GAME)
    // Native Infantry construction leaves COM locomotion to its creation
    // caller; placement/occupation queries already require that driver.
    if(!infantry->InitializeLocomotor()){delete infantry;continue;}
#endif
    const auto offset=foundation[ScenarioClass::Instance->Random.RandomRanged(0,cells-1)];
    auto at=GetMapCoords();at.X+=offset.X;at.Y+=offset.Y;
    auto* cell=MapClass::Instance.TryGetCellAt(at);CoordStruct where{int(at.X)*256+128,int(at.Y)*256+164,Location.Z};
    if(cell)where=cell->FindInfantrySubposition(where,false,false,false);
    ++Unsorted::ScenarioInit;
    const bool placed=cell && where!=CoordStruct::Empty && infantry->Unlimbo(where,DirType::North);
    --Unsorted::ScenarioInit;
    if(!placed)delete infantry;
    else {infantry->Scatter(CoordStruct::Empty,true,false);infantry->QueueMission(Mission::Move,false);}
   }
  }
  if(Owner->IsControlledByCurrentPlayer()&&!IsStrange()&&!game::type_resources().audio_unavailable)VocClass::PlayAt(RulesClass::Instance->SellSound,Location);
  MissionStatus=2;BeginMode(BStateType::Construction);IsReadyToCommence=false;
 } else if(MissionStatus==2 && IsReadyToCommence) {
  auto* house=Owner;house->RecheckTechTree=true;
  SetTarget(nullptr);
  if(Owner->IsControlledByCurrentPlayer()&&!game::type_resources().audio_unavailable)VoxClass::Play("EVA_StructureSold",-1,-1);
  house->GiveMoney(GetRefund());
  Limbo();
  float* stored[]{&Tiberium.Tiberium1,&Tiberium.Tiberium2,&Tiberium.Tiberium3,&Tiberium.Tiberium4};
  for(int i=0;i<4;++i)if(*stored[i]>0){const float amount=*stored[i];*stored[i]=0;house->GiveTiberium(float(rule_integer(amount)),i);}
  IsAlive=false;ActuallyPlacedOnMap=false;
  game::detach_map_object(*this);house->RecheckPower=house->RecheckRadar=true;
 }
 NeedsRedraw=true;game::map_object_changed();return 1;
}
