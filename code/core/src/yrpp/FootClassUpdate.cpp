// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::AI, calibrated to YR 0x4DA530.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// This is the original frame layer, not a host movement driver. The inherited
// Techno frame and active special-effect entries remain explicit dependencies.
#include "yrpp/FootClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/LocomotionClass.h"
#include <bit>
#include <cstdlib>

namespace {
int remaining(const CDTimerClass& timer) {
    int left=timer.TimeLeft;
    if(timer.StartTime!=-1) {
        const int elapsed=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)-static_cast<unsigned>(timer.StartTime));
        left=elapsed>=left?0:std::bit_cast<int>(static_cast<unsigned>(left)-static_cast<unsigned>(elapsed));
    }
    return left;
}
void update_sound_position(const CoordStruct& at,AudioController& controller) {
    // Inline 0x750D40's no-event-type branch. Positional audio with an actual
    // Voc type still needs its attenuation/event-creation implementation.
    auto* event=controller.GetEvent();
    if(controller.GetEventType())VocClass::UpdatePosition(&at,&controller);
    else controller.SetEvent(event,nullptr);
}
}

void FootClass::Update() {
    TechnoClass::Update();
    if(!IsAlive)return;
    unknown_bool_6B3=false;
    if(Unsorted::CurrentFrame%RulesClass::Instance->RadApplicationDelay==0
        && !GetTechnoType()->ImmuneToRadiation && !IsInAir() && !InLimbo) {
        const auto at=GetCoords();
        int radiation=MapClass::Instance.GetCellAt(CellStruct{short(at.X/256),short(at.Y/256)})->GetRadLevel();
        if(radiation>0) {
            int damage=static_cast<int>(radiation*RulesClass::Instance->RadLevelFactor);
            ReceiveDamage(&damage,0,RulesClass::Instance->RadSiteWarhead,nullptr,false,true,nullptr);
            if(!IsAlive)return;
        }
    }
    if(!IsInPlayfield && GetTechnoType()->BalloonHover
        && MapClass::Instance.IsWithinUsableArea(GetCell()->MapCoords,true))IsInPlayfield=true;
    if(!Locomotor)std::abort();
    if(Locomotor->Is_Moving_Now() && IsInAir() && HouseClass::CurrentPlayer->IsAlliedWith(Owner)
        && !remaining(SightTimer)) {
        vt_entry_48C(0,0,0,0);UpdateSight(0,0,0,0,0);
        const bool same_height=LastSightHeight==GetHeight();
        auto at=Location;
        MapClass::Instance.RevealArea3(&at,same_height?LastSightRange-3:0,LastSightRange+3,false);
        LastSightHeight=GetHeight();SightTimer.Start(15);
    }
    if(Unsorted::CurrentFrame%16==0 && IsInAir() && GetCell()->AttachedTag) {
        const auto at=GetMapCoords();
        GetCell()->AttachedTag->RaiseEvent(TriggerEvent::EnteredOrOverflownBy,this,at);
    }
    const int old_walk=WalkedFramesSoFar;
    // Techno + 0x2A8 is the reciprocal rocker/brute link, NOT the EMP timer.
    if(Locomotor && !IsSinking && !IsFallingDown
        && !(DirectRockerLinkedUnit && !GetTechnoType()->Pushy) && !InLimbo) {
        Locomotor->Process();
        if(!IsAlive)return;
        const auto can_animate=[&]{return !IsBeingWarpedOut() && !IsWarpingIn() && !IsAttackedByLocomotor;};
        const auto* unit=WhatAmI()==AbstractType::Unit?static_cast<UnitClass*>(this):nullptr;
        if((((Locomotor->Is_Moving_Now() && !IsWarpingIn())
                || (Target && GetTechnoType()->HoverAttack && unit && !unit->Deployed))
                && Unsorted::CurrentFrame%GetTechnoType()->WalkRate==0 && can_animate())
            || (GetTechnoType()->IdleRate && !Locomotor->Is_Moving_Now()
                && Unsorted::CurrentFrame%GetTechnoType()->IdleRate==0 && can_animate())
            || (GetTechnoType()->DeployToLand && GetHeight()>0 && can_animate()))
            WalkedFramesSoFar=std::bit_cast<int>(static_cast<unsigned>(WalkedFramesSoFar)+1u);
    }
    const bool sound_stops=(old_walk==WalkedFramesSoFar && !Locomotor->Is_Moving_Now())
        || IsFallingDown || IsCrashing || (DirectRockerLinkedUnit && GetTechnoType()->Pushy);
    if(sound_stops) {
        if(IsMoveSoundPlaying) {
            if(!MoveSoundDelay || IsFallingDown || IsCrashing) {
                MoveSoundAudioController.EndLooping();IsMoveSoundPlaying=false;MoveSoundDelay=0;
            }else --MoveSoundDelay;
        }
    }else {
        if(!IsMoveSoundPlaying && GetTechnoType()->MoveSound.Count>0) {
            MoveSoundAudioController.Stop();
            const auto at=Location;const auto& sounds=GetTechnoType()->MoveSound;
            const unsigned index=static_cast<unsigned>(Randomizer::Global.Random())%static_cast<unsigned>(sounds.Count);
            VocClass::PlayAt(sounds[index],at,&MoveSoundAudioController);IsMoveSoundPlaying=true;
        }
        MoveSoundDelay=3;
    }
    if(IsFallingDown!=WasFallingDown) {
        if(IsFallingDown && GetTechnoType()->VoiceFalling!=-1 && IsABomb)
            VocClass::PlayAt(GetTechnoType()->VoiceFalling,Location);
        WasFallingDown=IsFallingDown;
    }
    if(IsSinking!=WasSinkingAlready) {
        if(IsSinking) {
            if(GetTechnoType()->VoiceSinking!=-1)VocClass::PlayAt(GetTechnoType()->VoiceSinking,Location);
            const int sound=GetTechnoType()->SinkingSound!=-1?GetTechnoType()->SinkingSound:RulesClass::Instance->SinkingSound;
            if(sound!=-1)VocClass::PlayAt(sound,Location,&MoveSoundAudioController);
        }else if(!IsMoveSoundPlaying)MoveSoundAudioController.EndLooping();
        WasSinkingAlready=IsSinking;
    }
    if(IsCrashing!=WasCrashingAlready) {
        if(IsCrashing) {
            MoveSoundAudioController.EndLooping();
            if(GetTechnoType()->VoiceCrashing!=-1 && Owner->IsControlledByCurrentPlayer())
                VocClass::PlayAt(GetTechnoType()->VoiceCrashing,Location);
            if(GetTechnoType()->CrashingSound!=-1)VocClass::PlayAt(GetTechnoType()->CrashingSound,Location,&MoveSoundAudioController);
        }else if(!IsMoveSoundPlaying)MoveSoundAudioController.EndLooping();
        WasCrashingAlready=IsCrashing;
    }
    update_sound_position(Location,MoveSoundAudioController);
    if((Unsorted::CurrentFrame&0x3F)==0x3F && !Destination && !OnBridge
        && !GetCell()->SlopeIndex && vt_entry_2B0() && !GetHeight())Scatter(CoordStruct::Empty,true,false);
    struct HeldPiggy {IPiggyback* pointer=nullptr;~HeldPiggy(){if(pointer)pointer->Release();}} piggy;
    if(Locomotor) {
        constexpr GUID iid{0x92FEA800,0xA184,0x11D1,{0xB7,0x0A,0,0xA0,0x24,0xDD,0xAF,0xD1}};
        const HRESULT result=Locomotor->QueryInterface(iid,reinterpret_cast<void**>(&piggy.pointer));
        if(result<0 && result!=static_cast<HRESULT>(0x80004002u))std::abort();
        if(piggy.pointer && piggy.pointer->Is_Ok_To_End()) {
#if defined(_MSC_VER)
            Locomotor=nullptr;
#else
            if(Locomotor)Locomotor->Release();Locomotor=nullptr;
#endif
            ILocomotion* restored=nullptr;piggy.pointer->End_Piggyback(&restored);
#if defined(_MSC_VER)
            Locomotor.Attach(restored);
#else
            Locomotor=restored;
#endif
        }
    }
    DrawingYOffset=0;
    // 0x70D7E0 returns immediately for a null queue. Keep active docking a
    // genuine dependency, rather than using RadioClass's placeholder result.
    if(!IsWarpingIn() && QueueUpToEnter)UpdateEnterQueue();
    if(ParasiteEatingMe)ParasiteEatingMe->ParasiteImUsing->Update();
}

bool FootClass::IsCloakable() const {
    if(!TechnoClass::IsCloakable())return false;
    if(!GetTechnoType()->CloakStop)return true;
    if(!Locomotor)std::abort();
    return !Locomotor->Is_Moving_Now();
}
bool FootClass::IsParalyzed() const {return remaining(ParalysisTimer)!=0;}
// OpenTS Visual_Character; YR 0x4DA4E0 lets the active locomotor override
// rendering/target visibility before applying the ordinary cloak rules.
VisualType FootClass::VisualCharacter(VARIANT_BOOL raw,HouseClass* asking) const {
    if(Locomotor) {
        const auto visual=Locomotor->Visual_Character(static_cast<unsigned char>(raw)!=0);
        if(visual!=VisualType::Normal)return visual;
    }
    return TechnoClass::VisualCharacter(raw,asking);
}
