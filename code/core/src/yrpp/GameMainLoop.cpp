// EA REDALERT/CONQUER.CPP Main_Loop, fixed revision
// f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts Inc.
// GPL-3.0-or-later plus third_party/ea/LICENSE.TXT terms.
// YR 0x0055D360 / 0x0055E160: campaign/skirmish branch. The original phases run
// here in their original order. The host supplies only the device binding of the
// input phase, the drawing target of the render phase and its own frame-end
// bookkeeping (game_loop.hpp). The host scheduler (game_loop.cpp) replaces the
// single-player Sync_Delay (0x55E160) wait with a frame timer and stops when this
// returns true. It does not reproduce the outer game/menu loop at 0x0048CCC0.
// Sync_Delay's own GScreen input and render calls belong to the network wait.
//
// Left out because nothing in a standalone Campaign/Skirmish session reaches it:
// the Windows message pump (0x5D4D50) and focus wait (0x55D385), the network
// frame work (0x542520, the connection-state branches), record/playback
// (RecordingFlag), the debug statistics overlay (0x55F1E0, only drawn when
// EnableMPDebug is set), the autosave (0x67CEF0, save/load is not part of the
// core), the lap statistics at 0xA8B55C/0xA8B560/0xA8B564, and the in-game music
// kick-off at 0x55D3F8 (the audio session owns the theme).
#include "yrpp/Unsorted.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/GScreenClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/FoggedObjectClass.h"
#include "yrpp/AbstractClass.h"
#include "game_loop.hpp"
#include "api/runtime_diagnostics.hpp"
#include <algorithm>
#include <bit>

namespace {
// 0x55D8AB: the key is taken from the gadget list or the keyboard queue, the
// pointer from the mouse driver, and Update receives both.
void input_phase() {
    DWORD key = 0;
    int x = 0, y = 0;
    GScreenClass::Instance.GetInputAndUpdate(key, x, y);
    Game::KeyboardProcess(key);
}
void render_phase() { GScreenClass::Instance.Render(); }
void no_phase() {}
// 0x55DCAB..0x55DD96. 0xA8EC08/0C and 0xA8EC00/04 are the force-select and
// force-fire key pairs of the options; 0x82A030 is the step in pixels and
// 0x82A034 the force-select factor.
void keyboard_scroll_phase() {
    constexpr int step = 21;
    constexpr float select_factor = 2.5f;
    int distance = step;
    const auto* keyboard = InputManagerClass::Instance;
    if (keyboard && keyboard->IsForceSelectKeyPressed())
        distance = static_cast<int>(static_cast<double>(distance) * select_factor);
    else if (keyboard && keyboard->IsForceFireKeyPressed())
        distance = std::max(MapClass::Instance.MaxWidth, MapClass::Instance.MaxHeight) << 8;
    const auto flags = Game::KeyboardScrollFlags;
    auto& display = MouseClass::Instance;
    if (flags & 0x100) display.ScrollMap(6, &distance, true);
    if (flags & 0x1000) display.ScrollMap(2, &distance, true);
    if (flags & 0x1) display.ScrollMap(0, &distance, true);
    if (flags & 0x10) display.ScrollMap(4, &distance, true);
}
// 0x55D7DA: the pause timer ends a timed pause by releasing one logic pause.
void pause_phase(ScenarioClass& scenario) {
    if (scenario.IsGamePaused && !scenario.PauseTimer.HasTimeLeft()) {
        scenario.IsGamePaused = false;
        ScenarioClass::ResumeLogic();
    }
}
struct ScenarioBinding {
    ScenarioClass* previous;
    explicit ScenarioBinding(ScenarioClass* scenario) : previous(ScenarioClass::Instance) {
        if (scenario) ScenarioClass::Instance = scenario;
    }
    ~ScenarioBinding() { ScenarioClass::Instance = previous; }
};
}  // namespace

bool YRPP_FASTCALL Game::MainLoop() {
    // 0x0055D36B: an inactive game has nothing to run, and the caller's loop ends.
    if (!Game::IsActive) return true;
    auto* c = game::current_game_loop();
    if (!c) return true;
    auto& s = c->state;
    if (SessionClass::Instance.GameMode == GameMode::Campaign && !s.campaign_speed_setting)
        GameOptionsClass::Instance.GameSpeed = 2;
    if (!s.resuming_logic) s.frame_timer.Start(GameOptionsClass::Instance.GameSpeed);
    // A failed host hook is reported through the context; the game itself goes on.
    const auto fail = [&] {
        c->success = false;
        return false;
    };
    const bool world = c->scenario != nullptr;
    // Without a Scenario only the scheduler and the host hooks run.
    const auto render = [&]() -> bool {
        game::runtime_crash_checkpoint(game::RuntimePhase::render, Unsorted::CurrentFrame);
        const auto operation = world ? (c->render_operation ? c->render_operation : render_phase) : no_phase;
        if (c->render) return c->render(c->context, operation);
        if (world) operation();
        return true;
    };
    ScenarioBinding binding(c->scenario);
    if (s.resuming_logic) {
        if (world) {
            pause_phase(*ScenarioClass::Instance);
            c->paused = ScenarioClass::Instance->unknown_62C != 0;
            if (c->paused) {
                Game::CallBack();
                return false;
            }
        }
        goto resume_logic;
    }
    if (world) {
        pause_phase(*ScenarioClass::Instance);
        c->paused = ScenarioClass::Instance->unknown_62C != 0;
    }
    // 0x55D826: a paused iteration keeps the service pump, the screen effect and
    // the view animation running, renders, waits and returns without advancing.
    if (c->paused) {
        if (world) {
            Game::CallBack();
            ChronoScreenEffect::Update();
            if (TacticalClass::Instance) TacticalClass::Instance->Update();
        }
        if (!render()) return fail();
        // 0x0055D86D
        return !Game::IsActive;
    }
    // 0x55D878: input, fogged-object upkeep and the render share one gate. The
    // recording flag and the window focus of the original gate do not apply: a
    // session never records, and this host keeps simulating and drawing unfocused.
    if (Game::SpecialDialog == 0) {
        game::runtime_crash_checkpoint(game::RuntimePhase::input, Unsorted::CurrentFrame);
        if (c->input && !c->input(c->context, world ? input_phase : no_phase)) return fail();
        if (world) FoggedObjectClass::UpdateAll();
        // 0x55D8F2 renders before 0x55DC9E Logic.Update.
        if (!render()) return fail();
    }
    // 0x55D903: the drag-select abort lives for one input phase.
    Unsorted::DragSelectAborted = false;
    if (world) {
        // 0x55DBC8: one incremental Ground pass after rendering, before logic.
        // This belongs to the game iteration, never to each host repaint.
        MapClass::ObjectsInLayers[static_cast<int>(Layer::Ground)].Sort();
        // 0x55DC9E. The one-time autosave at 0x55DBE0 is not part of the core.
        game::runtime_crash_checkpoint(game::RuntimePhase::logic, Unsorted::CurrentFrame);
    resume_logic:
        LogicClass::Instance.Update();
        if (s.result_house) {
            s.resuming_logic = true;
            return false;
        }
        s.resuming_logic = false;
        // 0x55DCAB: held scroll arrows move the view after logic.
        keyboard_scroll_phase();
        // 0x55DD9B..0x55DE4A: planning events, messages, then the queued events,
        // the planning events they produced and the service pump.
        Game::PlanningManager_FlushEvents();
        MessageListClass::Instance.Manage();
        game::runtime_crash_checkpoint(game::RuntimePhase::events, Unsorted::CurrentFrame);
        EventClass::ProcessLocalQueues();
        Game::PlanningManager_FlushEvents();
        Game::CallBack();
    }
    // 0x55DE4F: a win, loss, restart or abort request leaves before the frame
    // advances and ends the caller's loop. The restart and abort requests
    // (0xA83D48, 0x8B41C0) have no counterpart here.
    const bool ended = Game::PlayerWins || Game::PlayerLoses;
    if (!ended) {
        // 0x55DE81, AFTER all frame consumers, and BEFORE the final wait.
        Unsorted::CurrentFrame = std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame) + 1u);
        // 0x55DE87: a timed input lock expires on a later frame.
        if (world && ScenarioClass::InputUnlockFrame && ScenarioClass::InputUnlockFrame < Unsorted::CurrentFrame) {
            ScenarioClass::UnlockInput();
            ScenarioClass::InputUnlockFrame = 0;
        }
    }
    if (world && !ended) {
        // 0x55DE9F: with Sync_Delay in the host scheduler, the frame ends here.
        // Objects retired during this frame are released only now, after every
        // consumer of the frame (logic, events, the pump) has finished with them.
        AbstractClass::RemoveAllInactive();
        Game::PlanningManager_Update();
    }
    if (c->frame_end && !c->frame_end(c->context)) return fail();
    // 0x0055DED2
    if (ended) return true;
    game::runtime_crash_checkpoint(game::RuntimePhase::idle, Unsorted::CurrentFrame);
    // 0x0055DEA9
    return !Game::IsActive;
}
