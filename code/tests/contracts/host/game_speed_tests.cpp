#include <gtest/gtest.h>
#include "support/scoped_state.hpp"
#include "game_loop.hpp"
#include "clock.hpp"
#include "core_call_scope.hpp"
#include "yrpp/SessionClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Unsorted.h"
#include <cfenv>
#include <cmath>
#include <limits>
#include <vector>

namespace {
struct Harness {
    ra2::test::ScopedValue<GameMode> mode_scope{SessionClass::Instance.GameMode, GameMode::Skirmish};
    // The scheduler drives Game::MainLoop, which only runs an active game.
    ra2::test::ScopedValue<bool> active_scope{Game::IsActive, true};
    ra2::test::ScopedValue<bool> focus_scope{Game::IsFocused, true};
    int previous_speed = GameOptionsClass::Instance.GameSpeed;
    int previous_frame = Unsorted::CurrentFrame;
    game::GameLoopState state;
    double milliseconds = 0;
    unsigned clock_advances = 0, polls = 0, presents = 0;
    unsigned long long work = 0, step_work = 0;
    std::vector<int> phases;
    game::GameLoopContext loop{state,
                               false,
                               this,
                               [](void* p, void (*)()) {
                                   auto& h = *static_cast<Harness*>(p);
                                   h.phases.push_back(Unsorted::CurrentFrame * 10 + 1);
                                   if (game::current_game_loop()->poll_input) ++h.polls;
                                   return true;
                               },
                               [](void* p, void (*)()) {
                                   auto& h = *static_cast<Harness*>(p);
                                   h.phases.push_back(Unsorted::CurrentFrame * 10 + 2);
                                   if (game::current_game_loop()->present) ++h.presents;
                                   return true;
                               },
                               [](void* p) {
                                   auto& h = *static_cast<Harness*>(p);
                                   // The frame-end hook follows the frame advance of its iteration.
                                   h.phases.push_back((Unsorted::CurrentFrame - 1) * 10 + 3);
                                   h.work += h.step_work;
                                   return true;
                               }};
    game::GameLoopBudget budget{this, [](void* p) noexcept { return static_cast<Harness*>(p)->work; }};
    explicit Harness(int level = 8) {
        Unsorted::CurrentFrame = 0;
        EXPECT_TRUE(game::set_game_loop_speed(state, level));
    }
    ~Harness() {
        GameOptionsClass::Instance.GameSpeed = previous_speed;
        Unsorted::CurrentFrame = previous_frame;
    }
    bool advance(double seconds, bool batch = true) {
        struct Call {
            Harness& h;
            double seconds;
            bool batch;
            bool result = false;
        } call{*this, seconds, batch};
        game::ClockServices clock{
            this, [](void* p) noexcept { return static_cast<std::uint32_t>(static_cast<Harness*>(p)->milliseconds); },
            [](void* p, double seconds) noexcept {
                auto& h = *static_cast<Harness*>(p);
                ++h.clock_advances;
                h.milliseconds = std::fmod(h.milliseconds + std::fmod(seconds, 4294967.296) * 1000, 4294967296.0);
                return true;
            }};
        return game::with_clock(
                   clock,
                   [](void* p) {
                       auto& c = *static_cast<Call*>(p);
                       c.result = c.batch ? game::advance_game_loop_batch(c.h.loop, c.seconds, &c.h.budget)
                                          : game::advance_game_loop(c.h.loop, c.seconds);
                   },
                   &call) &&
               call.result;
    }
};
}

TEST(GameSpeed, SingleEntryStillCapsHighSpeedAtOneIteration) {
    for (bool batch : {false, true}) {
        Harness h(8);
        ASSERT_TRUE(h.advance(0, batch));
        ASSERT_TRUE(h.advance(1.0 / 60, batch));
        EXPECT_EQ(h.state.iterations, batch ? 5u : 2u);
        EXPECT_EQ(h.polls, 2u);
        EXPECT_EQ(h.presents, 2u);
        EXPECT_EQ(h.clock_advances, 2u);
    }
}

TEST(GameSpeed, BothEntriesContainHookFailuresAndRestoreTheActiveContext) {
    for (bool batch : {false, true})
        for (bool throws : {false, true}) {
            Harness h(8);
            h.loop.context = &throws;
            h.loop.input = [](void* p, void (*)()) -> bool {
                if (*static_cast<bool*>(p)) throw 1;
                return false;
            };
            EXPECT_FALSE(h.advance(0, batch));
            EXPECT_EQ(Unsorted::CurrentFrame, 0);
            EXPECT_FALSE(h.state.finished);
            EXPECT_EQ(game::current_game_loop(), nullptr);
            EXPECT_TRUE(h.loop.present);
            EXPECT_TRUE(h.loop.poll_input);
        }
}

TEST(GameSpeed, HostBindingWrapsOnlyIterationsThatRun) {
    Harness h;
    h.loop.bind = [](void* p, void (*iteration)()) {
        auto& h = *static_cast<Harness*>(p);
        h.phases.push_back(Unsorted::CurrentFrame * 10 + 4);
        iteration();
        return true;
    };
    ASSERT_TRUE(h.advance(0));
    EXPECT_EQ(h.phases, (std::vector<int>{4, 1, 2, 3}));
    EXPECT_EQ(Unsorted::CurrentFrame, 1);
    h.phases.clear();
    ASSERT_TRUE(h.advance(0));
    EXPECT_TRUE(h.phases.empty()) << "The frame timer is still running: nothing is bound or run";
    h.loop.bind = [](void*, void (*)()) { return false; };
    EXPECT_FALSE(h.advance(1.0 / 240));
    EXPECT_EQ(Unsorted::CurrentFrame, 1) << "A failed binding must not run the iteration";
}

TEST(GameSpeed, HostBoundaryRestoresFloatingPointEnvironmentOnSuccessAndFailure) {
    std::fenv_t saved;
    ASSERT_EQ(std::fegetenv(&saved), 0);
    const auto restore = ra2::test::scope_exit([&] { std::fesetenv(&saved); });
    for (int mode : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD})
        for (bool fail : {false, true}) {
            ASSERT_EQ(std::fesetround(mode), 0);
            try {
                game::CoreCallScope host;
                std::fesetround(FE_TOWARDZERO);
                EXPECT_EQ(std::fegetround(), FE_TOWARDZERO);
                if (fail) throw 1;
            } catch (int) {
            }
            EXPECT_EQ(std::fegetround(), mode);
        }
}

TEST(GameSpeed, TargetRatesDoNotDependOnHostRefreshOrPresentation) {
    for (int hz : {30, 60, 144})
        for (int level : {7, 8})
            for (bool present : {false, true}) {
                Harness h(level);
                h.loop.present = present;
                ASSERT_TRUE(h.advance(0));
                for (int i = 0; i < 2 * hz; ++i)
                    ASSERT_TRUE(h.advance(1.0 / hz));
                const int expected = 1 + (level == 7 ? 240 : 480);
                EXPECT_EQ(Unsorted::CurrentFrame, expected) << hz << " / " << level;
                EXPECT_EQ(h.state.iterations, static_cast<unsigned>(expected));
                EXPECT_EQ(h.clock_advances, 1u + 2 * hz);
                EXPECT_NEAR(h.milliseconds, 2000, 1e-6);
                EXPECT_LE(h.presents, present ? 1u + 2 * hz : 0u);
                ASSERT_EQ(h.phases.size(), 3u * expected);
                for (int i = 0; i < expected; ++i)
                    for (int phase = 1; phase <= 3; ++phase)
                        EXPECT_EQ(h.phases[3 * i + phase - 1], 10 * i + phase);
            }
}

TEST(GameSpeed, BatchesKeepFrameAndWallClocksSeparateAndPollDeviceOnce) {
    Harness h;
    ASSERT_TRUE(h.advance(0));
    h.presents = h.polls = 0;
    CDTimerClass frames(4);
    ASSERT_TRUE(h.advance(1.0 / 60));
    EXPECT_EQ(h.state.batch_steps, 4u);
    EXPECT_TRUE(frames.Completed());
    EXPECT_EQ(h.presents, 1u);
    EXPECT_EQ(h.polls, 1u);
    EXPECT_NEAR(h.milliseconds, 1000.0 / 60, 1e-8);
    EXPECT_EQ(h.clock_advances, 2u);
    ASSERT_TRUE(h.advance(0));
    EXPECT_EQ(h.state.batch_steps, 0u);
}

TEST(GameSpeed, UnlimitedAndSlowWorkAreBoundedWithoutDebt) {
    Harness h(9);
    ASSERT_TRUE(h.advance(0));
    h.presents = 0;
    ASSERT_TRUE(h.advance(1.0 / 60));
    EXPECT_EQ(h.state.batch_steps, 32u);
    EXPECT_EQ(h.presents, 1u);
    ASSERT_TRUE(game::set_game_loop_speed(h.state, 8));
    ASSERT_TRUE(h.advance(0));
    h.step_work = 5000;
    h.presents = 0;
    ASSERT_TRUE(h.advance(1.0 / 30));
    EXPECT_EQ(h.state.batch_steps, 2u);
    EXPECT_TRUE(h.state.batch_limited);
    EXPECT_EQ(h.presents, 1u);
    h.step_work = 0;
    ASSERT_TRUE(h.advance(0));
    EXPECT_EQ(h.state.batch_steps, 0u);
    ASSERT_TRUE(h.advance(1.0 / 60));
    EXPECT_EQ(h.state.batch_steps, 4u);
    ASSERT_TRUE(h.advance(10));
    EXPECT_EQ(h.state.batch_steps, 1u);
    ASSERT_TRUE(h.advance(0));
    EXPECT_EQ(h.state.batch_steps, 0u);
}

TEST(GameSpeed, PauseSpeedChangesAndFailuresCannotCreateCatchUpBursts) {
    Harness h;
    ASSERT_TRUE(h.advance(0));
    ASSERT_TRUE(h.advance(0.002));
    h.state.host_paused = true;
    const auto before = h.state.iterations;
    ASSERT_TRUE(h.advance(10));
    EXPECT_EQ(h.state.iterations, before);
    h.state.host_paused = false;
    ASSERT_TRUE(h.advance(10));
    EXPECT_EQ(h.state.batch_steps, 1u);
    Game::IsFocused = false;
    ASSERT_TRUE(h.advance(10));
    EXPECT_EQ(h.state.batch_steps, 1u);
    Game::IsFocused = true;
    ASSERT_TRUE(h.advance(0));
    EXPECT_EQ(h.state.batch_steps, 0u);
    h.loop.paused = true;
    ASSERT_TRUE(h.advance(1.0 / 60));
    EXPECT_EQ(h.state.batch_steps, 0u);
    h.loop.paused = false;
    ASSERT_TRUE(h.advance(1.0 / 60));
    EXPECT_EQ(h.state.batch_steps, 1u);
    ASSERT_TRUE(h.advance(0.002));
    ASSERT_TRUE(game::set_game_loop_speed(h.state, 7));
    ASSERT_TRUE(h.advance(1.0 / 60));
    EXPECT_EQ(h.state.batch_steps, 1u);
    const auto calls = h.clock_advances;
    EXPECT_FALSE(h.advance(-1));
    EXPECT_FALSE(h.advance(std::numeric_limits<double>::infinity()));
    EXPECT_EQ(h.clock_advances, calls);
    h.loop.frame_end = [](void*) -> bool { throw 1; };
    EXPECT_FALSE(h.advance(1.0 / 60));
    EXPECT_EQ(game::current_game_loop(), nullptr);
    EXPECT_TRUE(h.loop.present);
    EXPECT_TRUE(h.loop.poll_input);
    h.loop.frame_end = nullptr;
    h.loop.success = true;
    EXPECT_TRUE(h.advance(1.0 / 60));
}

TEST(GameSpeed, BackgroundUsesTheSameScheduleAndStillHonorsHostPause) {
    for (auto mode : {GameMode::Campaign, GameMode::Skirmish})
        for (int level = 1; level <= 9; ++level) {
            std::vector<unsigned long long> foreground;
            for (bool focused : {true, false}) {
                Harness h(level);
                SessionClass::Instance.GameMode = mode;
                Game::IsFocused = focused;
                for (int i = 0; i < 120; ++i) {
                    ASSERT_TRUE(h.advance(1.0 / 60));
                    if (focused)
                        foreground.push_back(h.state.iterations);
                    else
                        EXPECT_EQ(h.state.iterations, foreground[i]) << level << " / " << i;
                }
                EXPECT_GT(h.state.iterations, 0u);
                const auto before = h.state.iterations;
                h.state.host_paused = true;
                ASSERT_TRUE(h.advance(10));
                EXPECT_EQ(h.state.iterations, before);
                Game::IsFocused = !focused;
                ASSERT_TRUE(h.advance(10));
                EXPECT_EQ(h.state.iterations, before);
                h.state.host_paused = false;
                ASSERT_TRUE(h.advance(1));
                EXPECT_EQ(h.state.iterations, before + 1);
            }
        }
}
