// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp ground movement and order admission;
// YR 0x7360C0 / 0x738970 / 0x744270 / 0x7441B0 / 0x744210.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/UnitClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/WeaponTypeClass.h"
#include <cstring>

bool UnitClass::InitializeLocomotor() noexcept {
    if(Locomotor)return true;
    if(!Type)return false;
    const bool teleport=!std::memcmp(&Type->Locomotor,&LocomotionClass::CLSIDs::Teleport,sizeof(GUID));
    if(std::memcmp(&Type->Locomotor,&LocomotionClass::CLSIDs::Drive,sizeof(GUID))&&!(teleport&&Type->Harvester))return false;
    try {
        auto* driver=GameCreate<DriveLocomotionClass>();if(!driver)return false;
        if(driver->Link_To_Object(this)<0){GameDelete(driver);return false;}
        if(teleport){
            auto* base=GameCreate<TeleportLocomotionClass>();
            if(!base){GameDelete(driver);return false;}
            base->Link_To_Object(this);driver->Begin_Piggyback(base);
        }
#if defined(_MSC_VER)
        Locomotor=static_cast<ILocomotion*>(driver);
#else
        driver->AddRef();Locomotor=driver;
#endif
        return true;
    }catch(...){return false;}
}
// OpenTS Captured; YR 0x7463A0 preserves the follower-car chain while
// applying the same original Foot ownership transition to each vehicle.
bool UnitClass::SetOwningHouse(HouseClass* house,bool){
 if(house==Owner)return false;
 auto* follower=FollowerCar;IsFollowerCar=false;
 if(follower){follower->SetOwningHouse(house,true);FollowerCar=follower;follower->IsFollowerCar=true;}
 return FootClass::SetOwningHouse(house,true);
}
void UnitClass::MarkAllOccupationBits(const CoordStruct& at) {
    auto* cell=MapClass::Instance.GetCellAt(at);
    const bool alt=MapClass::Instance.GetCellFloorHeight(at)+CellClass::BridgeHeight<=at.Z&&(unsigned(cell->Flags)&0x100u);
    (alt?cell->AltOccupationFlags:cell->OccupationFlags)|=0x20u;
}
void UnitClass::UnmarkAllOccupationBits(const CoordStruct& at) {
    auto* cell=MapClass::Instance.GetCellAt(at);
    const bool alt=MapClass::Instance.GetCellFloorHeight(at)+CellClass::BridgeHeight<=at.Z;
    (alt?cell->AltOccupationFlags:cell->OccupationFlags)&=~0x20u;
}
bool UnitClass::ReadyToNextMission() const {
    if(CurrentMission==Mission::Sticky||CurrentMission==Mission::Rescue||Deploying||Undeploying||Unloading)return false;
    if(QueuedMission!=Mission::Enter&&Locomotor&&Locomotor->Is_Moving_Now()&&GetHeight()>=0
        &&GetCurrentMission()!=Mission::Guard&&(GetCurrentMission()!=Mission::Attack||Target)&&!unknown_bool_B8)return false;
    return !UnloadTimer.State1&&!UnloadTimer.State2;
}
void UnitClass::Update() {
    if(!IsAlive||InLimbo)return;
    if(ReadyToNextMission()) {
        if(GetCurrentMission()==Mission::None&&QueuedMission==Mission::None)EnterIdleMode(false,true);
        NextMission();
    }
    FootClass::Update();if(!IsAlive||InLimbo)return;
    if(ReadyToNextMission())NextMission();
    if(Type->CanDisguise&&!Type->PermaDisguise)UpdateDisguise();
    UpdateFiring();if(!IsAlive)return;UpdateRotation();
}
void UnitClass::UpdateRotation() {
    if(Target&&!unknown_bool_6AF) {
        DirStruct direction;GetDirectionTo(&direction,Target);
        if(Type->Turret) {
            const auto* weapon=GetTurretWeapon();
            if(weapon&&weapon->WeaponType&&!weapon->WeaponType->OmniFire)
                SecondaryFacing.SetDesired(weapon->TurretLocked?PrimaryFacing.Current():direction);
        } else if(Type->SpeedType==SpeedType::Track&&!Destination&&!Locomotor->Is_Moving()&&PrimaryFacing.Current()==direction)
            PrimaryFacing.SetDesired(direction);
    }
    if(Type->TurretSpins)SecondaryFacing.SetDesired(DirStruct(int((SecondaryFacing.Current().GetValue<8>()+8)<<8)));
    else {
        unknown_bool_6AF=false;
        if(Type->Turret) {
            if(SecondaryFacing.IsRotating())unknown_bool_6AF=true;
            else if(!Target&&Unsorted::CurrentFrame-LastFireBulletFrame>=RulesClass::Instance->GuardAreaTargetingDelay+5
                &&!SpawnManager&&(!Type->DeployToFire||!Deployed)) {
                const auto* weapon=GetTurretWeapon();
                if(!Destination||(weapon&&weapon->WeaponType&&weapon->TurretLocked))SecondaryFacing.SetDesired(PrimaryFacing.Current());
                else if(!IsAttackedByLocomotor){DirStruct direction;GetDirectionTo(&direction,Destination);SecondaryFacing.SetDesired(direction);}
            }
        }
    }
    if(Type->Turret)TurretIsRotating=SecondaryFacing.IsRotating();
}
bool UnitClass::EnterIdleMode(bool initial,bool resume) {
    const bool result=FootClass::EnterIdleMode(initial,resume);
    if(HaveMegaMission()||GetCurrentMission()==Mission::Wait)return result;
    HandleNavigationList();
    Mission order=Mission::Guard;
    if(Destination)order=Mission::Move;
    else if(Type->Harvester||Type->Weeder) {
        if(HasAnyLink()||GetCurrentMission()==Mission::Harvest||QueuedMission==Mission::Harvest)return result;
        if(initial||!Owner->IsControlledByHuman()||GetCell()->LandType==LandType::Tiberium)order=Mission::Harvest;
        SetTarget(nullptr);SetDestination(nullptr,true);
    } else if(IsArmed()) {
        if(GetCurrentMission()==Mission::Guard||GetCurrentMission()==Mission::Area_Guard
            ||(GetCurrentMission()!=Mission::None&&(CurrentMissionControl()->Zombie||CurrentMissionControl()->Paralyzed)))return result;
        if(Owner->IQLevel2>=RulesClass::Instance->GuardArea||HasAbility(Ability::GuardArea)||Type->DefaultToGuardArea)order=Mission::Area_Guard;
        if(Team||SpawnManager||SlaveManager)order=Mission::Guard;
    } else {SetTarget(nullptr);SetDestination(nullptr,true);}
    if(CurrentMission!=Mission::Patrol&&CurrentMission!=Mission::Area_Guard&&CurrentMission!=Mission::Unload&&CurrentMission!=Mission::Hunt)
        QueueMission(order,false);
    return result;
}
Action UnitClass::MouseOverCell(const CellStruct* cell,bool fog,bool ignoreForce) const {
    if(!Owner->IsControlledByCurrentPlayer())return Action::None;
    auto action=FootClass::MouseOverCell(cell,fog,ignoreForce);
    if(action==Action::Move&&(Deployed||Deploying||Undeploying))return Action::None;
    if(action==Action::Attack&&!IsArmed())return Action::None;
    if(action==Action::Move&&Type->Harvester&&MapClass::Instance.GetCellAt(*cell)->LandType==LandType::Tiberium&&!OnBridge)return Action::Harvest;
    return action;
}
Action UnitClass::MouseOverObject(const ObjectClass* target,bool ignoreForce) const {
    auto action=FootClass::MouseOverObject(target,ignoreForce);
    if(target&&target->WhatAmI()==AbstractType::Building&&Type->Harvester&&Owner->IsControlledByCurrentPlayer()&&Owner->IsAlliedWith(target)){
        auto* refinery=static_cast<const BuildingClass*>(target);
        if(refinery->Type->DockUnload&&Type->Dock.FindItemIndex(refinery->Type)>=0&&action!=Action::Attack)
            return refinery->HasFreeLink(this)?Action::Enter:Action::NoEnter;
    }
    // OpenTS repair vehicle action, YR 0x73FD7B..0x73FE48.
    if(target&&action!=Action::Self_Deploy&&CombatDamage(-1)<0&&Owner->IsControlledByCurrentPlayer()){
        if(!Owner->IsAlliedWith(target))return Action::AttackSupport;
        if(target->IsStrange()&&target!=this&&const_cast<ObjectClass*>(target)->IsSurfaced()){
            if(target->WhatAmI()!=AbstractType::Aircraft||!MapClass::Instance.GetCellAt(target->GetCoords())->GetBuilding())
                if(target->GetHealthPercentage()<RulesClass::Instance->ConditionGreen)return Action::GRepair;
        }else if(target->WhatAmI()!=AbstractType::Building)return Action::Select;
    }
    if(action==Action::Attack&&!IsArmed())return Type->Crusher&&target->GetType()->Crushable?Action::Move:Action::Select;
    if(action==Action::Self_Deploy&&!Type->DeploysInto&&!Type->IsSimpleDeployer&&!Passengers.NumPassengers)return Action::None;
    return action;
}
bool UnitClass::ObjectClickedAction(Action action,ObjectClass* target,bool ignoreForce){
    const auto current=MouseOverObject(target,ignoreForce);
    if(action!=current){
        action=current;
        if(action==Action::Enter)action=Action::Move;
        else if(action==Action::Capture||action==Action::Sabotage)action=Action::Attack;
    }
    if(action==Action::Heal||action==Action::GRepair)action=Action::Attack;
    if(target==this&&(action==Action::NoMove||action==Action::Attack))return false;
    if(Berzerk)return false;
    return FootClass::ObjectClickedAction(action,target,ignoreForce);
}
// OpenTS Assign_Destination, YR normal ground navigation entrance. The
// original clearing of Path[0] is essential when redirecting a moving tank.
void UnitClass::SetDestination(AbstractClass* target,bool immediate) {
    if(target==Destination&&!UnlimboingInfantry)return;
    UnlimboingInfantry=false;
    if((Deployed&&CurrentMission==Mission::Sleep)||Deploying||Undeploying){unknown_5A0=nullptr;Destination=nullptr;return;}
    if(GetCurrentMission()!=Mission::Enter&&QueuedMission!=Mission::Enter||HasAnyLink())PathDirections[0]=-1;
    if(!target||immediate)unknown_abstract_array_588.Clear();
    if(Type->Harvester&&target&&target->WhatAmI()==AbstractType::Building&&GetCurrentMission()!=Mission::Unload){
        auto* refinery=static_cast<BuildingClass*>(target);
        if(Type->Dock.FindItemIndex(refinery->Type)>=0&&refinery->HasFreeLink(this)){
            if(SendCommand(RadioCommand::RequestLink,refinery)==RadioCommand::AnswerPositive
                &&GetCurrentMission()!=Mission::Enter&&GetCurrentMission()!=Mission::Harvest)QueueMission(Mission::Enter,false);
        }
    }
    // YR 0x741970: the chrono miner drives ordinary orders, but restores its
    // underlying teleport locomotor for a clear cell at its linked refinery.
    if(Type->Teleporter&&Type->Harvester&&Locomotor&&!IsBeingWarpedOut()&&!IsAttackedByLocomotor){
        auto* dock=GetNthLink();
        // Identify COM locomotors by the original class ID, including objects
        // constructed by the EXE rather than this module's C++ RTTI runtime.
        constexpr GUID persistID{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
        IPersist* persist=nullptr;GUID classID{};
        if(Locomotor->QueryInterface(persistID,reinterpret_cast<void**>(&persist))<0||!persist)return;
        const auto identified=persist->GetClassID(&classID);persist->Release();
        if(identified<0)return;
        auto* driver=!std::memcmp(&classID,&LocomotionClass::CLSIDs::Drive,sizeof(classID))
            ?static_cast<DriveLocomotionClass*>(static_cast<ILocomotion*>(Locomotor)):nullptr;
        const bool returning=dock&&dock->WhatAmI()==AbstractType::Building&&static_cast<BuildingClass*>(dock)->Type->DockUnload
            &&target&&target->WhatAmI()==AbstractType::Cell
            &&!static_cast<CellClass*>(target)->FindObjectOfType(AbstractType::Unit,false);
        if(returning&&driver){
            if(driver->Is_Piggybacking()&&driver->Is_Ok_To_End()){
                ILocomotion* restored=nullptr;driver->End_Piggyback(&restored);
#if defined(_MSC_VER)
                Locomotor.Attach(restored);
#else
                Locomotor=restored;driver->Release();
#endif
            }else{
                // 0x741970: finish the current drive track before restoring
                // Teleport. Retrying Enter must not restart Move_To below or
                // be discarded as an unchanged destination on the next call.
                Locomotor->Stop_Moving();ForceMission(Mission::None);
                QueueMission(Mission::Enter,false);
                unknown_bool_6AC=true;UnlimboingInfantry=true;
            }
        }else if(!returning&&!driver){
            auto* next=GameCreate<DriveLocomotionClass>();if(!next)return;
            next->Link_To_Object(this);next->Begin_Piggyback(Locomotor);
#if defined(_MSC_VER)
            Locomotor=static_cast<ILocomotion*>(next);
#else
            next->AddRef();Locomotor->Release();Locomotor=next;
#endif
        }
    }
    FootClass::SetDestination(target,immediate);
}

// OpenTS Scatter; YR 0x743A50's ground-drive branch.
void UnitClass::Scatter(const CoordStruct& from,bool force,bool urgent) {
    if(!CanScatter()||(!CurrentMissionControl()->Scatter&&!force)||PrimaryFacing.IsRotating()
        ||(Destination&&!urgent)||Deployed||Deploying||Undeploying||!Locomotor->Is_Powered())return;
    auto& map=MapClass::Instance;
    const auto origin=CellClass::Coord2Cell(GetDestination());
    if(from==CoordStruct::Empty) {
        const auto nearby=map.NearByLocation(origin,Type->SpeedType,-1,MovementZone::Normal,OnBridge,
            1,1,false,true,false,true,CellStruct::Empty,false,false);
        if(nearby!=CellStruct::Empty)SetDestination(map.GetCellAt(nearby),true);
        return;
    }
    if(CurrentMissionControl()->Paralyzed||Unloading||(Destination&&(!urgent||unknown_bool_6AF)))return;
    auto& random=ScenarioClass::Instance->Random;
    if(Target&&!force&&random.RandomRanged(1,4)!=1)return;
    const DirStruct direction(int((Math::atan2(double(from.Y)-Location.Y,double(Location.X)-from.X)-1.5707963267948966)*-10430.060040584269));
    const int start=(direction.GetValue<3>()+random.RandomRanged(0,2)-1)&7;
    // Preserve YR's ternary precedence (groundLevel + bool ? 4 : 0).
    const int height=(int(map.GetCellAt(origin)->Level)+int(IsOnBridge(nullptr))?4:0)*Unsorted::LevelHeight;
    CellStruct first=CellStruct::Empty,preferred=CellStruct::Empty;
    for(int i=0;i<8;++i) {
        const auto facing=FacingType((start+i)&7);auto* cell=map.GetCellAt(origin)->GetNeighbourCell(facing);
        if(!map.IsWithinUsableArea(cell->MapCoords,true)||IsCellOccupied(cell,facing,GetCellLevel(),nullptr,true)!=Move::OK)continue;
        if(first==CellStruct::Empty)first=cell->MapCoords;
        const CoordStruct position{cell->MapCoords.X*256+128,cell->MapCoords.Y*256+128,height};
        CellStruct adjusted;TacticalClass::AdjustCellForHeight(&adjusted,&position);
        if(adjusted==cell->MapCoords&&!cell->ContainsBridgeEx()){preferred=cell->MapCoords;break;}
    }
    const auto target=preferred!=CellStruct::Empty?preferred:first;
    if(target!=CellStruct::Empty){QueueMission(Mission::Move,false);SetDestination(map.GetCellAt(target),true);}
}
