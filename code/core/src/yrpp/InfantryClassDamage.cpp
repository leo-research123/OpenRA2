// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp Take_Damage; YR 0x517FA0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Unsorted.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>

DamageState InfantryClass::ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* attacker,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) {
    if(Crawling && *damage>0 && !ignoreDefenses)*damage=std::max(rule_integer(double(*damage)*warhead->ProneDamage),1);
    if(warhead && int(warhead->InfDeath)==9 && GetHeight()>0)*damage=0;
    const auto result=FootClass::ReceiveDamage(damage,distance,warhead,attacker,ignoreDefenses,preventEscape,sourceHouse);
    if(result==DamageState::PostMortem || result==DamageState::Unaffected)return result;
    auto& rules=*RulesClass::Instance;
    auto* source=static_cast<TechnoClass*>(attacker);
    if(result==DamageState::NowDead) {
        if(SlaveOwner && SlaveOwner->SlaveManager)SlaveOwner->SlaveManager->LostSlave(this);
        if(Transporter && Transporter->WhatAmI()==AbstractType::Unit && Transporter->GetTechnoType()->OpenTopped)
            Transporter->ExitedOpenTopped(this);
        Destroyed(attacker);StopMoving();Stun();
        QueueMission(Mission::None,false);QueueMission(Mission::Guard,false);NextMission();KillPassengers(source);
        const auto animation=[&](AnimTypeClass* type,const CoordStruct& at) -> AnimClass* {
            auto* memory=YRMemory::Allocate(sizeof(AnimClass));
            return memory?::new(memory) AnimClass(type,at,0,1,0x600,0,false):nullptr;
        };
        const auto virusParticle=[&] {
            if(auto* anim=animation(rules.InfantryVirus,Location)) {
                ParticleSystemClass::DefaultSystem->SpawnParticle(ParticleTypeClass::Array[anim->Type->SpawnsParticle],Location);
                delete anim;
            }
        };
        const bool immediate=ignoreDefenses && Type->Cyborg;
        if(immediate && IsABomb)animation(rules.InfantryExplode,Location);
        const auto remove=[&] {
            if(!Type->BalloonHover || !Crash(nullptr))UnInit();
        };
        if(GetHeight()<=10 && GetCell()->LandType==LandType::Water && IsABomb) {
            animation(rules.Wake,Location);
            auto splash=Location;splash.Z+=3;animation(rules.SplashList[0],splash);
            remove();return result;
        }
        if((Type->Cyborg && Crawling) || Type->JumpJet) {
            animation(rules.InfantryExplode,Location);remove();return result;
        }
        int death=int(warhead->InfDeath);
        // YR 0x00517FA0 tests sequence 33 (Paradrop), not AirDeathStart (34).
        // A lethal hit while falling uses the explosion/removal path: PlayAnim
        // deliberately rejects ordinary death sequences until landing, which
        // otherwise leaves a zero-health soldier visibly hanging from its chute.
        if(SequenceAnim==Sequence::Paradrop){if(death==8)virusParticle();death=3;}
        if(source && source->WhatAmI()==AbstractType::Building && static_cast<BuildingClass*>(source)->Type->LaserFence)death=5;
        if(Type->DeathAnims.Count) {
            if(death<0 || death>=Type->DeathAnims.Count || !Type->DeathAnims[death])death=0;
            if(Type->DeathAnims[death])animation(Type->DeathAnims[death],Location);
        }else if(Type->NotHuman) {
            PlayAnim(Sequence::Die1,true,false);if(death==8)virusParticle();
            if(!immediate)return result;
        }else {
            switch(death) {
            case 1:case 2:
                PlayAnim(death==1?Sequence::Die1:Sequence::Die2,true,false);
                if(!immediate)return result;
                break;
            case 3:animation(rules.InfantryExplode,Location);break;
            case 4:animation(rules.FlamingInfantry,Location);break;
            case 5:animation(AnimTypeClass::Array[1],Location);break;
            case 6:animation(rules.InfantryHeadPop,Location);break;
            case 7:animation(rules.InfantryNuked,Location);break;
            case 8:
                if(auto* anim=animation(rules.InfantryVirus,Location)) {
                    if(source)anim->Owner=source->Owner;else if(sourceHouse)anim->Owner=sourceHouse;
                }
                break;
            case 9: {
                UnmarkAllOccupationBits(Location);
                auto* cell=MapClass::Instance.GetCellAt(GetMapCoords());bool building=false;
                for(auto* object=cell->FirstObject;object;object=object->NextObject)if(object->WhatAmI()==AbstractType::Building)building=true;
                if((GroundType::Array[int(cell->LandType)].Cost[0]==0 && !OnBridge)
                    || cell->FindInfantrySubposition(Location,false,false,false)==CoordStruct::Empty || building) {
                    PlayAnim(Sequence::Die2,true,false);if(!immediate)return result;break;
                }
                MarkAllOccupationBits(Location);
                if(auto* anim=animation(rules.InfantryMutate,Location)) {
                    if(auto* owner=source?source->Owner:sourceHouse) {
                        anim->Owner=owner;anim->LightConvert=ColorScheme::Array[owner->ColorSchemeIndex]->LightConvert;
                    }
                    anim->MarkAllOccupationBits(anim->Location);
                }
                break;
            }
            case 10:animation(rules.InfantryBrute,Location);break;
            default:break;
            }
        }
        remove();return result;
    }
    if(!Owner->IsControlledByHuman() && Type->Civilian
        && (GetCurrentMission()==Mission::Guard || GetCurrentMission()==Mission::Area_Guard))QueueMission(Mission::Hunt,false);
    if(source)Scatter(source->Location,false,false);
    if(source && std::bit_cast<int>(PanicDurationLeft)<100) {
        if(Type->Fraidycat)PanicDurationLeft=300;
        else if(!Type->Fearless && !HasAbility(Ability::Fearless))PanicDurationLeft=100;
    }else if(!Type->Fearless && !HasAbility(Ability::Fearless)) {
        int increment=GetHealthPercentage()>rules.ConditionRed?25:50;
        if(GetHealthPercentage()>rules.ConditionYellow)increment/=2;
        PanicDurationLeft=unsigned(std::min(std::bit_cast<int>(PanicDurationLeft+unsigned(increment)),300));
    }
    return result;
}

// OpenTS Scatter; YR 0x51D0D0. Receiving damage does not invent a movement
// order: the original mission, stance, player-scatter and fraidycat gates all
// precede the search for an adjacent destination.
void InfantryClass::Scatter(const CoordStruct& from,bool forced,bool urgent) {
    const auto deployed=[&]{return SequenceAnim>=Sequence::Deploy && SequenceAnim<=Sequence::DeployedIdle;};
    if(deployed() && forced && urgent)PlayAnim(Sequence::Undeploy,false,false);
    else if(Owner->IsControlledByHuman() && deployed())return;
    if(Locomotor->Is_Moving())forced=false;
    if(!CurrentMissionControl()->Scatter && !forced)return;
    if(!Type->Fraidycat && Target && !forced)return;
    constexpr bool interrupt[]{1,1,1,1,1,0,1,0,1,1,1,0,0,0,0,0,1,1,1,1,0,0,1,1,1,1,1,0,1,1,1,0,0,1,0,1,0,1,1,1,1,1};
    if(SequenceAnim!=Sequence::Nothing && SequenceAnim!=Sequence::Undeploy && !interrupt[int(SequenceAnim)])return;
    if(RulesClass::Instance->PlayerScatter || HasAbility(Ability::Scatter) || urgent || !Owner->IsControlledByHuman()) {
        if(!forced && !Type->Fraidycat)return;
    }else if(!forced && (!Team || !Type->Fraidycat))return;
    unsigned facing;
    auto& random=ScenarioClass::Instance->Random;
    const auto angle=[](double y,double x) {
        return static_cast<unsigned short>(rule_integer((Math::atan2(y,x)-1.5707963267948966)*-10430.060040584269));
    };
    if(from==CoordStruct::Empty) {
        const auto at=GetCoords();
        const unsigned x=unsigned(at.X)&0xFFu,y=unsigned(at.Y)&0xFFu;
        facing=x==128 && y==128?PrimaryFacing.Current().GetValue<16>():angle(128.0-y,double(x)-128.0);
        facing=((facing+0x1000u)>>13)&7u;
        facing+=random.RandomRanged(0,4)-2;
        auto cell=CellClass::Coord2Cell(GetDestination());
        auto nearby=MapClass::Instance.NearByLocation(cell,Type->SpeedType,-1,MovementZone::Normal,OnBridge,
            1,1,false,true,false,true,CellStruct{0,0},false,false);
        if(nearby!=CellStruct::Empty){SetDestination(MapClass::Instance.GetCellAt(nearby),true);Locomotor->Process();return;}
    }else {
        const auto at=Location;
        facing=angle(double(from.Y)-at.Y,double(at.X)-from.X);
        facing=(((facing+0x1000u)>>13)&7u)+random.RandomRanged(0,4)-2;
    }
    const auto cell=CellClass::Coord2Cell(GetDestination());
    // Preserve the original ternary precedence: this is not groundLevel+4.
    const int height=(int(MapClass::Instance.GetCellAt(cell)->Level)+int(IsOnBridge(nullptr))?4:0)*Unsorted::LevelHeight;
    CellStruct first=CellStruct::Empty,preferred=CellStruct::Empty;
    for(int i=0;i<8;++i) {
        const int direction=(facing+i)&7u;const auto delta=Unsorted::AdjacentCell[direction];
        const CellStruct candidate{short(cell.X+delta.X),short(cell.Y+delta.Y)};
        auto* target=MapClass::Instance.GetCellAt(candidate);
        if(!MapClass::Instance.IsWithinUsableArea(candidate,true)
            || IsCellOccupied(target,static_cast<FacingType>(direction),GetCellLevel(),nullptr,true)!=Move::OK)continue;
        if(first==CellStruct::Empty)first=candidate;
        const CoordStruct position{int(candidate.X)*256+128,int(candidate.Y)*256+128,height};
        CellStruct adjusted;TacticalClass::AdjustCellForHeight(&adjusted,&position);
        if(candidate==adjusted && !target->ContainsBridgeEx()){preferred=candidate;break;}
    }
    const auto destination=preferred!=CellStruct::Empty?preferred:first;
    if(destination!=CellStruct::Empty){QueueMission(Mission::Move,false);SetDestination(MapClass::Instance.GetCellAt(destination),true);}
}
