#include "yrpp/SessionClass.h"
#include "game_loop.hpp"
#include "clock.hpp"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/VoxClass.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
namespace game {
namespace {
thread_local GameLoopContext* active_loop = nullptr;
bool valid_loop(double seconds) noexcept {
    return !active_loop && std::isfinite(seconds) && seconds >= 0 &&
           (SessionClass::Instance.GameMode == GameMode::Campaign ||
            SessionClass::Instance.GameMode == GameMode::Skirmish) &&
           GameOptionsClass::Instance.GameSpeed >= 0 && GameOptionsClass::Instance.GameSpeed <= 6;
}
// Stop stepping and consume the result request once, as the outer loop does.
// The host confirms continuation before the world is replaced.
void run_original_iteration() {
    if (Game::MainLoop()) {
        active_loop->state.finished = true;
        active_loop->state.result = Game::TakeScenarioResult();
    }
}
bool run_iteration(GameLoopContext& context) {
    active_loop = &context;
    struct Restore {
        ~Restore() { active_loop = nullptr; }
    } restore;
    ClockReadScope clock_scope;
    const int frame = Unsorted::CurrentFrame;
    if (context.bind) {
        if (!context.bind(context.context, run_original_iteration)) return false;
    } else
        run_original_iteration();
    // Only a complete iteration advances the frame; paused and final ones do not.
    if (Unsorted::CurrentFrame != frame) ++context.state.iterations;
    return context.success;
}
unsigned long long budget_time(const GameLoopBudget* budget) noexcept {
    if (budget && budget->microseconds) return budget->microseconds(budget->context);
    return static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}
}
GameLoopContext* current_game_loop() noexcept { return active_loop; }
bool wait_for_house_result(HouseClass& house, HouseResultPhase phase) {
    auto* loop = current_game_loop();
    if (!loop) {
        // A direct, synchronous House update can settle an idle audio system.
        // An active device needs the MainLoop continuation and clock owner.
        if (VoxClass::IsSpeaking()) throw std::logic_error("House result speech requires the game loop");
        return true;
    }
    auto& s = loop->state;
    if (!s.result_house) {
        s.result_house = &house;
        s.result_phase = phase;
        s.result_speech_timer.Start(120);
    }
    if (s.result_house != &house || s.result_phase != phase)
        throw std::logic_error("Mismatched House result continuation");
    // 0x4F861C/0x4F8743 test speech before testing the 120-tick deadline.
    // IsSpeaking itself advances EVA; the pump services the other audio work.
    if (VoxClass::IsSpeaking() && s.result_speech_timer.GetTimeLeft()) {
        Game::CallBack();
        return false;
    }
    s.result_house = nullptr;
    s.result_phase = HouseResultPhase::none;
    s.result_speech_timer.Stop();
    return true;
}
void reset_game_loop_schedule(GameLoopState& s) noexcept {
    s.batch_fraction = 0;
    s.batch_reset = true;
    s.batch_steps = 0;
    s.batch_limited = false;
    s.frame_timer.Stop();
}
bool set_game_loop_speed(GameLoopState& s, int level) noexcept {
    if (active_loop || level < 1 || level > 9) return false;
    const int original = level <= 6 ? 7 - level : 0;
    if (s.speed_level != level || GameOptionsClass::Instance.GameSpeed != original) reset_game_loop_schedule(s);
    s.speed_level = level;
    GameOptionsClass::Instance.GameSpeed = original;
    return true;
}
int game_loop_target_rate(const GameLoopState& s) noexcept {
    return s.speed_level == 7 ? 120 : s.speed_level == 8 ? 240 : 0;
}
namespace {
// Both host entry points share validation, wall time, suspension and failure handling.
// Only the number/timing of complete Game::MainLoop iterations differs.
bool advance_loop(GameLoopContext& c, double seconds, bool batch, const GameLoopBudget* budget) noexcept {
    auto& s = c.state;
    if (!valid_loop(seconds) || (batch && (s.speed_level < 1 || s.speed_level > 9))) return false;
    if (batch) {
        s.batch_steps = 0;
        s.batch_limited = false;
    }
    const bool timed = !batch || s.speed_level <= 6 ||
                       (SessionClass::Instance.GameMode == GameMode::Campaign && !s.campaign_speed_setting);
    // Host time advances once per call, before any frame consumer, even while suspended.
    if (!advance_clock(seconds)) return false;
    s.elapsed_seconds += seconds;
    // Standalone hosts omit YR's focus wait at 0x0055D385. Explicit pause still
    // stops the loop; focus controls device input and presentation state.
    if (s.host_paused || Game::SpecialDialog == 1) {
        if (!timed) reset_game_loop_schedule(s);
        return true;
    }
    if (s.finished) return true;
    try {
        if (s.resuming_logic) {
            // Speech follows host wall time, independently of the speed's
            // frame budget. Do not accumulate catch-up work during the wait.
            reset_game_loop_schedule(s);
            return run_iteration(c);
        }
        if (timed) {
            if (s.frame_timer.InProgress()) return true;
            const auto before = s.iterations;
            const bool result = run_iteration(c);
            if (batch) s.batch_steps = static_cast<unsigned>(s.iterations - before);
            return result;
        }
        constexpr unsigned max_steps = 32;
        constexpr unsigned long long max_work_usec = 4000;
        const bool paused = c.scenario ? c.scenario->unknown_62C != 0 : c.paused;
        unsigned steps = 1;
        if (s.batch_reset || paused || seconds > 0.1) {
            // A new session, resume, speed change or long stall has no catch-up debt.
            s.batch_fraction = 0;
            s.batch_reset = false;
        } else if (const int rate = game_loop_target_rate(s)) {
            const double due = s.batch_fraction + seconds * rate;
            const auto whole = static_cast<unsigned>(std::floor(due + 1e-9));
            s.batch_fraction = std::max(0.0, due - whole);
            steps = std::min(whole, max_steps);
            s.batch_limited = whole > max_steps;
        } else
            steps = max_steps;
        const bool old_present = c.present, old_poll = c.poll_input;
        struct Restore {
            GameLoopContext& c;
            bool present, poll;
            ~Restore() {
                c.present = present;
                c.poll_input = poll;
            }
        } restore{c, old_present, old_poll};
        const auto start = budget_time(budget);
        for (unsigned i = 0; i < steps; ++i) {
            const bool out_of_time = i > 0 && budget_time(budget) - start >= max_work_usec;
            const bool now_paused = c.scenario ? c.scenario->unknown_62C != 0 : c.paused;
            // Reserve one final presentation iteration even when work overruns.
            // The budget is soft by that one complete iteration; no logic is skipped.
            c.present = old_present && (i + 1 == steps || out_of_time || now_paused);
            c.poll_input = old_poll && i == 0;
            const auto before = s.iterations;
            if (!run_iteration(c)) return false;
            s.batch_steps += static_cast<unsigned>(s.iterations - before);
            if (s.finished || s.resuming_logic) break;
            if (Game::SpecialDialog == 1) {
                s.batch_fraction = 0;
                s.batch_reset = true;
                break;
            }
            if (c.paused || now_paused) {
                s.batch_fraction = 0;
                s.batch_reset = true;
                break;
            }
            if (out_of_time) {
                s.batch_limited = i + 1 < steps;
                break;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}
}  // namespace
bool advance_game_loop(GameLoopContext& c, double seconds) noexcept { return advance_loop(c, seconds, false, nullptr); }
bool advance_game_loop_batch(GameLoopContext& c, double seconds, const GameLoopBudget* budget) noexcept {
    return advance_loop(c, seconds, true, budget);
}
}  // namespace game
