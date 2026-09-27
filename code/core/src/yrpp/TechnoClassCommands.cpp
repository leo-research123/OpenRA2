// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::Player_Assign_Mission; YR 0x6FFBE0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/VocClass.h"
#include "clock.hpp"
#include "map_world.hpp"
#include <cstdlib>

#if !defined(RA2_YRPP_GAME)
// 0x00734270 also accepts null ECX; the static original-command entry carries
// that case without a null C++ member invocation.
void TechnoClass::ClearSidebarTabObject() const noexcept {Game::ClearSidebarTabObject(this);}
#endif

void TechnoClass::AddPassenger(FootClass* passenger){
 Passengers.AddPassenger(passenger);
 if(passenger&&GetTechnoType()->EnterTransportSound!=-1)VocClass::PlayIndexAtPos(GetTechnoType()->EnterTransportSound,GetCoords(),0);
}

void TechnoClass::Guard(){
 SetDestination(nullptr,true);SetTarget(nullptr);ArchiveTarget=nullptr;ForceMission(Mission::Guard);
}

// OpenTS Override/Restore_Mission; YR 0x7013A0 / 0x7013E0 retain the target
// alongside the base mission state. Retaliation must assign its attacker.
void TechnoClass::Override_Mission(Mission mission,AbstractClass* target,AbstractClass* destination){
 LastTarget=Target;MissionClass::Override_Mission(mission,target,destination);SetTarget(target);
}
bool TechnoClass::Mission_Revert(){
 if(!MissionClass::Mission_Revert())return false;
 SetTarget(LastTarget);return true;
}

Mission TechnoClass::RespondMegaEventMission(EventClass* event) {
    return static_cast<Mission>(static_cast<signed char>(event->MegaMission.Mission));
}
void TechnoClass::QueueVoice(int sound) {
    if(Unsorted::MoveFeedback && sound!=-1 && Owner->IsControlledByCurrentPlayer())QueuedVoiceIndex=static_cast<DWORD>(sound);
}
int TechnoClass::VoiceEnter() {
 const int voice=GetTechnoType()->VoiceEnter;
 if(voice==-1)return VoiceMove();QueueVoice(voice);return 0;
}
int TechnoClass::VoiceHarvest() {
 const int voice=GetTechnoType()->VoiceHarvest;
 if(voice==-1)return VoiceMove();QueueVoice(voice);return 0;
}
int TechnoClass::VoiceCapture() {
 const int voice=GetTechnoType()->VoiceCapture;
 if(voice==-1)return VoiceEnter();QueueVoice(voice);return 0;
}
int TechnoClass::VoiceMove() {
    auto& voices=GetTechnoType()->VoiceMove;
    if(!voices.Count)return 0; // Original EAX is unused (an incidental vector address).
    const auto random=static_cast<unsigned>(Randomizer::Global.Random());
    QueueVoice(voices[int(random%static_cast<unsigned>(voices.Count))]);return 0;
}
int TechnoClass::VoiceDeploy() {
    if(!unknown_bool_4F8){
        const auto* infantry=WhatAmI()==AbstractType::Infantry?static_cast<const InfantryClass*>(this):nullptr;
        const auto sequence=infantry?infantry->SequenceAnim:Sequence::Nothing;
        const bool deployed=infantry&&sequence>=Sequence::Deploy&&sequence<=Sequence::DeployedIdle;
        const int voice=deployed?GetTechnoType()->VoiceUndeploy:GetTechnoType()->VoiceDeploy;
        if(voice!=-1)QueueVoice(voice);
    }
    unknown_bool_4F8=false;return 0;
}
int TechnoClass::VoiceAttack(AbstractClass* target) {
    const int weapon=SelectWeapon(target);
    auto* kind=GetWeapon(weapon)->WeaponType;
    int voice=-1;
    if(kind && kind->Damage<0 && !_strcmpi(GetTechnoType()->ID,"FV"))voice=RulesClass::Instance->VoiceIFVRepair;
    else if(Veterancy.GetRemainingLevel()==Rank::Elite)
        voice=weapon?GetTechnoType()->VoiceSecondaryEliteWeaponAttack:GetTechnoType()->VoicePrimaryEliteWeaponAttack;
    else voice=weapon?GetTechnoType()->VoiceSecondaryWeaponAttack:GetTechnoType()->VoicePrimaryWeaponAttack;
    auto& voices=GetTechnoType()->VoiceAttack;
    if(voice==-1 && voices.Count) {
        const auto random=static_cast<unsigned>(Randomizer::Global.Random());
        voice=voices[int(random%static_cast<unsigned>(voices.Count))];
    }
    if(voice!=-1)QueueVoice(voice);return 0; // No caller consumes the legacy incidental EAX.
}
void TechnoClass::Flash(int duration) {
    if(!IsDisguisedAs(HouseClass::CurrentPlayer) || GetDisguise(true)){
        const bool changed=((Flashing.DurationRemaining^duration)&2)!=0;
        Flashing.DurationRemaining=duration;
        if(changed)game::map_object_changed();
    }
}
void YRPP_FASTCALL TechnoClass::ClearPlanningTokens(EventClass* event) {
    Game::PlanningManager_ClearToken(this,event);
}

// OpenTS Can_Deploy_Now selection admission, calibrated to
// YR Techno::CanDeploySlashUnload 0x700D50. Infantry keeps its own override.
bool TechnoClass::CanDeploySlashUnload() const {
    auto* cell=GetCell();
    if(cell&&cell->IsNearTunnelNW())return false;
    if(WhatAmI()==AbstractType::Building)return false; // 0x465D30
    if(WhatAmI()!=AbstractType::Unit)return !IsUnderEMP()&&GetTechnoType()->Passengers!=0;
    const auto& unit=*static_cast<const UnitClass*>(this);const auto* type=unit.Type;
    if(IsUnderEMP()||static_cast<signed char>(unit.TubeIndex)>=0)return false;
    if(!type->DeploysInto&&!type->Passengers&&!type->IsSimpleDeployer&&!BunkerLinkedItem&&!type->DeployFire)return false;
    if(type->DeploysInto&&(unit.ParasiteEatingMe||(type->DeploysInto->ConstructionYard&&CaptureManager)))return false;
    if(type->Passengers>0){
        if(OnBridge)return false;
        if(cell){
            if(unsigned(cell->Flags)&0x100u)return false;
            for(int direction:{4,0,2,6}){
                const auto delta=Unsorted::AdjacentCell[direction];
                auto* adjacent=MapClass::Instance.TryGetCellAt(CellStruct{short(cell->MapCoords.X+delta.X),short(cell->MapCoords.Y+delta.Y)});
                if(adjacent&&(unsigned(adjacent->Flags)&0x100u))return false;
            }
            if(Owner->IsHumanPlayer&&cell->LandType==LandType::Water)return false;
            if(auto* building=cell->GetBuilding();building&&!building->Type->Bunker)return false;
        }
    }
    return true;
}

#if !defined(RA2_YRPP_GAME)
int TechnoClass::GetRefund() const {return GetTechnoType()->GetRefund(Owner,false);}
#endif
