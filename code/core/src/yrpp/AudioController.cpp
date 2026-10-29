#include "yrpp/Audio.h"
#include "audio_session.hpp"
#include <algorithm>

#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
AudioController::~AudioController() { reinterpret_cast<void(YRPP_THISCALL*)(AudioController*)>(0x00405C00)(this); }
void AudioController::Stop() { reinterpret_cast<void(YRPP_THISCALL*)(AudioController*)>(0x00405D40)(this); }
void AudioController::End() { reinterpret_cast<void(YRPP_THISCALL*)(AudioController*)>(0x00405FD0)(this); }
void AudioController::StopLooping() { reinterpret_cast<void(YRPP_THISCALL*)(AudioController*)>(0x00405E80)(this); }
void AudioController::EndLooping() { reinterpret_cast<void(YRPP_THISCALL*)(AudioController*)>(0x00406060)(this); }
void YRPP_FASTCALL AudioController::SetEvent(void* event, VocClass* type) {
    reinterpret_cast<void(YRPP_FASTCALL*)(AudioController*, void*, VocClass*)>(0x004060F0)(this, event, type);
}
void* AudioController::GetEvent() {
    return reinterpret_cast<void*(YRPP_THISCALL*)(AudioController*)>(0x00406130)(this);
}
VocClass* AudioController::GetEventType() {
    return reinterpret_cast<VocClass*(YRPP_THISCALL*)(AudioController*)>(0x00406170)(this);
}
void YRPP_FASTCALL AudioController::AdjustAudioEventVolume(void* event, unsigned volume) {
    reinterpret_cast<void(YRPP_FASTCALL*)(void*, unsigned)>(0x004061D0)(event, volume);
}
void YRPP_FASTCALL AudioController::AdjustAudioEventPanning(void* event, unsigned pan) {
    reinterpret_cast<void(YRPP_FASTCALL*)(void*, unsigned)>(0x00406270)(event, pan);
}
VocClass* YRPP_FASTCALL AudioController::GetEventType(void* event) {
    return reinterpret_cast<VocClass*(YRPP_FASTCALL*)(void*)>(0x00406310)(event);
}
#else
AudioController::~AudioController() {
    Stop();
    if (auto* session = game::audio_session())
        for (auto& entry : session->events)
            if (entry.controller == this) entry.controller = nullptr;
    AudioIndex = nullptr;
}
void* AudioController::GetEvent() {
    auto* session = game::audio_session();
    if (!session || !session->ready || AudioIndex != &AudioIDXData::Instance) return nullptr;
    auto* entry = game::audio_event(Event);
    if (!entry || entry->stamp != Stamp || entry->type != EventType) {
        Event = nullptr;
        return nullptr;
    }
    return entry;
}
VocClass* AudioController::GetEventType() {
    auto* session = game::audio_session();
    return session && session->ready && AudioIndex == &AudioIDXData::Instance ? EventType : nullptr;
}
void YRPP_FASTCALL AudioController::SetEvent(void* handle, VocClass* type) {
    if (AudioIndex != &AudioIDXData::Instance) return;
    if (auto* old = static_cast<AudioEvent*>(GetEvent()))
        if (old->controller == this) old->controller = nullptr;
    auto* entry = game::audio_event(handle);
    Event = entry;
    EventType = type;
    if (entry) {
        Stamp = entry->stamp;
        if (!type) EventType = entry->type;
        entry->controller = this;
    }
}
void AudioController::Stop() {
    auto* session = game::audio_session();
    if (session && session->ready && AudioIndex == &AudioIDXData::Instance) {
        if (auto* entry = static_cast<AudioEvent*>(GetEvent())) entry->Release();
        Event = nullptr;
    }
    EventType = nullptr;
}
void AudioController::End() {
    auto* session = game::audio_session();
    if (session && session->ready && AudioIndex == &AudioIDXData::Instance) {
        if (auto* entry = game::audio_event(GetEvent())) {
            if (!(entry->Flags & 0x20)) entry->Flags |= 0x60;
            entry->controller = nullptr;
        }
        Event = nullptr;
    }
    EventType = nullptr;
}
void AudioController::StopLooping() {
    if (auto* entry = static_cast<AudioEvent*>(GetEvent())) {
        if ((unsigned(entry->type->Control) & 1) && !entry->type->Loop)
            entry->Release();
        else
            entry->controller = nullptr;
    }
    EventType = nullptr;
}
void AudioController::EndLooping() {
    if (auto* entry = static_cast<AudioEvent*>(GetEvent())) {
        if ((unsigned(entry->type->Control) & 1) && !entry->type->Loop) {
            End();
            return;
        }
        entry->controller = nullptr;
    }
    EventType = nullptr;
}
void YRPP_FASTCALL AudioController::AdjustAudioEventVolume(void* handle, unsigned volume) {
    if (auto* entry = game::audio_event(handle)) {
        const unsigned gain = (std::min(volume, 0x4000u) * unsigned(entry->type->Volume.GetVolume())) >> 14;
        if (entry->State == 3)
            entry->gain.SetTargetAt(gain, Audio::GetTime());
        else
            entry->gain.InitializeAt(gain, Audio::GetTime());
        entry->ApplyGain();
    }
}
void YRPP_FASTCALL AudioController::AdjustAudioEventPanning(void* handle, unsigned pan) {
    if (auto* entry = game::audio_event(handle)) {
        if (entry->State == 3)
            entry->pan_gain.SetTargetAt(std::min(pan, 0x4000u), Audio::GetTime());
        else
            entry->pan_gain.InitializeAt(std::min(pan, 0x4000u), Audio::GetTime());
        entry->ApplyGain();
    }
}
VocClass* YRPP_FASTCALL AudioController::GetEventType(void* handle) {
    auto* entry = game::audio_event(handle);
    return entry ? entry->type : nullptr;
}
#endif
