#pragma once
// The render phase of one main-loop iteration, as a simulation-side step (plan 4.7). The original
// advances some state while it draws: the radar animation and its events, the tooltip poll, the
// lifetime of electric bolts, the table of selectable objects. That state belongs to the game, not to
// a picture, so it advances in every iteration whether or not anything is presented, in the original
// order:
//
//   input -> [before_draw] -> (presentation extraction, only when presenting) -> [after_draw] -> Logic.Update
//
// The original renders the tactical view once per call of DisplayClass::Draw, which is up to twice per
// iteration (the Chrono capture pass and the normal pass). The steps that belong to one tactical render
// therefore run once per round. A round that is extracted brackets the extraction with
// render_phase_tactical_before / _after (DisplayClass::Draw); the rounds that are not extracted run
// headless from render_phase_after_draw. The number of rounds follows simulation state alone.
//
// These are game:: functions, not methods on original classes.
namespace game {
class MapViewHandle;
// `presenting` is true when this iteration's picture is extracted between the two steps.
void render_phase_before_draw(MapViewHandle&, bool presenting) noexcept;
void render_phase_after_draw(MapViewHandle&, bool presenting) noexcept;
// Tactical renders the original performs in this iteration: the Chrono capture pass (when pending)
// plus the normal pass (unless a Chrono transition owns the screen).
unsigned render_phase_tactical_rounds() noexcept;
void render_phase_tactical_before() noexcept;
void render_phase_tactical_after() noexcept;
bool render_phase_ui_active() noexcept;
void render_phase_ui_round_complete() noexcept;
void render_phase_timers_before() noexcept;
void render_phase_radials_before() noexcept;
void render_phase_planning_before() noexcept;
}
