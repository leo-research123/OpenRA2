// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp Take_Damage; YR 0x4D7330 adds parasite handling.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/FootClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/ScenarioClass.h"
#include <bit>

// OpenTS aircraft.cpp Crash, shared by YR FootClass at 0x4DEBB0.
bool FootClass::Crash(ObjectClass* killer) {
    if(GetHeight()<=0)return false;
    auto* source=static_cast<TechnoClass*>(killer);
    if(Health>0) {
        if(killer&&AttachedTag)AttachedTag->RaiseEvent(TriggerEvent::FirstDamaged_combatonly,this,CellStruct::Empty);
        if(AttachedTag&&IsAlive)AttachedTag->RaiseEvent(TriggerEvent::FirstDamaged_anysource,this,CellStruct::Empty);
        if(AttachedTag&&IsAlive&&killer)AttachedTag->RaiseEvent(TriggerEvent::FirstDamaged_anysource,this,CellStruct::Empty,false,source);
        if(!IsAlive)return true;
        RegisterDestruction(source);
        if(!IsAlive)return true;
        if(!IsAttackedByLocomotor)Health=0;
    }
    IsCrashing=true;SendToFirstLink(RadioCommand::NotifyUnlink);Stun();KillPassengers(source);
    if(WhatAmI()!=AbstractType::Unit&&!Unsorted::ScenarioInit) {
        auto& random=ScenarioClass::Instance->Random;
        RockingSidewaysPerFrame=float(double(random.RandomRanged(0,0x7FFFFFFE))*4.656612877414201e-10*0.15+0.1);
        if(!random.RandomRanged(0,1))RockingSidewaysPerFrame=-RockingSidewaysPerFrame;
        RockingForwardsPerFrame=float(double(random.RandomRanged(0,0x7FFFFFFE))*4.656612877414201e-10*0.1);
    }
    NotifyObjectExpired(false);
    return true;
}

DamageState FootClass::ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) {
    if(warhead && warhead->Sonic && ParasiteEatingMe) {
        ParasiteEatingMe->ParasiteImUsing->ExitUnit();
        if(source)static_cast<TechnoClass*>(source)->SetTarget(nullptr);
    }
    if(ParasiteEatingMe && source!=ParasiteEatingMe && *damage>ParasiteEatingMe->GetTechnoType()->SuppressionThreshold) {
        const int delay=std::bit_cast<int>(2u*unsigned(*damage)-unsigned(ParasiteEatingMe->GetTechnoType()->SuppressionThreshold));
        ParasiteEatingMe->ParasiteImUsing->SuppressionTimer.Start(delay);
    }
    if(ParasiteEatingMe && *damage<0) {
        ParasiteEatingMe->ParasiteImUsing->SuppressionTimer.Start(50);
        ParasiteEatingMe->ParasiteImUsing->ExitUnit();
    }
    const auto result=TechnoClass::ReceiveDamage(damage,distance,warhead,source,ignoreDefenses,preventEscape,sourceHouse);
    if(result==DamageState::PostMortem)return result;
    if(result!=DamageState::Unaffected && Team) {
        Team->MemberTookDamage(this,result,static_cast<TechnoClass*>(source));return result;
    }
    if(result==DamageState::NowDead || result==DamageState::Unaffected)return result;
    if(Team && Team->Type->Whiner && !Owner->IsControlledByHuman()) {
        if(!source)return result;
        BaseIsAttacked(static_cast<TechnoClass*>(source));
    }
    if(source && CurrentMissionControl()->NoThreat && !CurrentMissionControl()->Zombie)EnterIdleMode(false,true);
    return result;
}
