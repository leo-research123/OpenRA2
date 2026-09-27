// Scenario pause/resume sequence, YR 683EB0 / 683FB0.
#include "yrpp/ScenarioClass.h"
#include "yrpp/VocClass.h"
#include "scenario_runtime.hpp"
#include <stdexcept>

namespace {
const game::ScenarioPauseServices& pausing() {
    const auto& runtime = game::scenario_runtime();
    const auto* pause = runtime.pause;
    if (!ScenarioClass::Instance || !pause || !pause->volume || !*pause->volume ||
        !pause->suspend_audio || !pause->resume_audio || !pause->set_input_paused ||
        !pause->set_tooltip_state || !pause->release_mouse_capture || !pause->set_cursor ||
        !pause->restore_cursor || !pause->hide_cursor || !pause->show_cursor || !pause->render_frame ||
        !runtime.render || !runtime.render->redraw_sidebar ||
        (pause->tooltip && *pause->tooltip && !pause->tooltips_enabled))
        throw std::logic_error("Scenario pause requires the existing audio, input and display services");
    return *pause;
}
}

void YRPP_FASTCALL ScenarioClass::PauseGame() {
    const auto& pause = pausing();
    pause.suspend_audio(pause.context);
    PausedAudioVolume = (*pause.volume)->GetVolume();
    (*pause.volume)->SetVolume(0x4000);
    const auto& runtime = game::scenario_runtime();
    const int mode = runtime.session_mode(runtime.context);
    if (mode == 0 || mode == 5) {
        // The original debug log samples the clock even with its logger off.
        // Preserve that observable sample before pausing the count-up timer.
        (void)Instance->ElapsedTimer.GetTimeElapsed();
        Instance->ElapsedTimer.Pause();
    }
    pause.set_input_paused(pause.context, true);
    if (pause.tooltip && *pause.tooltip) pause.set_tooltip_state(pause.context, *pause.tooltip, false);
    pause.release_mouse_capture(pause.context);
    pause.set_cursor(pause.context, 0, false);
    pause.hide_cursor(pause.context);
    runtime.render->redraw_sidebar(runtime.render->context, 2);
    pause.render_frame(pause.context);
    pause.show_cursor(pause.context);
}

void YRPP_FASTCALL ScenarioClass::ResumeGame() {
    const auto& pause = pausing();
    pause.restore_cursor(pause.context);
    Instance->ElapsedTimer.Resume();
    (void)Instance->ElapsedTimer.GetTimeElapsed();
    if (pause.tooltip && *pause.tooltip)
        pause.set_tooltip_state(pause.context, *pause.tooltip, *pause.tooltips_enabled);
    (*pause.volume)->SetVolume(PausedAudioVolume);
    PausedAudioVolume = 0x4000;
    pause.set_input_paused(pause.context, false);
    pause.resume_audio(pause.context);
}
