// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 team.cpp lifecycle/AI/TMission_MOVE. YR 0x6E8A90,
// 0x6EA500/0x6EA870/0x6E9140/0x6EC7D0/0x6EBAD0 calibrate state transitions.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TeamClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/RulesClass.h"
#include <climits>
namespace {DynamicVectorClass<TeamClass*> teams;}
DynamicVectorClass<TeamClass*>& TeamClass::Array=teams;
TeamClass::TeamClass(TeamTypeClass* type,HouseClass* owner,int value) noexcept
 :AbstractClass(),Type(type),CurrentScript(nullptr),Owner(owner),Target(nullptr),SpawnCell(nullptr),ClosestMember(nullptr),
 QueuedFocus(nullptr),Focus(nullptr),unknown_44(value),TotalObjects(0),TotalThreatValue(0),CreationFrame(Unsorted::CurrentFrame),FirstUnit(nullptr),
 GuardAreaTimer{},SuspendTimer{},Tag(nullptr),IsTransient(false),NeedsReGrouping(false),GuardSlowerIsNotUnderStrength(false),IsForcedActive(false),
 IsHasBeen(false),IsFullStrength(false),IsUnderStrength(true),IsReforming(false),IsLagging(false),NeedsToDisappear(true),JustDisappeared(false),IsMoving(false),
 StepCompleted(true),TargetNotAssigned(false),IsLeavingMap(false),IsSuspended(false),AchievedGreatSuccess(false),CountObjects{}{
    GuardAreaTimer.Start(0);SuspendTimer.Start(0);Array.AddItem(this);
    AbstractClass::Array.AddItem(this);TypeExpirationListeners.AddItem(this);TagExpirationListeners.AddItem(this);
    if(Type){
        ++Type->cntInstances;
        SpawnCell=MapClass::Instance.TryGetCellAt(ScenarioClass::Instance->GetWaypointCoords(Type->Waypoint));
        if(Type->Tag)Tag=GameCreate<TagClass>(Type->Tag,CellStruct::Empty);
        CurrentScript=GameCreate<ScriptClass>(Type->ScriptType);
    }
}
TeamClass::~TeamClass(){
    while(FirstUnit)LiberateMember(FirstUnit);
    GameDelete(CurrentScript);if(Tag)Tag->Destroy();
    if(Type)--Type->cntInstances;
    AbstractClass::Array.Remove(this);TypeExpirationListeners.Remove(this);TagExpirationListeners.Remove(this);
    Array.Remove(this);NotifyObjectExpired(true);
}
void TeamClass::PointerExpired(AbstractClass* object,bool removed){
    if(Tag==object)Tag=nullptr;
    if(FirstUnit==object&&removed)FirstUnit=FirstUnit->NextTeamMember;
    if(Owner==object)Owner=nullptr;
    if(CurrentScript==object)CurrentScript=nullptr;
    if(Focus==object)Focus=nullptr;
    if(QueuedFocus==object)QueuedFocus=nullptr;
    if(SpawnCell==object)SpawnCell=nullptr;
    if(Type==object)Type=nullptr;
    if(ClosestMember==object)ClosestMember=nullptr;
    if(Target==object)Target=nullptr;
    if(static_cast<std::uintptr_t>(unknown_44)==reinterpret_cast<std::uintptr_t>(object))unknown_44=0;
}
bool TeamClass::AddMember(FootClass* foot,bool force){
    if(!foot||!Type||!Type->TaskForce)return false;
    int index=-1;
    for(int i=0;i<Type->TaskForce->CountEntries;++i)if(Type->TaskForce->Entries[i].Type==foot->GetTechnoType()&&
       (force||CountObjects[i]<Type->TaskForce->Entries[i].Amount)){index=i;break;}
    if(index<0)return false;
    if(foot->Team)foot->Team->LiberateMember(foot);
    if(!force)++CountObjects[index];
    foot->IsInitiated=FirstUnit==nullptr;foot->NextTeamMember=FirstUnit;FirstUnit=foot;foot->Team=this;
    foot->Group=Type->Group;++TotalObjects;NeedsToDisappear=JustDisappeared=true;
    return true;
}
void TeamClass::LiberateMember(FootClass* foot,int index,byte count){
    if(!foot||foot->Team!=this)return;
    FootClass** next=&FirstUnit;while(*next&&*next!=foot)next=&(*next)->NextTeamMember;
    if(!*next)return;
    *next=foot->NextTeamMember;foot->Team=nullptr;foot->NextTeamMember=nullptr;
    if(index<0&&Type&&Type->TaskForce)for(int i=0;i<Type->TaskForce->CountEntries;++i)
        if(Type->TaskForce->Entries[i].Type==foot->GetTechnoType()){index=i;break;}
    if(!count&&index>=0&&index<6)--CountObjects[index];
    --TotalObjects;NeedsToDisappear=JustDisappeared=true;
    if(ClosestMember==foot)ClosestMember=nullptr;
}
FootClass* TeamClass::FetchALeader() const {
    FootClass* result=FirstUnit;int score=-1;
    for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember)
        if(foot->IsAlive&&foot->Health>0&&!foot->InLimbo&&(foot->IsInitiated||foot->WhatAmI()==AbstractType::Aircraft)&&foot->GetTechnoType()->LeadershipRating>score){result=foot;score=foot->GetTechnoType()->LeadershipRating;}
    return result;
}
void TeamClass::AssignMissionTarget(AbstractClass* target){
    if(target!=QueuedFocus&&QueuedFocus)for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember){
        const bool destination=foot->Destination==QueuedFocus,attack=foot->Target==QueuedFocus;
        if(destination||attack){
            foot->QueueMission(Mission::Guard,false);
            if(destination)foot->SetDestination(nullptr,true);
            if(attack)foot->SetTarget(nullptr);
        }
    }
    if(!Focus||Focus==QueuedFocus)Focus=target;
    QueuedFocus=target;
    if(target&&target->WhatAmI()==AbstractType::Cell){
        IsLeavingMap=!MapClass::Instance.IsWithinUsableArea(static_cast<CellClass*>(target),true);
        if(IsLeavingMap)for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember)foot->SetDestination(nullptr,true);
    }
}
void TeamClass::Update(){
    if(IsSuspended){if(SuspendTimer.GetTimeLeft())return;IsSuspended=false;}
    if(!Type||!CurrentScript||!CurrentScript->Type)return;
    if(!FirstUnit){if(IsHasBeen)delete this;return;}
    if(NeedsToDisappear||JustDisappeared){
        // OpenTS Recalc_Strength / YR 0x6EA3E0: non-reinforceable teams
        // that have started keep executing their script after casualties.
        int desired=0;for(int i=0;i<Type->TaskForce->CountEntries;++i)desired+=Type->TaskForce->Entries[i].Amount;
        IsFullStrength=TotalObjects==desired;if(IsFullStrength)IsHasBeen=true;
        IsUnderStrength=Type->Reinforce?(desired>2?TotalObjects<=desired/3:TotalObjects<desired):!IsHasBeen;
        if(Type->GuardSlower)GuardSlowerIsNotUnderStrength=!IsUnderStrength;
        NeedsToDisappear=JustDisappeared=false;
    }
    if(!IsMoving&&(IsFullStrength||IsForcedActive)){IsMoving=IsHasBeen=true;IsUnderStrength=false;}
    if(!IsMoving||IsReforming||IsUnderStrength)return;
    bool next=false;
    if(StepCompleted){
        next=true;StepCompleted=false;++CurrentScript->CurrentMission;
        for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember)foot->ArchiveTarget=nullptr;
        if(!CurrentScript->HasCurrentMission()){delete this;return;}
        AssignMissionTarget(nullptr);Target=nullptr;
    }
    ScriptActionNode action;CurrentScript->GetCurrentAction(&action);
    // OpenTS TMission_ATTACK / Coordinate_Attack; YR 0x6ED090 /
    // 0x6EB490, quarry masks from 0x645BB0. Preserve in-flight cargo teams.
    if(action.Action==0){
        auto* leader=FetchALeader();if(!leader)return;
        if(!QueuedFocus){
            unsigned mask=0;
            switch(action.Argument){case 2:mask=0x20;break;case 3:mask=0x40;break;
            case 4:mask=8;break;case 5:mask=0x10;break;case 6:mask=0x1000;break;
            case 7:mask=0x2000;break;case 9:mask=0x800;break;case 10:mask=0x8000;break;case 11:mask=0x10000;break;}
            auto at=leader->Location;AssignMissionTarget(leader->GreatestThreat(static_cast<ThreatType>(mask),&at,Type->OnlyTargetHouseEnemy));
        }
        bool armed=false;for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember)
            if(foot->GetTechnoType()->Ammo<=0||foot->Ammo>0)armed=true;
        if(!QueuedFocus||!armed){StepCompleted=true;return;}
        if(!Focus)Focus=QueuedFocus;
        for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember){
            if(!foot->IsAlive||!foot->Health||foot->InLimbo||foot->IsFallingDown)continue;
            const auto mission=foot->GetCurrentMission();
            if(mission!=Mission::Attack&&mission!=Mission::Enter&&mission!=Mission::Capture&&mission!=Mission::Sabotage){
                foot->SendToFirstLink(RadioCommand::NotifyUnlink);foot->QueueMission(Mission::Attack,false);
                foot->SetTarget(nullptr);foot->SetDestination(nullptr,true);
            }
            if(!foot->Target)foot->SetTarget(Focus);
        }
        return;
    }
    if(action.Action==49){StepCompleted=true;AchievedGreatSuccess=true;return;}
    if(action.Action!=3)return;
    if(next)AssignMissionTarget(MapClass::Instance.TryGetCellAt(ScenarioClass::Instance->GetWaypointCoords(action.Argument)));
    if(!Focus)return;
    bool complete=true;
    const int following=CurrentScript->CurrentMission+1;
    const bool nextMove=following<CurrentScript->Type->ActionsCount&&CurrentScript->Type->ScriptActions[following].Action==3;
    for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember){
        if(!foot->IsAlive||!foot->Health)continue;
        if(foot->InLimbo||foot->IsFallingDown){complete=false;continue;}
        // Coordinate_Move, YR 0x6EBAD0: members may stop within Stray;
        // a whole infantry team cannot occupy one cell's three free spots.
        const int stray=RulesClass::Instance->Stray*(foot->IsInAir()?2:1);
        const bool arrived=foot->DistanceFrom3D(Focus)<=stray&&(foot->GetHeight()>=0||nextMove)&&
            (foot->WhatAmI()!=AbstractType::Aircraft||foot->GetZ()<=0||foot->GetCell()==Focus||nextMove);
        if(arrived){
            if(foot->CurrentMission==Mission::Move&&(!foot->Destination||
                (foot->DistanceFrom3D(foot->Destination)<=RulesClass::Instance->CloseEnough&&!foot->Locomotor->Is_Moving()))&& !foot->Target){
                foot->SetDestination(nullptr,true);foot->EnterIdleMode(false,true);
            }
            if(foot->Destination)complete=false;
            continue;
        }
        if(Type->Aggressive&&foot->Target){if(foot->Destination)complete=false;continue;}
        complete=false;
        if(foot->CurrentMission!=Mission::Move){foot->QueueMission(Mission::Move,false);if(foot->ReadyToNextMission())foot->NextMission();}
        if(!foot->Destination)foot->SetDestination(Focus,true);
    }
    StepCompleted=complete;
}

// OpenTS Took_Damage; YR 0x6EB380 retains regrouping but omits the TS
// assignment of the attacker to the team's focus at the end of this branch.
void TeamClass::MemberTookDamage(FootClass*,DamageState state,TechnoClass* source){
    if(!Type||!source||state==DamageState::Unaffected||Type->Suicide)return;
    if(IsMoving){
        for(auto* foot=FirstUnit;foot;foot=foot->NextTeamMember)if(foot==source)return;
        if(!FirstUnit||FirstUnit->WhatAmI()==AbstractType::Aircraft||!FirstUnit->GetWeapon(0)->WeaponType||Focus==source||!Type->Annoyance)return;
    }
    SpawnCell=nullptr;NeedsReGrouping=true;IsReforming=true;
}
