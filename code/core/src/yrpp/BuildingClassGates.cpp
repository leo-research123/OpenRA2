// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 building.cpp:
// Do_MISSION_OPEN, Is_Blocked_By_Occupier, Open_Gate, Is_Gate_Open.
// Calibrated to YR 0x0044E3A0 / 0x0044E440 / 0x00452540 / 0x004525F0.
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/VocClass.h"
#include "map_world.hpp"
#include "x87_integer.hpp"
#include <bit>

#if !defined(RA2_YRPP_GAME)
namespace {
bool gate_delay_finished(const RateTimer& timer) {
    if (!timer.Rate) return true;
    int remaining = timer.TimeLeft;
    if (timer.StartTime != -1) {
        const int elapsed = std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) - unsigned(timer.StartTime));
        remaining = elapsed >= remaining ? 0 : std::bit_cast<int>(unsigned(remaining) - unsigned(elapsed));
    }
    // The original tests (Rate - remaining) / Rate == 1.0. For int32
    // operands this is exactly remaining == 0; do not clamp paused timers.
    return remaining == 0;
}
}

bool BuildingClass::IsTraversable() const {
    return !Type->Gate || (GetCurrentMission() == Mission::Open && !UnloadTimer.State1 && UnloadTimer.State2);
}

bool BuildingClass::MakeTraversable() {
    if (!Type->Gate) return true;
    if (GetCurrentMission() != Mission::Open || UnloadTimer.AreStates10() || UnloadTimer.AreStates00()) {
        ForceMission(Mission::None);
        QueueMission(Mission::Open, false);
        NextMission();
    } else if (GetCurrentMission() == Mission::Open && UnloadTimer.AreStates01())
        return true;
    return false;
}

bool BuildingClass::IsBlockedByOccupier() const {
    const auto origin = GetMapCoords();
    bool blocked = false;
    for (auto* part = GetFoundationData(false); *part != CellStruct{0x7FFF, 0x7FFF}; ++part) {
        const CellStruct at{std::bit_cast<short>(static_cast<unsigned short>(unsigned(origin.X) + unsigned(part->X))),
                            std::bit_cast<short>(static_cast<unsigned short>(unsigned(origin.Y) + unsigned(part->Y)))};
        auto* object = MapClass::Instance.GetCellAt(at)->FirstObject;
        while (object == this)
            object = object->NextObject;
        // Any other object holds the gate open, including an enemy. Scan
        // every foundation cell in order, as the original helper does.
        if (object) blocked = true;
    }
    return blocked;
}

int BuildingClass::Mission_Open() {
    enum GateState : int { StartOpening = 0, Opening = 1, Open = 2, StartClosing = 3, Closing = 4, Closed = 5 };
    if (!Type->Gate) {
        QueueMission(Mission::Guard, false);
        return 1;
    }
    const auto restart_delay = [&] {
        GateTimer.Start(game::x87_scaled_quotient_to_integer(Type->GateCloseDelay, 900.0, 1));
    };
    switch (MissionStatus) {
        case GateState::StartOpening:
            if (UnloadTimer.AreStates01()) {
                restart_delay();
                MissionStatus = GateState::Open;
                break;
            }
            if (!UnloadTimer.AreStates11()) {
                if (UnloadTimer.AreStates10())
                    UnloadTimer.Update();
                else {
                    UnloadTimer.StartTimer11(Type->DeployTime);
                    VocClass::PlayAt(RulesClass::Instance->GateDown, Location);
                    RectangleStruct area;
                    GetRenderDimensions(&area);
                    TacticalClass::Instance->RegisterDirtyArea(area, false);
                }
            }
            MissionStatus = GateState::Opening;
            restart_delay();
            return 0;
        case GateState::Open:
            if (IsBlockedByOccupier())
                restart_delay();
            else if (gate_delay_finished(GateTimer))
                MissionStatus = GateState::StartClosing;
            break;
        case GateState::StartClosing:
            UnloadTimer.StartTimer10(Type->DeployTime);
            MissionStatus = GateState::Closing;
            VocClass::PlayAt(RulesClass::Instance->GateUp, Location);
            return 0;
        case GateState::Opening:
            if (UnloadTimer.AreStates01()) MissionStatus = GateState::Open;
            [[fallthrough]];
        case GateState::Closing:
            if (UnloadTimer.AreStates00()) {
                EnterIdleMode(false, true);
                MissionStatus = GateState::Closed;
                NeedsRedraw = true;
                game::map_object_changed();
            }
            if (UnloadTimer.AreStates11() || UnloadTimer.AreStates10() || UnloadTimer.AreStates01()) {
                const int stage =
                    game::x87_scaled_quotient_to_integer(UnloadTimer.PercentageDone(), double(Type->GateStages), 1);
                if (stage != static_cast<signed char>(GateStage)) {
                    GateStage = std::bit_cast<signed char>(static_cast<unsigned char>(stage));
                    NeedsRedraw = true;
                    game::map_object_changed();
                }
            }
            return 0;
        default:
            break;
    }
    const int delay = game::x87_scaled_quotient_to_integer(CurrentMissionControl()->Rate, 900.0, 1);
    const int jitter = ScenarioClass::Instance->Random.RandomRanged(0, 2);
    return std::bit_cast<int>(unsigned(delay) + unsigned(jitter));
}
#endif
