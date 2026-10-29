// Original stream-list controls used by radar movie preparation and retirement.
#include "yrpp/Audio.h"
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
void Audio::FadeSounds() noexcept { reinterpret_cast<void(YRPP_CDECL*)()>(0x408390)(); }
bool Audio::SoundsFaded() noexcept { return reinterpret_cast<bool(YRPP_CDECL*)()>(0x408410)(); }
void Audio::HoldSounds() noexcept { reinterpret_cast<void(YRPP_CDECL*)()>(0x408200)(); }
void Audio::ReleaseSounds() noexcept { reinterpret_cast<void(YRPP_CDECL*)()>(0x408230)(); }
void Audio::PauseSounds() noexcept { reinterpret_cast<void(YRPP_CDECL*)()>(0x408270)(); }
void Audio::ResumeSounds() noexcept { reinterpret_cast<void(YRPP_CDECL*)()>(0x4082F0)(); }
void Audio::RestoreSounds() noexcept { reinterpret_cast<void(YRPP_CDECL*)()>(0x4083D0)(); }
#else
#include "audio_session.hpp"
void Audio::SetStreamVolume(float volume) noexcept {
    if (auto* session = game::audio_session()) {
        session->movie_stream_gain = volume;
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed) {
                stream->playback.gain = stream->identity.GetGain();
                session->parameters(stream->playback);
            }
    }
}
void Audio::FadeSounds() noexcept {
    if (auto* session = game::audio_session())
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed) stream->identity.Fade();
}
bool Audio::SoundsFaded() noexcept {
    if (auto* session = game::audio_session())
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed && stream->identity.IsFading()) return false;
    return true;
}
void Audio::RestoreSounds() noexcept {
    if (auto* session = game::audio_session())
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed) stream->identity.RestoreVolume();
}
void Audio::HoldSounds() noexcept {
    if (auto* session = game::audio_session())
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed) ++stream->identity.Busy;
}
void Audio::ReleaseSounds() noexcept {
    if (auto* session = game::audio_session())
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed && stream->identity.Busy) --stream->identity.Busy;
}
void Audio::PauseSounds() noexcept {
    if (auto* session = game::audio_session())
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed) stream->identity.Pause();
}
void Audio::ResumeSounds() noexcept {
    if (auto* session = game::audio_session())
        for (auto& stream : session->streams)
            if (stream && !stream->destroyed) stream->identity.Resume();
}
#endif
