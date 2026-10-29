// YR 0x0048D080 Game::CallBack, the service pump the main loop runs after the
// queued events and while paused.
#include "yrpp/Unsorted.h"
#include "audio_session.hpp"

#if !defined(RA2_YRPP_GAME)
void Game::CallBack() noexcept {
    // 0x48D084 runs the audio AI (0x406F70) first; it is the only part of the pump
    // that applies here. The rest of 0x48D080 services the network and online
    // sessions (0x48D1E0, the WOL queue at 0xA8EA70, the 1 Hz keep-alive), which do
    // not exist in the Campaign and Skirmish modes this host runs.
    game::run_native_audio_ai();
}
#endif
