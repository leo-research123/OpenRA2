#pragma once
#include "yrpp/Timer.h"
#include "yrpp/GeneralDefinitions.h"
class ScenarioClass;
namespace game {
// Host coroutine storage for the original Main_Loop's end-of-frame wait.
// Game/Logic/Scenario retain gameplay state; this only yields to the host event loop.
struct GameLoopState {
    SysTimerClass frame_timer;
    bool started=false,focused=true;
    GameMode mode=GameMode::Skirmish;
    bool campaign_speed_setting=true;
    double elapsed_seconds=0;
    unsigned long long iterations=0;
};
struct GameLoopContext {
    GameLoopState& state;
    bool paused;
    void* context;
    bool (*input)(void*);
    bool (*render)(void*);
    bool (*logic)(void*);
    bool success=true;
    ScenarioClass* scenario=nullptr;
};
GameLoopContext* current_game_loop() noexcept;
bool advance_game_loop(GameLoopContext&,double seconds) noexcept;
}
