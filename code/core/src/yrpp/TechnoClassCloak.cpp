// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Cloaking_AI, calibrated to YR 0x6FB740.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// The state machine is native; activation and disappearance
// callbacks are still original-entry dependencies, not empty success stubs.
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/Surface.h"
#include "yrpp/VocClass.h"
#include "RulesClassReaders.hpp"
#include "scenario_runtime.hpp"
#include <bit>
#include <cmath>

namespace {
void advance_cloak(StageClass& animation) {
    int left=animation.Timer.TimeLeft;
    if(animation.Timer.StartTime!=-1) {
        const int elapsed=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)-static_cast<unsigned>(animation.Timer.StartTime));
        left=elapsed>=left?0:std::bit_cast<int>(static_cast<unsigned>(left)-static_cast<unsigned>(elapsed));
    }
    if(!left && animation.Rate) {
        animation.HasChanged=true;
        animation.Value=std::bit_cast<int>(static_cast<unsigned>(animation.Value)+static_cast<unsigned>(animation.Step));
        animation.Timer.Start(animation.Rate);
    }else animation.HasChanged=false;
}
}
// OpenTS Do_Uncloak/Do_Cloak. YR does not restart CloakDelayTimer here;
// that happens only when Cloaking_AI finishes the uncloaking transition.
void TechnoClass::Uncloak(bool silent) {
    if(CloakState!=::CloakState::Cloaked && CloakState!=::CloakState::Cloaking)return;
    CloakState=::CloakState::Uncloaking;
    CloakProgress.Start(GetTechnoType()->CloakingSpeed);
    CloakProgress.Value=RulesClass::Instance->CloakingStages-1;CloakProgress.Step=-1;
    if(!silent)VocClass::PlayAt(RulesClass::Instance->CloakSound,Location,nullptr);
}
void TechnoClass::Cloak(bool silent) {
    if(CloakState!=::CloakState::Uncloaked && CloakState!=::CloakState::Uncloaking)return;
    Disappear(false);CloakState=::CloakState::Cloaking;
    CloakProgress.Start(GetTechnoType()->CloakingSpeed);CloakProgress.Value=0;CloakProgress.Step=1;
    if(!silent)VocClass::PlayAt(RulesClass::Instance->CloakSound,Location,nullptr);
    if(Owner!=HouseClass::CurrentPlayer && IsSelected)Deselect();
}
// OpenTS Visual_Character; YR 0x703860. Raw targeting queries do not
// inherit the player-facing shadow silhouette of friendly cloaked objects.
VisualType TechnoClass::VisualCharacter(VARIANT_BOOL raw,HouseClass* asking) const {
    if(GetTechnoType()->Invisible && IsOwnedByCurrentPlayer)return VisualType::Normal;
    if(GetTechnoType()->Invisible && !IsOwnedByCurrentPlayer && !Unsorted::ArmageddonMode)return VisualType::Hidden;
    if(CloakState==::CloakState::Uncloaked || Unsorted::ArmageddonMode || WhatAmI()==AbstractType::Building)return VisualType::Normal;
    if(CloakState==::CloakState::Cloaked) {
        if(static_cast<unsigned char>(raw))return asking && IsSensorVisibleToHouse(asking)?VisualType::Shadowy:VisualType::Hidden;
        // Same native presentation contract as IsRadarVisible: a live
        // window has non-empty WindowBounds, not a fabricated Win32 handle.
        if(DSurface::WindowBounds.Width<=0 || DSurface::WindowBounds.Height<=0
            || IsOwnedByCurrentPlayer || IsSensorVisibleToPlayer())return VisualType::Shadowy;
        const auto& runtime=game::scenario_runtime();
        if(runtime.session_mode(runtime.context)!=0 && Owner && HouseClass::CurrentPlayer
            && HouseClass::CurrentPlayer->IsAlliedWith(Owner) && Owner->IsAlliedWith(HouseClass::CurrentPlayer))return VisualType::Shadowy;
        return VisualType::Hidden;
    }
    if(CloakProgress.Value<=0)return VisualType::Normal;
    const int stage=rule_integer(double(CloakProgress.Value)/RulesClass::Instance->CloakingStages*256.0);
    if(stage<0x40)return VisualType::Indistinct;
    if(stage<0x80)return VisualType::Darken;
    if(stage<0xC0 || (!static_cast<unsigned char>(raw) && IsOwnedByCurrentPlayer))return VisualType::Shadowy;
    return stage<0xFF?VisualType::Ripple:VisualType::Hidden;
}
void TechnoClass::UpdateCloak(bool) {
    if(CloakState==::CloakState::Uncloaked) {
        if(!((IsCloakable() && !IsUnderEMP() && !IsParalyzed() && !IsBeingWarpedOut() && !IsWarpingIn())
            || HasAbility(Ability::Cloak)))return;
        // Native map placement has not allocated transport/radio links for
        // every type yet. No capacity means no dock, not an invented link.
        auto* dock=RadioLinks.Capacity?GetNthLink(0):nullptr;
        if(dock && dock->WhatAmI()==AbstractType::Building && static_cast<BuildingClass*>(dock)->Type->WeaponsFactory)return;
        advance_cloak(CloakProgress);
        if(IsReadyToCloak() && (GetHealthPercentage()>RulesClass::Instance->ConditionRed
            || ScenarioClass::Instance->Random.RandomRanged(0,99)<4))Cloak(false);
        return;
    }
    advance_cloak(CloakProgress);
    if(CloakProgress.Value<0)CloakProgress.Value=0;
    switch(CloakState) {
        case ::CloakState::Cloaking: {
            Mark(MarkType::Change);
            if(!CloakProgress.Rate)CloakProgress.Start(1);
            const auto visual=VisualCharacter(1,nullptr);
            if(visual==VisualType::Darken) {
                if(GetHealthPercentage()<=RulesClass::Instance->ConditionRed
                    && ScenarioClass::Instance->Random.RandomRanged(0,99)<10)Uncloak(true);
            }else if(visual==VisualType::Shadowy || visual==VisualType::Hidden) {
                CloakState=::CloakState::Cloaked;CloakProgress.Start(0);CloakProgress.Value=0;Mark(MarkType::Change);
                if(WhatAmI()==AbstractType::Unit && static_cast<UnitClass*>(this)->FlagHouseIndex!=-1)Reveal();
                else {
                    DynamicVectorClass<TechnoClass*> attackers;
                    for(int i=Array.Count-1;i>=0;--i) {
                        auto* attacker=Array[i];
                        if(attacker->Target==this && (MapClass::Instance.GetCellAt(GetCoords())->Sensors_InclHouse(attacker->Owner->ArrayIndex)
                            || attacker->Owner==Owner))attackers.AddItem(attacker);
                    }
                    Disappear(false);
                    for(int i=attackers.Count-1;i>=0;--i)attackers[i]->SetTarget(this);
                }
                if(WhatAmI()==AbstractType::Unit && !Owner->IsControlledByHuman())Scatter(CoordStruct::Empty,true,false);
            }
        }break;
        case ::CloakState::Cloaked:if(ShouldNotBeCloaked())Uncloak(false);break;
        case ::CloakState::Uncloaking: {
            Mark(MarkType::Change);
            const auto visual=VisualCharacter(1,nullptr);
            if(visual==VisualType::Normal) {
                CloakProgress.Start(0);CloakProgress.Value=0;CloakState=::CloakState::Uncloaked;
                const double delay=RulesClass::Instance->CloakDelay*900.0;
                CloakDelayTimer.Start(std::isfinite(delay)&&delay>=-2147483648.0&&delay<2147483648.0?int(delay):INT32_MIN);
                Mark(MarkType::Change);
            }else if(visual==VisualType::Indistinct && IsReadyToCloak())Cloak(true);
        }break;
        default:break;
    }
}
