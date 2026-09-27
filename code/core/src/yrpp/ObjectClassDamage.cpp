// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp Take_Damage; YR 0x5F5390.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/ObjectClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/MapClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>

DamageState ObjectClass::ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* attacker,
        bool ignoreDefenses,bool,HouseClass* attackingHouse) {
    const int before=Health;
    if(before<=0 || !*damage || (!ignoreDefenses && GetType()->Immune))return DamageState::Unaffected;
    const int maximum=GetType()->Strength;
    if(!ignoreDefenses)*damage=MapClass::GetTotalDamage(*damage,warhead,GetType()->Armor,distance);
    if(WhatAmI()==AbstractType::Building && !static_cast<BuildingClass*>(this)->Type->CanC4)*damage=std::max(*damage,1);
    if(!*damage)return DamageState::Unaffected;
    if(*damage<0) {
        const int old=Health;Health=std::bit_cast<int>(unsigned(Health)-unsigned(*damage));
        if(Health>maximum)Health=maximum;
        if(Health!=old)Flash(7);
        return DamageState::Unaffected;
    }
    auto result=DamageState::Unchanged;
    if(before<=*damage)*damage=before;
    else if(before>=(maximum>>1) && before-*damage<(maximum>>1))result=DamageState::NowYellow;
    const double red=double(maximum)*RulesClass::Instance->ConditionRed;
    if(double(before)>red && double(before-*damage)<red)result=DamageState::NowRed;
    Health=before-*damage;
    if(Health<=0 && WhatAmI()==AbstractType::Infantry && !ignoreDefenses) {
        auto* infantry=static_cast<InfantryClass*>(this);
        if(infantry->Type->Cyborg && !infantry->Crawling) {
            if(auto* storage=YRMemory::Allocate(sizeof(AnimClass)))::new(storage) AnimClass(RulesClass::Instance->InfantryExplode,Location,0,1,0x600,0,false);
            Health=std::max(rule_integer(double(infantry->Type->Strength)*0.25),1);
            infantry->Crawling=true;infantry->PlayAnim(Sequence::Crawl,true,false);result=DamageState::NowRed;
        }
    }
    const int after=Health;
    const auto event=[&](int id,TechnoClass* source=nullptr){if(AttachedTag)AttachedTag->RaiseEvent(static_cast<TriggerEvent>(id),this,CellStruct::Empty,false,source);};
    const auto threshold=[&](int combat,int any){if(attacker)event(combat);if(IsAlive)event(any);};
    if(result==DamageState::NowYellow)threshold(39,42);
    if(!IsAlive)return DamageState::PostMortem;
    if(result==DamageState::NowRed)threshold(40,43);
    if(!IsAlive)return DamageState::PostMortem;
    auto* source=attacker?static_cast<TechnoClass*>(attacker):nullptr;
    if(Health!=before && GetType() && before==GetType()->Strength) {
        if(attacker)event(38);
        if(IsAlive)event(41);
        if(AttachedTag){if(!IsAlive)return DamageState::PostMortem;if(attacker)event(41,source);}
    }
    if(!IsAlive || (after>0 && Health<=0))return DamageState::PostMortem;
    if(!Health) {
        auto* sourceHouse=source?source->Owner:nullptr;
        if(!attackingHouse || (source && attackingHouse==sourceHouse))RegisterDestruction(source);
        else RegisterKill(attackingHouse);
        result=DamageState::NowDead;Disappear(true);
    }
    if(IsAlive && attacker && result!=DamageState::NowDead)event(6,source);
    if(IsAlive && attacker && result!=DamageState::NowDead)event(44,source);
    if(IsAlive && IsSelected)Mark(MarkType::Change);
    return result;
}
