// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 code/radar.cpp:
// Init_Clear / Radar_Activate / Set_Radar_State / Toggle_Radar / Draw_It.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Original state transitions: 0x00656BE0, 0x00656CB0, 0x00656DF0;
// animation fragment of Draw: 0x006531DB..0x006532A4.
#include "yrpp/RadarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/VocClass.h"
#include "scenario_runtime.hpp"

void RadarClass::Init_Clear() {
    DisplayClass::Init_Clear();
    unknown_14AC=unknown_14B0=unknown_14FC=0;
    unknown_bool_14BC=unknown_bool_14BD=false;
    IsAvailableNow=false;
    unknown_bool_14D9=true;
    unknown_cells_1124.Clear();
    unknown_points_125C.Clear();
}
void RadarClass::ActivateRadar(bool enable, bool playSound) noexcept {
    if (enable) {
        if (unknown_14AC!=0 && unknown_14AC!=2) return;
        unknown_14AC=3;
        RadarAudio.EndLooping();
    } else {
        RadarAudio.EndLooping();
        unknown_14AC=2;
    }
    if (playSound && RulesClass::Instance)
        VocClass::PlayGlobal(enable ? RulesClass::Instance->RadarOn : RulesClass::Instance->RadarOff,0x2000,1.0f,nullptr);
}
void RadarClass::SetRadarMode(int mode, bool playSound) noexcept {
#if defined(RA2_YRPP_GAME)
    const auto* session=&SessionClass::Instance;
#else
    const auto* houses=game::scenario_runtime().houses;
    const auto* session=houses ? houses->session : nullptr;
#endif
    if (session && session->GameMode!=GameMode::Campaign && HouseClass::CurrentPlayer &&
        HouseClass::CurrentPlayer->Defeated && session->MultiplayerObserver) mode=1;
    if (unknown_14B0==static_cast<DWORD>(mode)) return;
    if ((unknown_14B0==3 || (unknown_14B0==4 && mode!=3)) && unknown_14AC!=5) {
        unknown_14B4=static_cast<DWORD>(mode); return;
    }
    if (!mode) ActivateRadar(false,unknown_14B0==1);
    else if (mode==1) {
        if ((unknown_14AC==1 || unknown_14AC==5) && IsAvailableNow) QueueNextMovie();
        else ActivateRadar(IsAvailableNow,playSound);
    } else if (mode!=4) {
        if (unknown_14AC!=1 && unknown_14AC!=5) ActivateRadar(true,false);
        else QueueNextMovie();
    }
    unknown_14B0=static_cast<DWORD>(mode);
}
void RadarClass::SetRadarAvailability(bool available) noexcept {
    if (available==IsAvailableNow) return;
    IsAvailableNow=available;
    if (unknown_14B0==1) ActivateRadar(available);
    else SetRadarMode(1);
}
bool RadarClass::AdvanceRadarAnimation() noexcept {
    if (unknown_14AC==0 || unknown_14AC==1 || unknown_timer_1500.HasTimeLeft()) return false;
    unknown_timer_1500.Start(4); // System timer ticks are 16 ms, original 0x00653245.
    if (unknown_14AC==2) {
        if (unknown_14FC<=1) { unknown_14FC=0; unknown_14AC=0; RadarAudio.EndLooping(); }
        else --unknown_14FC;
    } else {
        if (++unknown_14FC>=32) { RadarAudio.EndLooping(); unknown_14FC=32; unknown_14AC=1; unknown_bool_14DA=true; }
    }
    return true;
}
void RadarClass::QueueNextMovie() noexcept { unknown_14AC=4; unknown_14FC=25; }
bool RadarClass::IsRadarExisting() const noexcept { return IsAvailableNow; }
bool RadarClass::IsPlayerNames() const noexcept { return unknown_14B0==2 && unknown_14AC==1; }
bool RadarClass::IsPlayingMovie() const noexcept { return unknown_14B0==3 && unknown_14AC==1; }
void RadarClass::RedrawRadar(bool complete) noexcept {
    unknown_bool_14D9=true;
    unknown_bool_14DA=unknown_bool_14DA || complete;
}
