#include "audio_session.hpp"
#include "yrpp/VoxClass.h"
#include "yrpp/ThemeClass.h"
void Audio::AI() noexcept {
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
    reinterpret_cast<void(YRPP_CDECL*)()>(0x00406F70)();
#else
    auto* active = game::audio_session();
    if (!active) return;
    auto& s = *active;
    if (s.updating || s.suspended) return;
    s.updating = true;
    struct RestoreUpdate {
        bool& updating;
        ~RestoreUpdate() { updating = false; }
    } restore{s.updating};
    const auto now = Audio::GetTime();
    if (now - s.last_update <= 33) return;
    Audio::UpdateEvents(now);
    VocClass::UpdatePositionedSounds();
    VoxClass::Update();
    ThemeClass::Instance.AI();
    // Match 0x00406F70: callbacks see the previous tick until all phases finish.
    s.last_update = now;
#endif
}
