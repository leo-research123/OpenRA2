// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Take_Damage; calibrated to YR 0x442230.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/VocClass.h"
#include "map_world.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

DamageState BuildingClass::ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* attacker,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) {
    if(attacker==this&&!GetTechnoType()->DamageSelf)return DamageState::Unaffected;
    const int oldFrame=GetShapeNumber();auto* source=static_cast<TechnoClass*>(attacker);
    if(source&&!IsStrange()) {
        Owner->LATime=Unsorted::CurrentFrame;Owner->LAEnemy=source->Owner->ArrayIndex;
        // The native map session deliberately does not start the faction's
        // recruit/dispatch commander. Unit reactions remain in Techno damage.
#if defined(RA2_YRPP_GAME)
        BaseIsAttacked(source);
#endif
    }
    const auto* foundation=GetFoundationData(false);
    std::vector<TechnoClass*> linked;
    for(int i=0;i<RadioLinks.Capacity;++i)if(auto* object=GetNthLink(i))linked.push_back(object);
    if((Type->LaserFence&&!ignoreDefenses)||(Type->BridgeRepairHut&&Type->Immune))return DamageState::Unaffected;
    auto state=DamageState::Unaffected;
    if(Health) {
        state=TechnoClass::ReceiveDamage(damage,distance,warhead,attacker,ignoreDefenses,preventEscape,sourceHouse);
        if(!IsAlive)return state;
        switch(state) {
        case DamageState::NowYellow:
            if(FireParticleSystem)FireParticleSystem->SpawnFrames=float(double(FireParticleSystem->SpawnFrames)*1.5);
            [[fallthrough]];
        case DamageState::NowRed: {
            if(Type->DamageSound==-1) { /* Native session has no default damage sound bank. */ }
            for(auto* entry=foundation;*entry!=CellStruct{0x7FFF,0x7FFF};++entry) {
                const auto own=GetMapCoords();CoordStruct at{(own.X+entry->X)*256+128,(own.Y+entry->Y)*256+128,0};
                at.Z=MapClass::Instance.GetCellFloorHeight(at);
                if(!warhead->Sparky)continue;
                const int choice=ScenarioClass::Instance->Random.RandomRanged(0,Type->GetFoundationWidth()+Type->GetFoundationHeight(false)+5);
                const int index=choice>=1&&choice<=5?0:choice>=6&&choice<=8?1:choice==9?2:-1;
                if(index>=0)if(auto* memory=YRMemory::Allocate(sizeof(AnimClass))) {
                    const int loops=index==2?1:ScenarioClass::Instance->Random.RandomRanged(1,3);
                    const auto where=MapClass::GetRandomCoordsNear(at,96,false);
                    auto* animation=::new(memory) AnimClass(RulesClass::Instance->DamageFireTypes[index],where,0,loops,0x600,0,false);
                    animation->SetOwnerObject(this);
                }
            }
            break;
        }
        case DamageState::NowDead:
            if(BunkerLinkedItem){linked.erase(std::remove(linked.begin(),linked.end(),BunkerLinkedItem),linked.end());UnloadBunker();}
            if(CaptureManager)CaptureManager->FreeAll();
            if(LocomotorTarget)ReleaseLocomotor(true);
            for(auto* object:linked) {
                const auto a=GetCoords(),b=object->GetCoords();const double x=double(a.X)-b.X,y=double(a.Y)-b.Y,z=double(a.Z)-b.Z;
                if(int(std::sqrt(x*x+y*y+z*z))<256||Type->Helipad) {
                    int lethal=object->GetTechnoType()->Strength*10;
                    object->ReceiveDamage(&lethal,0,RulesClass::Instance->C4Warhead,nullptr,true,true,nullptr);
                }else {SendCommand(static_cast<RadioCommand>(23),object);object->QueueUpToEnter=nullptr;}
            }
            if(Type->CanBeOccupied)UnloadOccupants(false,false);
            if(LightSource)LightSource->Deactivate();
            Destory(nullptr,source,ignoreDefenses,foundation);
            // 0x0044266C..0x004426A2: a positive destruction timer takes
            // the immediate UnInit/LeaveRubble path. UnInit queues deletion,
            // so DamageArea can finish safely without drawing eight stale frames.
            if(C4Timer.GetTimeLeft()>0){
                if(Owner)Owner->RegisterLoss(this,false);
                // The native placement adapter owns the multi-cell foundation.
                // Release it before base Limbo clears IsOnMap; otherwise the
                // remaining cells retain a pointer to the deferred deletion.
                game::detach_map_object(*this);UnInit();LeaveRubble();
                ActuallyPlacedOnMap=false;game::map_object_changed();
            }
            break;
        case DamageState::PostMortem:return state;
        default:break;
        }
    }
    if(!IsAlive)return DamageState::NowDead;
    if(source&&state!=DamageState::Unaffected) {
        if(!Type->Insignificant&&!IsStrange())Owner->BuildingUnderAttack(this);
        OwnerCountryIndex=source->Owner->ArrayIndex;
        auto* weapon=GetWeapon(0)->WeaponType;
        if(CurrentMission!=Mission::Selling&&!Owner->IsAlliedWith(source)&&weapon&&!weapon->Projectile->AA
            &&(!Target||!InAuxiliarySearchRange(Target))) {
            if(source->WhatAmI()==AbstractType::Aircraft||(Owner->IsControlledByHuman()&&!RulesClass::Instance->PlayerReturnFire)) {
                if(!SecondaryFacing.IsRotating()&&IsPowerOnline())SecondaryFacing.SetDesired(DirStruct(short(ScenarioClass::Instance->Random.Random()<<8)));
            }else SetTarget(source);
        }
    }
    const auto refresh=[&] {
        const bool damaged=GetHealthPercentage()<=RulesClass::Instance->ConditionYellow;
        if(IsDamaged==damaged)return;IsDamaged=damaged;
        for(int i=0;i<21;++i)if(Anims[i]) {
            const auto& entry=Type->BuildingAnim[i];
            const char* name=damaged?entry.Damaged:entry.Anim;
            if(name&&*name)PlayAnim(name,static_cast<BuildingAnimSlot>(i),damaged,false,0);
        }
    };
    if(state!=DamageState::Unaffected)refresh();
    if(oldFrame!=GetShapeNumber()){NeedsRedraw=true;refresh();}
    UpdateDamageFires();game::map_object_changed();return state;
}
