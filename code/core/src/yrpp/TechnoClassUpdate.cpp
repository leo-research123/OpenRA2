// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::AI, calibrated to YR 0x6F9E50.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Frame orchestration and the small frame-owned state machines. Calls into
// unfinished effects/targeting remain explicit original-entry dependencies.
#include "yrpp/TechnoClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/BombClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/FileFormats/VXL.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include "scenario_runtime.hpp"
#include "type_resources.hpp"
#include "map_world.hpp"

namespace {
int wrap(unsigned value){return std::bit_cast<int>(value);}
int add(int a,int b){return wrap(static_cast<unsigned>(a)+static_cast<unsigned>(b));}
int integer(double value){return std::isfinite(value)&&value>=-2147483648.0&&value<2147483648.0?int(value):INT32_MIN;}
int left(const CDTimerClass& timer) {
    const int elapsed=wrap(static_cast<unsigned>(Unsorted::CurrentFrame)-static_cast<unsigned>(timer.StartTime));
    return timer.StartTime==-1?timer.TimeLeft:elapsed>=timer.TimeLeft?0:wrap(static_cast<unsigned>(timer.TimeLeft)-static_cast<unsigned>(elapsed));
}
template<class Stage> void tint(Stage& stage,CDTimerClass& pulse,const CDTimerClass& duration,bool airstrike) {
    const auto change=[&](int next,int frames){stage=next;pulse.Start(frames);};
    switch(stage) {
        case 0:change(1,6);break;
        case 1:if(!left(pulse))change(2,4);break;
        case 2:if(!left(pulse))change(3,ScenarioClass::Instance->Random.RandomRanged(-5,5)+20);break;
        case 3:if(!left(pulse))change(4,airstrike?64:8);break;
        case 4:if(!left(pulse))change(5,airstrike?64:16);break;
        case 5:if(!left(pulse)) {
            if(duration.GetTimeLeft()>=(airstrike?158:54))change(4,airstrike?64:8);
            else stage=6;
        }break;
        case 6:if(duration.GetTimeLeft()<=30)change(7,6);break;
        case 7:if(!left(pulse))change(8,4);break;
        case 8:if(!left(pulse))change(9,20);break;
        case 9:if(!left(pulse))stage=10;break;
        default:stage=10;break;
    }
}
void update_audio_position(const CoordStruct& at,AudioController& controller) {
    // 0x750D40's no-event-type branch; actual sound attenuation remains a
    // separate dependency rather than a substituted position-only callback.
    auto* event=controller.GetEvent();
    if(controller.GetEventType())VocClass::UpdatePosition(&at,&controller);
    else controller.SetEvent(event,nullptr);
}
bool allied_techno(const HouseClass* house,const AbstractClass* target) {
    // 0x4F9AF0 checks Techno, not the more permissive Object discriminator.
    if(!target || (target->AbstractFlags & ::AbstractFlags::Techno)==::AbstractFlags::None)return false;
    return house->IsAlliedWith(static_cast<const TechnoClass*>(target)->GetOwningHouse());
}
}

bool FlashData::Update() {
    if(!DurationRemaining)return false;
    DurationRemaining=add(DurationRemaining,-1);FlashingNow=(DurationRemaining&1)!=0;return true;
}
void RecoilData::Update() {
    if(State==RecoilState::Inactive)return;
    TravelSoFar=static_cast<float>(double(TravelPerFrame)+TravelSoFar);
    TravelFramesLeft=add(TravelFramesLeft,-1);
    if(TravelFramesLeft>0)return;
    if(State==RecoilState::Recovering){State=RecoilState::Inactive;TravelSoFar=0.0f;return;}
    if(State==RecoilState::Compressing) {
        State=RecoilState::Holding;TravelFramesLeft=Turret.HoldFrames;TravelPerFrame=0.0f;
        if(TravelFramesLeft>1)return;
    }else if(State!=RecoilState::Holding)return;
    State=RecoilState::Recovering;TravelFramesLeft=std::max(Turret.RecoverFrames,1);
    TravelPerFrame=static_cast<float>(-double(Turret.Travel)/TravelFramesLeft);
}
void TechnoClass::UpdateInvulnerabilityTint() {
    if(IsIronCurtained())tint(IronTintStage,IronTintTimer,IronCurtainTimer,false);
    else IronTintStage=0;
}
void TechnoClass::UpdateAirstrikeTint() {
    if(WhatAmI()==AbstractType::Building && Airstrike && Airstrike->Target==this)
        tint(AirstrikeTintStage,AirstrikeTintTimer,AirstrikeTimer,true);
    else AirstrikeTintStage=0;
}
bool TechnoClass::ShouldSelfHealOneStep() const {
    if(!GetTechnoType()->SelfHealing && !HasAbility(Ability::SelfHeal))return false;
    if(Unsorted::CurrentFrame%integer(RulesClass::Instance->RepairRate*900.0))return false;
    return Health>0 && Health<GetTechnoType()->Strength;
}
bool TechnoClass::IsVoxel() const {
    return GetTechnoType()->MainVoxel.VXL && !GetTechnoType()->MainVoxel.VXL->Initialized;
}
bool TechnoClass::IsWarpingSomethingOut() const {return TemporalTargetingMe && TemporalTargetingMe->Target;}
bool TechnoClass::CanPassiveAcquireTargets() {
    if(IsWarpingSomethingOut() || SlaveOwner || !GetTechnoType()->CanPassiveAquire)return false;
    if(WhatAmI()==AbstractType::Building && static_cast<BuildingClass*>(this)->Type->CanBeOccupied && !GetOccupantCount())return false;
    const bool full=CaptureManager && CaptureManager->CannotControlAnyMore();
    return (!IsEngineer() || !Owner->IsControlledByHuman()) && !full && IsArmed();
}
bool TechnoClass::CanOpportunityFire() {
    auto* foot=(AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(this):nullptr;
    if(!Target && !Owner->IsControlledByHuman() && foot && foot->Team
        && !foot->Team->Type->Suicide && foot->Team->Type->Aggressive && CurrentMission==Mission::Move)return true;
    if(!CanPassiveAcquireTargets())return false;
    if(CurrentMission==Mission::Move) {
        if(GetTechnoType()->BalloonHover) {
            if(!foot)return false;
            if(foot->Destination==GetCell() && (!PlanningToken || PlanningToken->PlanningNodes.Count<=0))return true;
        }
        if(WhatAmI()==AbstractType::Unit && static_cast<UnitClass*>(this)->Type->DeployToFire
            && foot->Destination==GetCell() && (!PlanningToken || PlanningToken->PlanningNodes.Count<=0))return true;
    }
    if(GetTechnoType()->OpportunityFire)return true;
    if(CurrentMission!=Mission::Guard)return false;
    const int index=IsNotSprayAttack();
    const auto* weapon=GetWeapon(index);
    const bool area=weapon && weapon->WeaponType && weapon->WeaponType->AreaFire;
    const bool same=SelectWeapon(Target)==index;
    return !area || !same;
}

void TechnoClass::Update() {
    IsMouseHovering=false;
    if(GetTechnoType()->IsGattling)update_audio_position(Location,GattlingSoundController);
    UpdateInvulnerabilityTint();UpdateAirstrikeTint();
    if(QueuedVoiceIndex!=0xFFFFFFFFu) {
        if(!QueuedVoiceSoundController.GetEvent()) {
            unknown_4F4=QueuedVoiceIndex;
            VocClass::PlayGlobal(static_cast<int>(QueuedVoiceIndex),0x2000,1.0f,&QueuedVoiceSoundController);
            QueuedVoiceIndex=0xFFFFFFFFu;
        }else if(unknown_4F4==QueuedVoiceIndex)QueuedVoiceIndex=0xFFFFFFFFu;
    }
    if(Berzerk) {
        BerzerkDurationLeft=add(BerzerkDurationLeft,-1);
        if(BerzerkDurationLeft<=0) {
            Berzerk=false;BerzerkDurationLeft=0;SetTarget(nullptr);
            QueueMission(Owner->IsHumanPlayer?Mission::Guard:Mission::Hunt,false);
        }
    }
    if(EstimatedHealth>Health)EstimatedHealth=Health;
    if((Unsorted::CurrentFrame&4) && EstimatedHealth<Health) {
        if(add(EstimatedHealth,30)<0)EstimatedHealth=-30;
        EstimatedHealth=add(EstimatedHealth,1);
    }
    if(GetTechnoType()->Turret) {
        if(TurretIsRotating) {
            if(IsTurretRotateSoundPlaying) {
                TurretRotateSoundController.Stop();
                const auto at=Location;
                // PlayAt 0x7509E0 returns immediately when audio is disabled.
                if(!game::type_resources().audio_unavailable)
                    VocClass::PlayAt(GetTechnoType()->TurretRotateSound,at,&TurretRotateSoundController);
                IsTurretRotateSoundPlaying=false;
            }
        }else {TurretRotateSoundController.EndLooping();IsTurretRotateSoundPlaying=true;}
        update_audio_position(Location,TurretRotateSoundController);
    }
    if(CurrentRanking!=Veterancy.GetRemainingLevel()) {
        if(CurrentRanking!=Rank::Invalid) {
            if(Veterancy.GetRemainingLevel()==Rank::Elite || Veterancy.GetRemainingLevel()==Rank::Veteran) {
                // Preserve promotion state and elite flashing without audio.
                // Native map hosts cannot enter the original sound thunks;
                // game-backed hosts retain both original notifications.
                if(Owner->IsControlledByCurrentPlayer() && !game::type_resources().audio_unavailable) {
                    VocClass::PlayAt(Veterancy.GetRemainingLevel()==Rank::Elite
                        ?RulesClass::Instance->UpgradeEliteSound:RulesClass::Instance->UpgradeVeteranSound,Location);
                    VoxClass::Play("EVA_UnitPromoted");
                }
                if(Veterancy.GetRemainingLevel()==Rank::Elite)Flashing.DurationRemaining=RulesClass::Instance->EliteFlashTimer;
            }
        }
        CurrentRanking=Veterancy.GetRemainingLevel();
    }
    if(DrainingMe && GetTechnoType()->ResourceDestination
        && Unsorted::CurrentFrame%RulesClass::Instance->DrainMoneyFrameDelay==0) {
        int amount=RulesClass::Instance->DrainMoneyAmount;
        if(Owner->Available_Money()<amount)amount=Owner->Available_Money();
        Owner->TakeMoney(amount);DrainingMe->Owner->GiveMoney(amount);
    }
    if(DrainTarget && Owner->IsAlliedWith(DrainTarget)) {
        if(DrainAnim){DrainAnim->UnInit();DrainAnim=nullptr;}
        if(DrainTarget) {
            DrainTarget->DrainingMe=nullptr;
            if(DrainTarget->Owner)DrainTarget->Owner->RecheckPower=true;
            DrainTarget=nullptr;
        }
    }
    if(IsVoxel()){vt_entry_41C();if(!IsAlive)return;}
    const auto at=GetCoords();
    auto* cell=MapClass::Instance.GetCellAt(CellStruct{short(at.X/256),short(at.Y/256)});
    if(!GetTechnoType()->CanBeHidden || !cell->IsCovered() || WhatAmI()==AbstractType::Aircraft) {
        if(BehindAnim){if(WhatAmI()!=AbstractType::Building)GetTechnoType();BehindAnim->UnInit();BehindAnim=nullptr;}
    }else {
        Point2D position{};RectangleStruct bounds{};
        DrawBehindMark(&position,&bounds); // original drawing/effect entry not yet native
    }
    HouseClass* controlling=Owner;
    if(Transporter && Transporter->GetTechnoType()->OpenTopped && Transporter->MindControlledBy)
        controlling=Transporter->MindControlledBy->Owner;
    if(!controlling->IsControlledByHuman() && Target && allied_techno(controlling,Target)) {
        auto* infantry=WhatAmI()==AbstractType::Infantry?static_cast<InfantryClass*>(this):nullptr;
        auto* building=Target->WhatAmI()==AbstractType::Building?static_cast<BuildingClass*>(Target):nullptr;
        const bool occupy=infantry && building && building->CanBeOccupiedBy(infantry);
        const bool engineer=infantry && infantry->Type->Engineer;
        const bool power=GetWeapon(1)->WeaponType && GetWeapon(1)->WeaponType->Warhead->ElectricAssault
            && building && building->Type->Overpowerable;
        if(!engineer && !power && !occupy && !Berzerk)SetTarget(nullptr);
    }
    if(Target && Unsorted::CurrentFrame%16==0 && CurrentMission!=Mission::Capture && CurrentMission!=Mission::Sabotage) {
        const auto error=GetFireErrorWithoutRange(Target,SelectWeapon(Target));
        if(static_cast<int>(error)==5 || static_cast<int>(error)==6)SetTarget(nullptr);
    }
    if(GetTechnoType()->TurretRecoil){TurretRecoil.Update();BarrelRecoil.Update();}
    if(GetTechnoType()->IsChargeTurret && GetTechnoType()->HasMultipleTurrets() && !GetTechnoType()->IsGattling) {
        int stage=0;
        if(ChargeTurretDelay>0) {
            const int scaled=wrap(static_cast<unsigned>(left(RearmTimer))*static_cast<unsigned>(GetTechnoType()->TurretCount));
            stage=scaled/ChargeTurretDelay;
            if(stage>=GetTechnoType()->TurretCount)stage=GetTechnoType()->TurretCount-1;
            else if(stage<0)stage=0;
        }
        CurrentTurretNumber=stage;
    }
    if(UnloadTimer.IsTimerFinished())UnloadTimer.SetToDone();
    if(unknown_bool_41E)unknown_bool_41E=add(unknown_bool_41E,-1);
    if(Target && (ShouldLoseTargetNow&0xFFu))switch(CurrentMission) {
        case Mission::Sleep:case Mission::Enter:case Mission::Stop:case Mission::Ambush:case Mission::Unload:
        case Mission::Construction:case Mission::Selling:case Mission::Repair:case Mission::Missile:
        case Mission::Harmless:case Mission::Wait:case Mission::Open:SetTarget(nullptr);break;
        default:break;
    }
    MissionAccumulateTime=add(MissionAccumulateTime,1);
    MissionClass::Update();
    if(!left(TargetingTimer)) {
        if(MegaMissionIsAttackMove())UpdateAttackMove();
        else if((CurrentMission==Mission::Move || CurrentMission==Mission::Harvest || CurrentMission==Mission::Guard)
            && CanOpportunityFire()) {
            auto* old=Target;unknown_4FC=Unsorted::CurrentFrame;
            auto origin=GetCoords();
            if(TargetAndEstimateDamage(origin,static_cast<ThreatType>(1)) && Target!=old)ShouldLoseTargetNow=(ShouldLoseTargetNow&0xFFFFFF00u)|1u;
        }
    }
    if(AttachedBomb && !InLimbo && AttachedBomb->TimeToExplode())AttachedBomb->Detonate();
    if(SlaveManager)SlaveManager->Update();
    if(CaptureManager)CaptureManager->HandleOverload();
    if(!IsAlive)return;
    if(ShouldSelfHealOneStep()) {
        Health=add(Health,1);
        if(GetHealthPercentage()>RulesClass::Instance->ConditionYellow || GetHeight()<-10)
            if(DamageParticleSystem)DamageParticleSystem->UnInit();
    }
    const bool full=Health>=GetTechnoType()->Strength || Health==0;
    if(WhatAmI()==AbstractType::Unit && !GetTechnoType()->Organic && !full) {
        if(Unsorted::CurrentFrame%RulesClass::Instance->SelfHealUnitFrames==0 && Owner->DoUnitsSelfHeal()) {
            Health=add(Health,std::min(Owner->GetUnitSelfHealStep(),add(GetTechnoType()->Strength,-Health)));
            if(GetHealthPercentage()>RulesClass::Instance->ConditionYellow || GetHeight()<-10)
                if(DamageParticleSystem)DamageParticleSystem->UnInit();
        }
    }else if((WhatAmI()==AbstractType::Infantry || (WhatAmI()==AbstractType::Unit && GetTechnoType()->Organic))
        && !full && Unsorted::CurrentFrame%RulesClass::Instance->SelfHealInfantryFrames==0 && Owner->DoInfantrySelfHeal())
        Health=add(Health,std::min(Owner->GetInfSelfHealStep(),add(GetTechnoType()->Strength,-Health)));
    UpdateCloak(false);
    if(SpawnManager)SpawnManager->Update();
    if(CloakState==::CloakState::Uncloaked && MapClass::Instance.GetCellAt(GetCoords())->CloakGen_InclHouse(Owner->ArrayIndex))Sensed();
    if(CloakState==::CloakState::Cloaked && !MapClass::Instance.GetCellAt(GetCoords())->CloakGen_InclHouse(Owner->ArrayIndex))Sensed();
    const auto& runtime=game::scenario_runtime();
    const bool campaign=runtime.session_mode(runtime.context)==static_cast<int>(GameMode::Campaign);
    if(campaign && !Owner->IsControlledByCurrentPlayer() && Target && !allied_techno(Owner,Target) && CombatDamage(-1)<0)SetTarget(nullptr);
    if(Target && !campaign && Owner->IsHumanPlayer && CombatDamage(-1)<0 && !allied_techno(Owner,Target))SetTarget(nullptr);
    if(Target && Target->WhatAmI()==AbstractType::Aircraft && WhatAmI()==AbstractType::Unit && CombatDamage(-1)<0) {
        auto* aircraft=static_cast<AircraftClass*>(Target);
        if(aircraft->GetHeight()>0 || MapClass::Instance.GetCellAt(aircraft->Location)->GetBuilding())SetTarget(nullptr);
    }
    if(WhatAmI()!=AbstractType::Aircraft && Target && !BelongsToATeam()) {
        auto* foot=(AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None?static_cast<FootClass*>(this):nullptr;
        if((!foot || !foot->Destination) && (!IsInSameZoneAs(Target) || InOpenToppedTransport)) {
            if(foot)foot->ApproachTarget(0);
            if((!foot || !foot->Destination) && !IsCloseEnough(Target,SelectWeapon(Target)))SetTarget(nullptr);
        }
    }
    if(WhatAmI()!=AbstractType::Building) {
        if(!left(Animation.Timer) && Animation.Rate) {
            Animation.HasChanged=true;Animation.Value=add(Animation.Value,Animation.Step);Animation.Timer.Start(Animation.Rate);
        }else Animation.HasChanged=false;
    }
    const int old_flash=Flashing.DurationRemaining;const bool was_lit=(old_flash&2)==2;
    if(Flashing.Update())Mark(MarkType::Change);
    // The host retains derived sprite intensity. NeedsRedraw can already be
    // set, so original Mark(Change) alone does not invalidate that cache.
    if(was_lit!=((Flashing.DurationRemaining&2)==2))game::map_object_changed();
    if(old_flash && old_flash!=Flashing.DurationRemaining && WhatAmI()==AbstractType::Building
        && was_lit!=((Flashing.DurationRemaining&2)==2)) {
        RectangleStruct bounds;GetRenderDimensions(&bounds);TacticalClass::Instance->RegisterDirtyArea(bounds,false);
        static_cast<BuildingClass*>(this)->UpdateAnimAppearance();
    }
    auto* type=GetTechnoType();
    if(type->DamageSparks && GetHealthPercentage()<RulesClass::Instance->ConditionYellow && GetHeight()>-10) {
        DynamicVectorClass<ParticleSystemTypeClass*> sparks;
        for(int i=0;i<type->DamageParticleSystems.Count;++i)
            if(static_cast<int>(type->DamageParticleSystems[i]->BehavesLike)==3)sparks.AddItem(type->DamageParticleSystems[i]);
        if(!SparkParticleSystem && sparks.Count>0) {
            const double chance=GetHealthPercentage()<RulesClass::Instance->ConditionRed
                ?RulesClass::Instance->ConditionRedSparkingProbability:RulesClass::Instance->ConditionYellowSparkingProbability;
            if(ScenarioClass::Instance->Random.RandomRanged(0,2147483646)*4.656612877414201e-10<chance) {
                if(void* storage=YRMemory::Allocate(sizeof(ParticleSystemClass))) {
                    const auto offset=GetTechnoType()->GetParticleSysOffset();const auto here=GetCoords();
                    const CoordStruct location{add(here.X,offset.X),add(here.Y,offset.Y),add(here.Z,offset.Z)};
                    auto* selected=sparks[ScenarioClass::Instance->Random.RandomRanged(0,sparks.Count-1)];
                    SparkParticleSystem=::new(storage) ParticleSystemClass(selected,location,nullptr,this,CoordStruct::Empty,nullptr);
                }else SparkParticleSystem=nullptr;
            }
        }
    }
    RadarTrackingUpdate(false);
    if(wrap(EMPLockRemaining)>0 && !--EMPLockRemaining) {
        if(WhatAmI()==AbstractType::Building) {
            auto* building=static_cast<BuildingClass*>(this);
            if(!building->Type->InvisibleInGame){building->EnableStuff();if(building->Type->Radar)Owner->RecheckRadar=true;}
        }else if((AbstractFlags & ::AbstractFlags::Foot)!=::AbstractFlags::None) {
            auto* foot=static_cast<FootClass*>(this);if(foot->Locomotor)foot->Locomotor->Power_On();
            for(int i=0;i<AnimClass::Array.Count;++i) {
                auto* anim=AnimClass::Array[i];
                if(anim && anim->OwnerObject==this && anim->Type==RulesClass::Instance->EMPulseSparkles)anim->RemainingIterations=0;
            }
        }
    }
}
