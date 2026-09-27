#include "yrpp/Audio.h"
#include "api/audio_backend.hpp"

#ifndef RA2_AUDIO_GAME
namespace { AudioStream* audio_stream = nullptr; }
AudioStream*& AudioStream::Instance = audio_stream;
#endif

bool YRPP_FASTCALL AudioStream::PlayWAV(const char* filename, bool resource_file) {
    game::AudioBackend* backend = nullptr;
    return game::get_audio_backend(backend) &&
        backend->play_wav(*this, filename, resource_file);
}

bool Audio::IsAvailable() noexcept {
    game::AudioBackend* backend=nullptr;
    return game::get_audio_backend(backend) && backend->stream_device_available();
}
AudioStream* AudioStream::Create(int bufferSize,int flags) noexcept {
    game::AudioBackend* backend=nullptr;AudioStream* stream=nullptr;
    return game::get_audio_backend(backend) && backend->create_stream(bufferSize,flags,stream)?stream:nullptr;
}
void AudioStream::Destroy() noexcept {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend))backend->destroy_stream(*this);
}
void YRPP_FASTCALL AudioStream::SetName(const char* name) noexcept {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend))backend->name_stream(*this,name);
}
void YRPP_FASTCALL AudioStream::SetVolumeControl(AudioVolumeControl* control) noexcept {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend))backend->bind_stream_volume(*this,control,false);
}
void YRPP_FASTCALL AudioStream::SetSecondaryVolumeControl(AudioVolumeControl* control) noexcept {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend))backend->bind_stream_volume(*this,control,true);
}
void AudioStream::Stop() noexcept {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend))backend->stop_stream(*this);
}
void AudioStream::Pause() noexcept {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend))backend->pause_stream(*this);
}
void AudioStream::Resume() noexcept {
    game::AudioBackend* backend=nullptr;
    if(game::get_audio_backend(backend))backend->resume_stream(*this);
}
bool AudioStream::IsPlaying() const noexcept {
    game::AudioBackend* backend=nullptr;bool playing=false;
    return game::get_audio_backend(backend) && backend->stream_is_playing(*this,playing) && playing;
}
unsigned long long AudioStream::EndTime() const noexcept {
    game::AudioBackend* backend=nullptr;unsigned long long time=0;
    if(game::get_audio_backend(backend))backend->stream_end_time(*this,time);
    return time;
}
