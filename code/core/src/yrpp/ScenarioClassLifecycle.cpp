/*
 * Scenario state initialization adapts EA REDALERT/SCENARIO.CPP,
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
 * Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with the additional
 * terms in third_party/ea/LICENSE.TXT. YR defaults and operations calibrated
 * to 6832C0/683610; normal destruction is inlined at 6BEAC3..6BEB83.
 */
#include "yrpp/YRPPCore.h"
#include "yrpp/ScenarioClass.h"
#include "scenario_runtime.hpp"
#include <bit>
#include <cstring>
#include <cwchar>
#include <stdexcept>

ScenarioClass::ScenarioClass() : SpecialFlags{}, Random(0), ElapsedTimer(0), PauseTimer(0),
        GlobalVariables{}, LocalVariables{} {
    Reset();
    unknown_3598 = 0;
    IsRandom = false;
    // Original SpecialClass initialization preserves its high thirteen bits.
    const DWORD flags = 0x8088u;
    static_assert(sizeof(SpecialFlags) == sizeof(flags));
    std::memcpy(&SpecialFlags, &flags, sizeof(flags));
    TechLevel = 1;
    CampaignIndex = -1;
    NextScenario[0] = AltNextScenario[0] = '\0';
    ClearWaypoints();
    Name[0] = UINameLoaded[0] = Briefing[0] = L'\0';
    FileName[0] = BriefingCSF[0] = UIName[0] = '\0';
    View1 = View2 = View3 = View4 = CellStruct{0, 0};
    // Fresh portable objects initialize variable representations, instead of
    // inspecting indeterminate allocator bytes as the original constructor did.
}

ScenarioClass::~ScenarioClass() = default;

void ScenarioClass::Reset() {
    HomeCell = AltHomeCell = 699;
    UniqueID = 1000000;
    Difficulty1 = Difficulty2 = 1;
    ElapsedTimer.Start(); ElapsedTimer.Pause();
    PauseTimer.Start(0); PauseTimer.Pause();
    unknown_62C = 0;
    IsGamePaused = false;
    MissionTimer.Start(0); MissionTimer.Pause();
    MissionTimerTextCSF = const_cast<wchar_t*>(L"");
    std::memset(MissionTimerText, 0, sizeof(MissionTimerText));
    ShroudRegrowTimer.Start(0); FogTimer.Start(0); IceTimer.Start(0);
    unknown_timer_123c.Start(0); AmbientTimer.Start(0);
    Theater = TheaterType::None;
    Intro = Brief = Win = Lose = Action = PostScore = PreMapSelect = -1;
    ThemeIndex = -1;
    HumanPlayerHouseTypeIndex = 0;
    CarryOverMoney = 0.0;
    CarryOverCap = Percent = 0;
    unknown_34A0 = 0;
    FreeRadar = TrainCrate = false;
    TiberiumGrowthEnabled = VeinGrowthEnabled = IceGrowthEnabled = true;
    BridgeDestroyed = VariablesChanged = AmbientChanged = EndOfGame = false;
    TimerInherit = SkipScore = OneTimeOnly = SkipMapSelect = TruckCrate = FillSilos = false;
    TiberiumDeathToVisceroid = true;
    IgnoreGlobalAITriggers = false;
    PlayerSideIndex = 0;
    MultiplayerOnly = PickedUpAnyCrate = false;
    unknown_timer_34C0.Start(0);
    StartingDropships = 0;
    AmbientOriginal = AmbientCurrent = AmbientTarget = 100;
    NormalLighting = {{100, 100, 100}, 50, 8};
    IonAmbient = 87;
    IonLighting = {{30, 40, 75}, 0, 0};
    NukeAmbient = 200;
    NukeLighting = {{175, 150, 125}, 100, 100};
    NukeAmbientChangeRate = 1;
    DominatorAmbient = 150;
    DominatorLighting = {{85, 20, 30}, 0, 0};
    DominatorAmbientChangeRate = 1;
    InitTime = 10000;
    Stage = 0;
    UserInputLocked = false;
    ParTimeEasy = ParTimeMedium = ParTimeDifficult = 3600;
    UnderParTitle[0] = UnderParMessage[0] = OverParTitle[0] = OverParMessage[0] = '\0';
    LSLoadMessage[0] = LSBrief[0] = '\0';
    LS640BriefLocX = LS640BriefLocY = LS800BriefLocX = LS800BriefLocY = 0;
    LS640BkgdName[0] = LS800BkgdName[0] = LS800BkgdPal[0] = '\0';
    TeamsPresent = false;
    NumCoopHumanStartSpots = 4;
    AllowableUnits.Clear();
    AllowableUnitMaximums.Clear();
    DropshipUnitCounts.Clear();
    const auto& runtime = game::scenario_runtime();
    if (runtime.session_mode(runtime.context) == 5) {
        // Target writes Instance->Name, rather than this->Name. Preserve that
        // distinction; a session must publish its Scenario before this reset.
        if (!Instance) throw std::logic_error("Skirmish Scenario reset requires the active Scenario instance");
        const wchar_t* text = game::scenario_text("GUI:SkirmishGame", 522);
        unsigned i = 0;
        for (; i < 44 && text[i]; ++i) Instance->Name[i] = text[i];
        Instance->Name[i] = L'\0';
    }
}

void ScenarioClass::ClearWaypoints() {
    for (auto& cell : Waypoints) cell = {0, 0};
}
int ScenarioClass::CreateUniqueID() {
    UniqueID = std::bit_cast<std::int32_t>(static_cast<DWORD>(UniqueID) + 1u);
    return UniqueID;
}
char ScenarioClass::SetGlobal(int index, char value) {
    if (static_cast<unsigned>(index) >= 50u) return 0;
    char previous = GlobalVariables[index].Value;
    if (previous != value) {
        GlobalVariables[index].Value = value;
        VariablesChanged = true;
        const auto& runtime = game::scenario_runtime();
        runtime.variable_changed(runtime.context, true, index);
    }
    return previous;
}
char ScenarioClass::SetLocal(int index, char value) {
    if (static_cast<unsigned>(index) >= 100u) return 0;
    char previous = LocalVariables[index].Value;
    if (previous != value) {
        LocalVariables[index].Value = value;
        VariablesChanged = true;
        const auto& runtime = game::scenario_runtime();
        runtime.variable_changed(runtime.context, false, index);
    }
    return previous;
}
bool ScenarioClass::GetGlobal(int index, char* value) {
    if (static_cast<unsigned>(index) >= 50u) return false;
    *value = GlobalVariables[index].Value;
    return true;
}
bool ScenarioClass::GetLocal(int index, char* value) {
    if (static_cast<unsigned>(index) >= 100u) return false;
    *value = LocalVariables[index].Value;
    return true;
}
