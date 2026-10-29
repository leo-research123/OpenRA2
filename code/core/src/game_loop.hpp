#pragma once
#include "yrpp/Timer.h"
#include "yrpp/GeneralDefinitions.h"
class ScenarioClass;
class HouseClass;
namespace game {
enum class HouseResultPhase { none, victory, defeat };
// Host coroutine storage for the original Main_Loop's end-of-frame wait.
// Game/Logic/Scenario retain gameplay state; this only yields to the host event loop.
struct GameLoopState {
    SysTimerClass frame_timer;
    bool campaign_speed_setting = true;
    // Game::MainLoop returned true: this standalone session stops iterating.
    // The original outer loop at 0x0048CCC0 also invokes an end-game handler.
    bool finished = false;
    int result = 0;  // Consumed original result, retained while the host asks what to do next.
    double elapsed_seconds = 0;
    unsigned long long iterations = 0;
    int speed_level = 5;
    bool host_paused = false, batch_reset = true;
    double batch_fraction = 0;
    unsigned batch_steps = 0;
    bool batch_limited = false;
    // Execution cursors only: the original House retains all gameplay state.
    // A speech wait suspends the current House, Logic pass and MainLoop frame.
    HouseClass* result_house = nullptr;
    HouseResultPhase result_phase = HouseResultPhase::none;
    SysTimerClass result_speech_timer;
    int logic_house_index = -1;
    bool resuming_logic = false;
};
// Host services of one Main_Loop iteration. Game::MainLoop runs the original phases itself;
// these hooks only bind what the core cannot own: the device sampling of the input phase,
// the drawing target of the render phase and the host's per-iteration bookkeeping.
struct GameLoopContext {
    GameLoopState& state;
    bool paused;
    void* context;
    // Input phase (GScreenClass::GetInputAndUpdate, then Game::KeyboardProcess). The host
    // binds its device state, then runs the core operation or replaces it. Null: the
    // input phase is skipped.
    bool (*input)(void*, void (*operation)());
    // Render phase (GScreenClass::Render). The host binds its drawing target and runs
    // the operation, or performs the headless presentation update instead. Null: the
    // operation runs unbound.
    bool (*render)(void*, void (*operation)());
    // After the last original phase of an iteration that ran the world, including the
    // iteration that ends the game. Null: nothing to do.
    bool (*frame_end)(void*);
    bool success = true;
    // The bound Scenario. Null runs the scheduler and the hooks only, with none of the
    // original world phases (host scheduler tests).
    ScenarioClass* scenario = nullptr;
    // Intermediate iterations keep their presentation state transitions but
    // do not emit drawing packets. Device polling happens once per batch.
    bool present = true, poll_input = true;
    // Binds the host's per-operation state (scenario services, resources, tooltips) around
    // one iteration of the main loop; called only for iterations that run. The operation
    // is Game::MainLoop. Null: runs unbound.
    bool (*bind)(void*, void (*operation)()) = nullptr;
    // Native presentation entry supplied by the owner of the render hook. The simulation scheduler
    // invokes it at the original screen phase without including presentation implementation headers.
    void (*render_operation)() = nullptr;
};
GameLoopContext* current_game_loop() noexcept;
// Original House result barrier, yielded to the serialized host clock/pump.
bool wait_for_house_result(HouseClass&, HouseResultPhase);
// Single-step and bounded-batch scheduling share one driver. Each actual step
// calls Game::MainLoop; no gameplay phases are duplicated in the scheduler.
// Host hook exceptions are caught here and reported as false.
bool advance_game_loop(GameLoopContext&, double seconds) noexcept;
bool set_game_loop_speed(GameLoopState&, int level) noexcept;
void reset_game_loop_schedule(GameLoopState&) noexcept;
int game_loop_target_rate(const GameLoopState&) noexcept;
// Scheduling budget only; never a replacement for the game's clock. Tests
// may supply a deterministic source without changing gameplay time.
struct GameLoopBudget {
    void* context = nullptr;
    unsigned long long (*microseconds)(void*) noexcept = nullptr;
};
bool advance_game_loop_batch(GameLoopContext&, double seconds, const GameLoopBudget* budget = nullptr) noexcept;
}
