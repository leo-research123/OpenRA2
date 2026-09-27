// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 unit.cpp Take_Damage/Explode.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA terms: third_party/opents/LICENSE.md. YR 0x737C90 / 0x738680.
// Ordinary ground vehicle death; specialized naval sinking, crashing and
// crate drops retain their existing incomplete native services.
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TeamClass.h"

namespace {
void explosion(AnimTypeClass* type,const CoordStruct& at) {
    if(!type)return;
    if(auto* storage=YRMemory::Allocate(sizeof(AnimClass)))
        ::new(storage) AnimClass(type,at,0,1,0x600,0,false);
}
}

void UnitClass::Explode() {
    auto& random=ScenarioClass::Instance->Random;
    if(Type->Explosion.Count) {
        auto* animation=Type->Explosion[unsigned(random.Random())%unsigned(Type->Explosion.Count)];
        if((Type->Explodes||HasAbility(Ability::Explodes))&&(Type->Ammo==-1||Ammo>0))
            animation=Type->Explosion[Type->Explosion.Count-1];
        explosion(animation,Location);
        // YR's 0x738750..0x7387FE computes the harvester power but never calls
        // Wide_Area_Damage; its screen-shake entry is a no-op. Do not copy TS's blast.
    }
    if(Type->DestroyAnim.Count)
        explosion(Type->DestroyAnim[unsigned(random.Random())%unsigned(Type->DestroyAnim.Count)],Location);
}

DamageState UnitClass::ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* attacker,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) {
    const bool selected=IsSelected&&Owner->IsControlledByCurrentPlayer();
    const auto result=FootClass::ReceiveDamage(damage,distance,warhead,attacker,ignoreDefenses,preventEscape,sourceHouse);
    if(result!=DamageState::NowDead)return result;

    if(BunkerLinkedItem&&BunkerLinkedItem->WhatAmI()==AbstractType::Building)
        static_cast<BuildingClass*>(BunkerLinkedItem)->ClearBunker();
    if(Type->DeathFrames>0) {
        if(DeathFrameCounter==-1){DeathFrameCounter=0;Destroyed(attacker);}
        Health=1;IsAlive=true;
    }else {
        Destroyed(attacker);
        if(GetHeight()<=10&&IsABomb&&GetCell()->LandType==LandType::Water) {
            explosion(RulesClass::Instance->Wake,Location);
            auto at=Location;at.Z+=5;
            const auto& splashes=RulesClass::Instance->SplashList;
            if(splashes.Count)explosion(splashes[splashes.Count-1],at);
        }else Explode();
    }

    // Remove the cell content before placing survivors. Deferred UnInit below
    // releases locomotor reservations, Logic/Display membership and identity.
    Mark(MarkType::Up);
    if(Type->OpenTopped)MarkPassengersAsExited();
    if(GetHeight()>208)KillPassengers(static_cast<TechnoClass*>(attacker));
    if(!Type->BalloonHover)while(Passengers.NumPassengers) {
        auto* passenger=RemoveFirstPassenger();if(!passenger)break;
        const auto move=passenger->IsCellOccupied(GetCell(),FacingType::None,-1,nullptr,true);
        ++Unsorted::ScenarioInit;
        passenger->OnBridge=OnBridge;
        auto at=Location;at.Z=GetCell()->GetCoords().Z;if(OnBridge)at=GetCenterCoords();
        if(!preventEscape&&!IsABomb&&(move==Move::OK||move==Move::MovingBlock)
            &&passenger->Unlimbo(at,DirType(PrimaryFacing.Current().GetValue<8>()))) {
            passenger->Transporter=nullptr;
            if(Type->OpenTopped&&Owner!=passenger->Owner)passenger->GotHijacked();
            passenger->Scatter(CoordStruct::Empty,true,false);
            if(!passenger->Owner->IsControlledByHuman()) {
                if(Team)Team->AddMember(passenger,false);
                else passenger->QueueMission(Mission::Hunt,false);
            }
            if(selected)passenger->Select();
        }else {passenger->Limbo();passenger->UnInit();}
        --Unsorted::ScenarioInit;
    }

    auto& random=ScenarioClass::Instance->Random;
    InfantryTypeClass* crew=nullptr;
    if(HijackerInfantryType!=-1)crew=InfantryTypeClass::Array.GetItemOrDefault(HijackerInfantryType);
    else if(!preventEscape&&Type->Crewed&&!Type->Passengers
        &&double(random.RandomRanged(0,0x7FFFFFFE))*4.656612877414201e-10<RulesClass::Instance->CrewEscape)
        crew=GetCrew();
    if(crew) {
        auto* survivor=GameCreate<InfantryClass>(crew,Owner);
        if(survivor) {
            survivor->OnBridge=OnBridge;
            if(survivor->InitializeLocomotor()&&survivor->Unlimbo(Location,DirType::North)) {
                survivor->Health=random.RandomRanged(5,crew->Strength/2);
                survivor->Scatter(CoordStruct::Empty,true,false);
                survivor->QueueMission(Owner->IsControlledByHuman()?Mission::Guard:Mission::Hunt,false);
                if(selected)survivor->Select();
                if(AttachedTag&&AttachedTag->ShouldReplace())survivor->AttachTrigger(AttachedTag);
            }else delete survivor;
        }
    }

    // Original ground-vehicle tail 0x738493..0x7384A5. A zero-health object
    // must not remain registered as a live map unit after ReceiveDamage.
    if(Type->BalloonHover){if(!Crash(nullptr))UnInit();}
    else if(!IsSinking)UnInit();
    return result;
}
