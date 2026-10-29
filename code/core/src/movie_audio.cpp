#include "movie_player.hpp"
#include "yrpp/Audio.h"
#include "yrpp/VoxClass.h"
#include "audio_session.hpp"
namespace game {
namespace {
bool movie_audio = false;
}
void enter_movie_audio() noexcept {
    if (movie_audio) return;
    VoxClass::Pause();
    Audio::HoldSounds();
    Audio::PauseSounds();
    Audio::SetStreamVolume(0.5f);
    movie_audio = true;
}
void leave_movie_audio() noexcept {
    if (!movie_audio) return;
    Audio::ReleaseSounds();
    Audio::ResumeSounds();
    Audio::RestoreSounds();
    Audio::SetStreamVolume(1.0f);
    VoxClass::Resume();
    movie_audio = false;
}
}
