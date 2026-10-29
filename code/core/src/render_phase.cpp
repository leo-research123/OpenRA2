#include "render_phase.hpp"
#include "ui_cycle.hpp"
#include "tactical_timer_cycle.hpp"
#include "planning_cycle.hpp"
#include "yrpp/PlanningTokenClass.h"
#include "tactical_radial_cycle.hpp"
#include "tactical_placement_cycle.hpp"
#include "game_ui_runtime.hpp"
#include "yrpp/MessageListClass.h"
#include "aircraft_shadow_cycle.hpp"
#include "ebolt_lifetime.hpp"
#include "radbeam_state.hpp"
#include "shroud_cycle.hpp"
#include "map_view.hpp"
#include "map_world.hpp"
#include "map_world_internal.hpp"
#include "screen_effect.hpp"
#include "yrpp/SuperClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Unsorted.h"
namespace game {
namespace {
// State of the iteration in progress; the main loop is single threaded.
struct Iteration {
    unsigned rounds_run = 0, ui_rounds_run = 0;
    // Inside an iteration's render phase. Steps that advance state run only then; a host repaint that draws the
    // current state again (outside an iteration) must not advance it.
    MapViewHandle* view = nullptr;
    bool active = false;
    bool timers_run = false, radials_run = false, planning_run = false;
} iteration;
// Bind resources only: headless state updates have no drawing backend and
// cannot accidentally submit pixels. The same original state methods run here.
void ui_state_operation(MapViewHandle& view, bool initialize, bool fallback) noexcept {
    try {
        if (!view.ui_resources) view.ui_resources = std::make_unique<UiResources>();
        auto& resources = *view.ui_resources;
        (void)resources.load(view.scenario.PlayerSideIndex);
        MapDrawingContext drawing;
        MapDrawStatistics statistics;
        GameUiFrame frame{drawing, resources, statistics};
        frame.render_state = &view.ui_render;
        struct Arguments { GameUiFrame& frame; bool initialize, fallback; } arguments{frame, initialize, fallback};
        with_game_ui_state(frame, [](void* value) {
            auto& args = *static_cast<Arguments*>(value);
            auto& f = args.frame;
            if (args.initialize) initialize_ui_screen(f, true);
            if (!args.fallback) return;
            const unsigned rounds = (chrono_capture_pending() ? 1u : 0u) + 1u;
            while (iteration.ui_rounds_run < rounds) {
                const bool capture = chrono_capture_pending() && iteration.ui_rounds_run == 0;
                const bool transition = !capture && ChronoScreenEffect::Status != 0;
                if (!transition) {
                    if (iteration.rounds_run <= iteration.ui_rounds_run) {
                        render_phase_tactical_before();
                        render_phase_tactical_after();
                    }
                    advance_ui_radar(f, RadarClass::Instance, 1);
                    finish_ui_radar(f, RadarClass::Instance);
                    advance_ui_sidebar(SidebarClass::Instance);
                    finish_ui_sidebar(SidebarClass::Instance);
                    finish_ui_power(PowerClass::Instance, 1);
                }
                advance_ui_messages(f, MessageListClass::Instance);
                if (CCToolTip::Instance) advance_ui_tooltip(*CCToolTip::Instance);
                ++iteration.ui_rounds_run;
            }
        }, &arguments);
    } catch (...) { /* Resource failure cannot cross the iteration boundary. */ }
}
}
unsigned render_phase_tactical_rounds() noexcept {
    return (chrono_capture_pending() ? 1u : 0u) + (ChronoScreenEffect::Status == 0 ? 1u : 0u);
}
void render_phase_tactical_before() noexcept {
    ++iteration.rounds_run;
    iteration.timers_run = false;
    iteration.radials_run = false;
    iteration.planning_run = false;
    if (iteration.active) { ebolt_follow_owners(); radbeam_before_draw(); advance_shroud_view(); }
    auto* world = current_map_world();
    if (!world || !world->impl->loaded || !TacticalClass::Instance) return;
    // Original Render 0x6D3D10 and the draw calls of the bodies: the table of selectable objects restarts
    // with the buildings in view, then the mobile bodies register in draw order. The registration is
    // independent of whether anything is drawn, so every round performs it.
    register_world_selectables(*world);
    // Original DrawObjects: the shadow-height cycle of the aircraft the draw reaches (aircraft_shadow_cycle.hpp).
    run_aircraft_shadow_cycle();
    if (iteration.active && iteration.view) advance_tactical_placement(*iteration.view);
}
void render_phase_planning_before() noexcept {
    if (iteration.active && !iteration.planning_run) {
        if (PlanningNodeClass::PlanningModeActive) advance_planning_view();
        iteration.planning_run = true;
    }
}
void render_phase_radials_before() noexcept {
    if (iteration.active && !iteration.radials_run) { advance_tactical_radials(); iteration.radials_run = true; }
}
void render_phase_timers_before() noexcept {
    if (iteration.active && !iteration.timers_run) {
        advance_tactical_timers();
        iteration.timers_run = true;
    }
}
void render_phase_tactical_after() noexcept {
    render_phase_planning_before();
    render_phase_radials_before();
    render_phase_timers_before();
    if (iteration.active) { ebolt_advance(); radbeam_after_draw(); }
}
void render_phase_before_draw(MapViewHandle& view, bool presenting) noexcept {
    iteration = {};
    iteration.active = true;
    iteration.view = &view;
    (void)presenting;
    ui_state_operation(view, true, false);
}
bool render_phase_ui_active() noexcept { return iteration.active; }
void render_phase_ui_round_complete() noexcept { if (iteration.active) ++iteration.ui_rounds_run; }
void render_phase_after_draw(MapViewHandle& view, bool) noexcept {
    ui_state_operation(view, false, true);
    // Rounds that were not extracted (headless iteration, or an extraction that did not reach the view)
    // still advance the simulation-side state of the round.
    for (const unsigned rounds = render_phase_tactical_rounds(); iteration.rounds_run < rounds;) {
        render_phase_tactical_before();
        render_phase_tactical_after();
    }
    iteration.active = false;
}
}
