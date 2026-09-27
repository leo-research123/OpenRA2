// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Take_Damage; YR 0x701900 / 0x7087C0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/BombClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BulletClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
#include <bit>
#include <vector>

// YR-specific 0x70D690 (no pinned OpenTS counterpart) uses the current turret weapon
// as the fallback and executes an immediate bullet detonation.
void TechnoClass::FireDeathWeapon(int additionalDamage) {
    const auto* type=GetTechnoType();
    auto* weapon=type->DeathWeapon;
    if(!weapon)if(auto* slot=GetPrimaryWeapon())weapon=slot->WeaponType;
    int damage=weapon?rule_integer(double(weapon->Damage)*type->DeathWeaponDamageModifier)
                     :rule_integer(double(type->Strength)*0.5);
    if(!weapon)weapon=RulesClass::Instance->DeathWeapon;
    if(!weapon)return;
    damage+=additionalDamage;
    if(auto* bullet=weapon->Projectile->CreateBullet(this,this,damage,weapon->Warhead,0,weapon->Bright)) {
        bullet->SetWeaponType(weapon);bullet->Limbo();
        bullet->Detonate(GetCoords());bullet->Release();
    }
}

namespace {
template<class T,class... A>T* create(A&&... args) {
    auto* memory=YRMemory::Allocate(sizeof(T));
    return memory?::new(memory) T(std::forward<A>(args)...):nullptr;
}

// OpenTS techno.cpp::Record_The_Kill; YR 0x702D40 / 0x703230.
void destruction_events(TechnoClass& victim,bool hasSource) {
 const auto event=[&](int id){if(victim.IsAlive&&victim.AttachedTag)victim.AttachedTag->RaiseEvent(static_cast<TriggerEvent>(id),&victim,CellStruct::Empty);};
 if(hasSource){event(6);event(4);}
 if(victim.WhatAmI()!=AbstractType::Unit){if(hasSource)event(7);event(48);event(29);}
}
int kill_value(TechnoClass& victim,HouseClass* source) {
 int value=victim.GetTechnoType()->GetActualCost(victim.Owner);
 if(source&&source->IsAlliedWith(&victim))return 0;
 if(victim.Veterancy.IsVeteran())value=std::bit_cast<int>(unsigned(value)*2u);
 else if(victim.Veterancy.IsElite())value=std::bit_cast<int>(unsigned(value)*3u);
 return value;
}
void record_kill_totals(TechnoClass& victim,HouseClass* source) {
 auto* type=victim.GetTechnoType();const auto kind=victim.WhatAmI();
 const int owner=victim.Owner->ArrayIndex,index=type->GetArrayIndex();
 UnitTrackerClass* tracker=nullptr;
 if(kind==AbstractType::Building){
  if(type->Insignificant)return;
  auto& building=static_cast<BuildingClass&>(victim);
  if(building.OwnerCountryIndex!=0xFFFFFFFFu)++victim.Owner->TotalKilledBuildings;
  if(source&&!type->DontScore){tracker=&source->KilledBuildingTypes;if(owner>=0&&owner<20)++source->KilledBuildingsOfHouses[owner];}
 }else if(kind==AbstractType::Infantry||kind==AbstractType::Unit||kind==AbstractType::Aircraft){
  ++victim.Owner->TotalKilledUnits;
  if(source&&!type->DontScore){
   tracker=kind==AbstractType::Infantry?&source->KilledInfantryTypes:kind==AbstractType::Unit?&source->KilledUnitTypes:&source->KilledAircraftTypes;
   if(owner>=0&&owner<20)++source->KilledUnitsOfHouses[owner];
  }
 }
 // UnitTrackerClass::IncrementUnitCount 0x749020 accepts only its populated range.
 if(tracker&&index>=0&&index<tracker->UnitCount)++tracker->UnitTotals[index];
}
}

void TechnoClass::OnFinishRepair() {
 Mark(MarkType::Change);Health=EstimatedHealth=GetTechnoType()->Strength;
 if(WhatAmI()==AbstractType::Building){
  SetRepairState(0);static_cast<BuildingClass*>(this)->ToggleDamagedAnims(GetHealthPercentage()<=RulesClass::Instance->ConditionYellow);
  if(RulesClass::Instance->BuildingRepairedSound!=-1)VocClass::PlayAt(RulesClass::Instance->BuildingRepairedSound,GetCoords());
 }
}

void TechnoClass::RegisterDestruction(TechnoClass* source) {
 destruction_events(*this,source!=nullptr);
 if(GetTechnoType()->DontScore)return;
 if(source){
  const int value=kill_value(*this,source->Owner);TechnoClass* recipient=nullptr;
  if(source->InOpenToppedTransport&&source->Transporter&&source->Transporter->GetTechnoType()->Trainable)recipient=source->Transporter;
  else if(source->GetTechnoType()->Trainable)recipient=source;
  else if(source->GetTechnoType()->Spawned){if(source->SpawnOwner&&source->SpawnOwner->GetTechnoType()->Trainable)recipient=source->SpawnOwner;}
  else if(source->CanOccupyFire()&&source->WhatAmI()==AbstractType::Building){
   auto* building=static_cast<BuildingClass*>(source);recipient=building->Occupants.GetItemOrDefault(building->FiringOccupantIndex);
  }
  if(recipient)recipient->Veterancy.Add(recipient->GetTechnoType()->GetActualCost(Owner),value);
  Owner->WhoLastHurtMe=source->Owner->ArrayIndex;source->Owner->PointTotal+=value;
 }
 record_kill_totals(*this,source?source->Owner:nullptr);
}
void TechnoClass::RegisterKill(HouseClass* source) {
 destruction_events(*this,source!=nullptr);
 if(source){Owner->WhoLastHurtMe=source->ArrayIndex;source->PointTotal+=kill_value(*this,source);}
 record_kill_totals(*this,source);
}
// OpenTS Crew_Type; YR 0x707D20, also used by Unit's 0x740EE0 thunk.
InfantryTypeClass* TechnoClass::GetCrew() const {
 if(!GetTechnoType()->Crewed)return nullptr;
 const auto& rules=*RulesClass::Instance;
 auto* crew=Owner->SideIndex==0?rules.AlliedCrew:Owner->SideIndex==1?rules.SovietCrew:
     Owner->SideIndex==2?rules.ThirdCrew:rules.Technician;
 if(Owner->Type->SideIndex==-1)return rules.Technician;
 if(IsArmed()&&ScenarioClass::Instance->Random.RandomRanged(0,99)<15)return rules.Technician;
 return crew;
}
namespace {
void sound(const TypeList<int>& sounds,const CoordStruct& at) {
    if(sounds.Count)VocClass::PlayAt(sounds[unsigned(Randomizer::Global.Random())%sounds.Count],at,nullptr);
}
}

DamageState TechnoClass::ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* attacker,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) {
    auto* type=GetTechnoType();auto& rules=*RulesClass::Instance;
    auto* source=static_cast<TechnoClass*>(attacker);
    const bool healing=*damage<0;
    if(!ignoreDefenses && !healing) {
        *damage=rule_integer(double(*damage)/(Owner->GetArmorMultiplier(type)*ArmorMultiplier));
        if(HasAbility(Ability::Stronger))*damage=rule_integer(double(*damage)/rules.VeteranArmor);
        *damage=std::max(*damage,1);
        if(source && type->TypeImmune && source->GetTechnoType()==type && source->Owner==Owner)return DamageState::Unaffected;
    }
    if(IsIronCurtained() && !ignoreDefenses && !healing) {
        MapClass::FlashbangWarheadAt(std::bit_cast<int>(2u*unsigned(*damage)),warhead,Location,true,
            static_cast<SpotlightFlags>(ForceShielded==1?6:1));
        *damage=0;return DamageState::Unaffected;
    }
    if(IsBeingWarpedOut() && !ignoreDefenses){*damage=0;return DamageState::Unaffected;}
    if(type->DamageReducesReadiness) {
        const double ratio=double(*damage)/type->Strength*type->ReadinessReductionMultiplier;
        Ammo=std::max(rule_integer(double(Ammo)-type->Ammo*ratio),0);StartReloading();
    }
    if(BunkerLinkedItem && !ignoreDefenses && warhead) {
        if((WhatAmI()==AbstractType::Building && warhead->PenetratesBunker)
            || (WhatAmI()!=AbstractType::Building && !warhead->PenetratesBunker && GetCell()->GetBuilding()==BunkerLinkedItem)) {
            *damage=0;return DamageState::Unaffected;
        }
    }
    if(warhead) {
        if((warhead->Radiation && type->ImmuneToRadiation)
            || (warhead->PsychicDamage && type->ImmuneToPsionicWeapons)
            || (warhead->Poison && type->ImmuneToPoison)
            || (!warhead->AffectsAllies && source && source->Owner->IsAlliedWith(Owner))) {
            *damage=0;return DamageState::Unaffected;
        }
        if(warhead->Psychedelic) {
            if(Owner->IsAlliedWith(sourceHouse) || type->ImmuneToPsionics || WhatAmI()==AbstractType::Building)return DamageState::Unaffected;
            *damage=MapClass::GetTotalDamage(*damage,warhead,GetType()->Armor,0);BerzerkDurationLeft=*damage;
            if(!Berzerk) {
                Berzerk=true;
                if((AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None) {
                    auto* foot=static_cast<FootClass*>(this);if(foot->Team)foot->Team->LiberateMember(foot,-1,0);
                }
                SetTarget(nullptr);QueueMission(Mission::Hunt,false);
            }
            return DamageState::Unchanged;
        }
    }
    auto result=ObjectClass::ReceiveDamage(damage,distance,warhead,attacker,ignoreDefenses,preventEscape,sourceHouse);
    if(source)Owner->RegisterDamage(rule_integer(double(type->GetCost())*(double(*damage)/type->Strength)),source->Owner);
    if(result==DamageState::PostMortem)return result;
    if(result==DamageState::NowDead && WhatAmI()==AbstractType::Building && warhead && warhead->CausesDelayKill) {
        auto* building=static_cast<BuildingClass*>(this);
        if(building->Type->EligibleForDelayKill) {
            const int delay=warhead->DelayKillFrames;
            if(!building->C4Applied || delay<building->C4Timer.GetTimeLeft()){building->C4Applied=true;building->C4Timer.Start(delay);}
            IsAlive=true;Health=1;return DamageState::PostMortem;
        }
    }
    if(result!=DamageState::Unaffected) {
        RadarFlashTimer.Start(rules.RadarCombatFlashTime);
        if(result!=DamageState::NowDead && type->CanDisguise && !type->PermaDisguise) {
            if(IsDisguised())ClearDisguise();
            DisguiseBlinkTimer.Start(std::bit_cast<int>(2u*unsigned(*damage)));
        }
    }
    if(!Health)result=DamageState::NowDead;
    if(result==DamageState::NowDead) {
        if(SlaveManager)SlaveManager->Killed(source,nullptr);
        const auto releaseDrain=[](TechnoClass* object) {
            if(!object->DrainTarget)return;
            if(object->DrainAnim){object->DrainAnim->UnInit();object->DrainAnim=nullptr;}
            if(object->DrainTarget) {
                object->DrainTarget->DrainingMe=nullptr;
                if(object->DrainTarget->Owner)object->DrainTarget->Owner->RecheckPower=true;
                object->DrainTarget=nullptr;
            }
        };
        releaseDrain(this);if(DrainingMe)releaseDrain(DrainingMe);
        if(CaptureManager)CaptureManager->FreeAll();
        if(Owner->IsControlledByCurrentPlayer())sound(type->VoiceDie,Location);
        sound(type->DieSound,Location);
        SendToEachLink(RadioCommand::NotifyUnlink);Stun();
        if(FireParticleSystem){FireParticleSystem->UnInit();FireParticleSystem=nullptr;}
        if(GetHeight()>10 || !IsABomb || GetCell()->LandType!=LandType::Water) {
            if(type->MaxDebris>0) {
                auto& random=ScenarioClass::Instance->Random;
                int remaining=random.RandomRanged(type->MinDebris,type->MaxDebris-1);
                for(int index=0;type->DebrisTypes.Count && remaining>0;index=(index+1)%type->DebrisTypes.Count) {
                    const int value=random.Random();
                    const unsigned absolute=value<0?0u-unsigned(value):unsigned(value);
                    const int count=std::min(int(absolute%unsigned(type->DebrisMaximums[index]+1)),remaining);
                    for(int i=0;i<count;++i){auto at=GetCoords();create<VoxelAnimClass>(type->DebrisTypes[index],&at,Owner);}
                    remaining-=count;
                }
                const auto& debris=type->DebrisAnims.Count?type->DebrisAnims:rules.MetallicDebris;
                if(debris.Count && (type->DebrisAnims.Count || !type->DebrisTypes.Count))for(int i=0;i<remaining;++i) {
                    auto at=GetCoords();at.Z+=20;
                    create<AnimClass>(debris[random.RandomRanged(0,debris.Count-1)],at,0,1,0x600,0,false);
                }
            }
            auto* weapon=GetWeapon(CurrentWeaponNumber)->WeaponType;
            if(type->Explodes || HasAbility(Ability::Explodes) || (weapon && weapon->Suicide)) {
                KillPassengers(source);FireDeathWeapon(0);
            }
        }
        if(AttachedBomb)AttachedBomb->Detonate();
        return result;
    }
    if(result==DamageState::Unchanged) {
        if(type->DamageSound!=-1)VocClass::PlayIndexAtPos(type->DamageSound,Location,0);
        if(type->DamageSound!=-1)VocClass::PlayIndexAtPos(type->DamageSound,Location,0);
        if((type->ToProtect || unknown_bool_3CF) && !Owner->IsControlledByHuman() && source)BaseIsAttacked(source);
    }else if(result==DamageState::NowYellow && type->VoiceFeedback.Count
        && Randomizer::Global.RandomRanged(0,99)<30 && Owner->IsControlledByCurrentPlayer())sound(type->VoiceFeedback,GetCoords());
    if(source && !Owner->IsAlliedWith(source))HasBeenAttacked=true;
    Reveal();
    if(GetHealthPercentage()<=rules.ConditionYellow) {
        if(result==DamageState::NowYellow || result==DamageState::NowRed) {
            std::vector<ParticleSystemTypeClass*> systems;
            for(int i=type->DamageParticleSystems.Count-1;i>=0;--i)
                if(int(type->DamageParticleSystems[i]->BehavesLike)==0)systems.push_back(type->DamageParticleSystems[i]);
            if(!DamageParticleSystem && !systems.empty() && GetHeight()>-10) {
                auto offset=type->DamageSmokeOffset;
                if(type->DamSmkOffScrnRel) {
                    const auto converted=TacticalClass::Instance->ClientToCoords({offset.X,offset.Y});
                    const int horizontal=10*offset.Z;
                    const int height=rule_integer(Math::sqrt(double(horizontal)*horizontal*2.0)*0.58);
                    offset={converted.X-horizontal,converted.Y-horizontal,offset.Z>=0?-height:height};
                }
                const CoordStruct at{Location.X+offset.X,Location.Y+offset.Y,Location.Z+offset.Z};
                const int index=ScenarioClass::Instance->Random.RandomRanged(0,int(systems.size())-1);
                DamageParticleSystem=create<ParticleSystemClass>(systems[index],at,nullptr,this,CoordStruct::Empty,nullptr);
            }
        }
    }else if(DamageParticleSystem)DamageParticleSystem->UnInit();
    if(healing)return result;
    const bool foot=(AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None;
    const bool scatters=rules.PlayerScatter || HasAbility(Ability::Scatter);
    if(CanRetaliateToAttacker(attacker,warhead)) {
        if(source && (IsCloseEnough(source,SelectWeapon(source)) || !Owner->IsControlledByHuman()
            || DistanceFrom3D(source)<=(double(type->GuardRange)+0.5)*256.0))Override_Mission(Mission::Attack,source,nullptr);
        if(foot && !Target && !static_cast<FootClass*>(this)->Destination && scatters)Scatter(CoordStruct::Empty,true,false);
    }else if(foot && CurrentMissionControl()->Scatter && !IsTether) {
        auto* unit=static_cast<FootClass*>(this);
        if(!unit->Locomotor->Is_Moving() && !Target && !unit->Destination && WhatAmI()!=AbstractType::Aircraft
            && (!Owner->IsControlledByHuman() || scatters))Scatter(CoordStruct::Empty,true,false);
    }
    return result;
}

bool TechnoClass::CanRetaliateToAttacker(ObjectClass* attacker,WarheadTypeClass*) {
    auto* type=GetTechnoType();const bool human=Owner->IsControlledByHuman();
    if(!attacker || !type->CanRetaliate || SlaveOwner || SlaveManager || (DrainTarget && !Owner->IsHumanPlayer))return false;
    if(CaptureManager && CaptureManager->CannotControlAnyMore())return false;
    if(SpawnManager || (human && Target) || !CurrentMissionControl()->Retaliate || Owner->IsAlliedWith(attacker)
        || attacker->IsDisguisedAs(Owner) || CombatDamage(-1)<=0 || !IsArmed())return false;
    const int index=SelectWeapon(attacker);const auto error=GetFireErrorWithoutRange(attacker,index);
    if(error==FireError::ILLEGAL || error==FireError::CANT)return false;
    if(human && attacker->WhatAmI()==AbstractType::Building &&
        ((WhatAmI()==AbstractType::Infantry && static_cast<InfantryClass*>(this)->Type->C4) || HasAbility(Ability::C4)))return false;
    if(human && WhatAmI()==AbstractType::Unit) {
        auto* deploy=static_cast<UnitClass*>(this)->Type->DeploysInto;
        if(deploy && deploy->Artillary)return false;
    }
    if(human && !RulesClass::Instance->PlayerReturnFire && WhatAmI()!=AbstractType::Building
        && CurrentMission!=Mission::Area_Guard && CurrentMission!=Mission::Guard && CurrentMission!=Mission::Sticky)return false;
    if((AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None) {
        auto* foot=static_cast<FootClass*>(this);
        if((foot->Team && foot->Team->Type->Suicide) || attacker==foot->ParasiteEatingMe)return false;
    }
    if(!human && Target && (Target->AbstractFlags & ::AbstractFlags::Object)!=::AbstractFlags::None
        && ThreatCoeffients(attacker,&CoordStruct::Empty)<ThreatCoeffients(static_cast<ObjectClass*>(Target),&CoordStruct::Empty))return false;
    auto* weapon=GetWeapon(index)->WeaponType;
    return !weapon || !weapon->Warhead || weapon->Warhead->Verses[int(attacker->GetTechnoType()->Armor)]>0.0099999998;
}
