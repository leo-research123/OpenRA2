// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::What_Action / Active_Click_With; YR overrides.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"
#include <cstdlib>

bool InfantryClass::CanDeploySlashUnload() const {
    if(auto* cell=GetCell();cell&&cell->IsNearTunnelNW())return false;
    if(Type->UndeployDelay>-1){
        if(IsDeployed()||SequenceAnim==Sequence::Undeploy)return false;
        if(RearmTimer.GetTimeLeft()!=0)return false;
    }
    return Type->Deployer;
}

Action InfantryClass::MouseOverObject(const ObjectClass* object,bool ignoreForce) const {
    auto action=FootClass::MouseOverObject(object,ignoreForce);
    if(!object||action==Action::ToggleSelect)return action;
    auto* target=const_cast<ObjectClass*>(object);
    auto* building=target->WhatAmI()==AbstractType::Building?static_cast<BuildingClass*>(target):nullptr;
    const bool ours=Owner->IsControlledByCurrentPlayer(),allied=Owner->IsAlliedWith(target);
    const bool warped=target->IsBeingWarpedOut();
    const auto* input=InputManagerClass::Instance;
    bool fire=!ignoreForce&&input&&input->IsForceFireKeyPressed();
    if(!ignoreForce&&input&&input->IsForceSelectKeyPressed()&&fire)fire=false;
    const bool deployed=SequenceAnim>=Sequence::Deploy&&SequenceAnim<=Sequence::DeployedIdle;
    auto& rules=*RulesClass::Instance;
    if(Type->Engineer&&ours&&target->AttachedBomb&&target->BombVisible)return Action::DisarmBomb;
    if(Type->Engineer&&building&&ours&&!target->IsStrange()&&building->Type->Repairable) {
        if(building->Type->BridgeRepairHut)return MapClass::Instance.IsLinkedBridgeDestroyed(building->GetMapCoords())?Action::GRepair:Action::NoGRepair;
        if(allied||(building->Owner->Type->MultiplayPassive&&building->Type->CanBeOccupied&&!warped)) {
            if(building->Type->Hospital&&GetHealthPercentage()<rules.ConditionGreen)return Action::Enter;
            if(building->GetHealthPercentage()==rules.ConditionGreen)return building->Type->Grinding?Action::Repair:Action::NoGRepair;
            return Action::GRepair;
        }
        if(building->Type->Capturable)return building->GetHealthPercentage()>rules.EngineerCaptureLevel?Action::Damage:Action::Capture;
    }
    if(building&&ours&&!warped&&building->CanBeOccupiedBy(const_cast<InfantryClass*>(this))&&!fire)return Action::Capture;
    if(Type->Engineer&&action==Action::Attack)return Action::NoMove;
    if(CombatDamage(-1)<0&&ours) {
        if(!allied)return Action::AttackSupport;
        if(target->WhatAmI()==AbstractType::Infantry) {
            if(target==this)return Action::GuardArea;
            if(target->GetHealthPercentage()<rules.ConditionGreen)return Action::Heal;
        }
        if((target->AbstractFlags&::AbstractFlags::Techno)==::AbstractFlags::None||!target->GetTechnoType()->Passengers) {
            if(action!=Action::GuardArea&&action!=Action::Move)return Action::Select;
            return action;
        }
    }
    if(ours&&Type->VehicleThief&&target->IsStrange()) {
        if(target->GetTechnoType()->IsTrain)return Action::Select;
        if(target->GetOwningHouse()!=Owner) {
            if(!ScenarioClass::Instance->SpecialFlags.HarvesterImmune)return Action::Capture;
            for(auto* base:rules.HarvesterUnit)if(static_cast<const void*>(base)==target)return Action::Select;
            return Action::Capture;
        }
    }
    const auto radioEnter=[&] {
        const int answer=int(const_cast<InfantryClass*>(this)->SendCommand(static_cast<RadioCommand>(15),static_cast<TechnoClass*>(target)));
        if(answer==1)action=Action::Enter;else if(answer==10)action=Action::NoEnter;
    };
    if(building&&allied&&ours&&!warped&&building->Type->Hospital&&!fire) {
        if(Health>=Type->Strength)action=Action::NoEnter;
        else if(!Type->Deployer||!deployed)radioEnter();
    }
    if(building&&allied&&ours&&!warped&&building->Type->Armory) {
        if(Veterancy.IsElite())action=Action::NoEnter;
        else if(!Type->Deployer||!deployed)radioEnter();
    }
    if(ours&&(Type->C4||HasAbility(Ability::C4))&&action==Action::Attack&&building&&!target->IsStrange())
        return building->Type->CanC4&&!building->Type->InvisibleInGame?Action::Sabotage:Action::Attack;
    const auto* weapon=GetWeapon(SelectWeapon(target))->WeaponType;
    if(action==Action::Attack&&weapon) {
        if(weapon->SabotageCursor)return target->WhatAmI()==AbstractType::Unit&&static_cast<UnitClass*>(target)->BunkerLinkedItem?Action::Select:Action::Demolish;
        if(weapon->MigAttackCursor&&building&&building->Type->CanC4&&!building->Type->InvisibleInGame)return Action::Airstrike;
    }
    if(ours&&action==Action::Attack&&Type->Ivan)return !target->GetType()->Bombable||target->AttachedBomb?Action::NoIvanBomb:Action::IvanBomb;
    if(ours&&Type->Deployer&&deployed) {
        if(action==Action::Move)return Action::NoMove;
        if(action==Action::Attack&&(!Type->DeployFire||!IsCloseEnough(target,SelectWeapon(nullptr))))return Action::NoMove;
        if(action==Action::Self_Deploy&&(PlanningNodeClass::PlanningModeActive||Type->UndeployDelay>-1))return Action::NoDeploy;
    }
    if(ours&&action==Action::Self_Deploy&&Type->Deployer&&Type->UndeployDelay>-1&&SequenceAnim==Sequence::Undeploy)return Action::NoDeploy;
    if(action==Action::Self_Deploy) {
        if(PlanningNodeClass::PlanningModeActive)action=Action::NoDeploy;
        else if(ours) {
            const auto* turret=GetTurretWeapon();
            if(turret&&turret->WeaponType&&turret->WeaponType->AreaFire)action=Action::AreaAttack;
            else if(!Type->Deployer)action=Action::None;
        }else action=Action::None;
    }
    if(action!=Action::NoEnter&&allied&&ours&&(target->AbstractFlags&::AbstractFlags::Techno)!=::AbstractFlags::None
        &&!warped&&!target->IsWarpingIn()&&action!=Action::Attack&&action!=Action::GuardArea
        &&target->GetTechnoType()->Passengers>0&&IsControllable()) {
        auto* foot=(target->AbstractFlags&::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(target):nullptr;
        if(Type->BalloonHover||(foot&&((foot->Team&&!foot->Team->Type->Loadable)||foot->Locomotor->Is_Moving())))action=Action::NoEnter;
        else radioEnter();
    }
    if(ours&&Type->Infiltrate&&action==Action::Attack) {
        if(!allied&&building&&((!Type->Agent&&building->Type->Capturable)||(Type->Agent&&building->Type->Spyable)))action=Action::Capture;
        else if(!IsArmed())action=Action::None;
    }else if(action==Action::Attack&&!IsArmed())action=Action::AttackSupport;
    if(action==Action::Select&&building&&ours&&!building->IsStrange()&&building->Type->Grinding)
        return target->GetOwningHouse()==HouseClass::CurrentPlayer?Action::Repair:Action::NoEnter;
    if(ours&&action==Action::None&&IsControllable())action=Action::NoMove;
    else if(action==Action::Attack&&target->GetType()->Immune)return Action::NoMove;
    if(action==Action::NoMove&&target->IsDisguised()&&!target->GetDisguiseHouse(true)) {
        auto* disguise=target->GetDisguise(true);
        if(disguise&&disguise->WhatAmI()==AbstractType::TerrainType)return Action::Move;
    }
    return action;
}

Action InfantryClass::MouseOverCell(const CellStruct* where,bool checkFog,bool ignoreForce) const {
    if(!Owner->IsControlledByCurrentPlayer())return Action::None;
    auto action=FootClass::MouseOverCell(where,checkFog,ignoreForce);
    if(CombatDamage(-1)<0 && Owner->IsControlledByCurrentPlayer() && action==Action::Attack)action=Action::GuardArea;
    auto* cell=MapClass::Instance.GetCellAt(*where);
    if(Type->JumpJet && (action==Action::Move || action==Action::NoMove)
        && (cell->IsNearTunnelNW() || cell->IsNearTunnelES()))action=Action::None;
    const bool deployed=SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle;
    if(Owner->IsControlledByCurrentPlayer() && Type->Deployer && deployed) {
        if(action==Action::Move)action=Action::NoMove;
        else if(action==Action::Attack && (!Type->DeployFire
            || !IsCloseEnough3D({int(where->X)*256+128,int(where->Y)*256+128,0},SelectWeapon(nullptr))))action=Action::NoMove;
    }else if(action==Action::Move && cell->Tile_Is_Tunnel())return Action::EnterTunnel; // 0x484F10 is true
    if(ScenarioClass::Instance->SpecialFlags.FogOfWar && checkFog && cell->FoggedObjects
        && cell->FoggedObjects->Count)std::abort(); // remembered engineer target needs FoggedObjectClass
    if(action==Action::Attack && !IsArmed())action=Action::AttackSupport;
    if(Owner->IsControlledByCurrentPlayer() && action==Action::None && !Type->JumpJet && IsControllable())action=Action::NoMove;
    return action;
}
bool InfantryClass::CellClickedAction(Action action,CellStruct* cell,CellStruct* follow,bool ignoreForce) {
    if(SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle
        && MouseOverCell(cell,false,ignoreForce)==Action::NoMove)return false;
    if(Berzerk)return false;
    return FootClass::CellClickedAction(action,cell,follow,ignoreForce);
}
bool InfantryClass::ObjectClickedAction(Action,ObjectClass* target,bool ignoreForce) {
    auto action=MouseOverObject(target,ignoreForce);
    switch(action) {
        case Action::Capture:case Action::Damage:case Action::GRepair:action=Action::Capture;break;
        case Action::Heal:case Action::DisarmBomb:action=Action::Attack;break;
        default:break;
    }
    if((target==this && action==Action::Attack) || Berzerk)return false;
    return FootClass::ObjectClickedAction(action,target,ignoreForce);
}
