#include "yrpp/Audio.h"
#include "api/audio_backend.hpp"

AudioController::~AudioController() {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->destroy_controller(*this);
}
void AudioController::Stop() {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->stop(*this);
}
void AudioController::End() {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->end(*this);
}
void AudioController::StopLooping() {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->stop_looping(*this);
}
void AudioController::EndLooping() {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->end_looping(*this);
}
void YRPP_FASTCALL AudioController::SetEvent(void* event, VocClass* type) {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->set_event(*this, static_cast<game::AudioEventHandle>(event), type);
}
void* AudioController::GetEvent() {
    game::AudioBackend* backend = nullptr;
    game::AudioEventHandle event = nullptr;
    return game::get_audio_backend(backend) &&
        backend->get_event(*this, event) ? event : nullptr;
}
VocClass* AudioController::GetEventType() {
    game::AudioBackend* backend = nullptr;
    VocClass* type = nullptr;
    return game::get_audio_backend(backend) &&
        backend->get_controller_type(*this, type) ? type : nullptr;
}
void YRPP_FASTCALL AudioController::AdjustAudioEventVolume(void* event, unsigned int volume) {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->set_volume(static_cast<game::AudioEventHandle>(event), volume);
}
void YRPP_FASTCALL AudioController::AdjustAudioEventPanning(void* event, unsigned int pan) {
    game::AudioBackend* backend = nullptr;
    if (game::get_audio_backend(backend)) backend->set_panning(static_cast<game::AudioEventHandle>(event), pan);
}
VocClass* YRPP_FASTCALL AudioController::GetEventType(void* event) {
    game::AudioBackend* backend = nullptr;
    VocClass* type = nullptr;
    return game::get_audio_backend(backend) &&
        backend->get_event_type(static_cast<game::AudioEventHandle>(event), type) ? type : nullptr;
}
