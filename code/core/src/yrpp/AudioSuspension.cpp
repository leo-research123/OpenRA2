// Original device suspension entries.
#include "yrpp/Audio.h"
#include "audio_session.hpp"
#if !defined(RA2_YRPP_GAME)
void Audio::SuspendAll() noexcept {
#if defined(RA2_AUDIO_GAME)
    reinterpret_cast<void(YRPP_CDECL*)()>(0x00406F00)();
#else
    game::suspend_native_audio(true);
#endif
}
void Audio::ResumeAll() noexcept {
#if defined(RA2_AUDIO_GAME)
    reinterpret_cast<void(YRPP_CDECL*)()>(0x00406F40)();
#else
    game::suspend_native_audio(false);
#endif
}
#endif
