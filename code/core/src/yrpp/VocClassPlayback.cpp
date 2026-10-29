#include "audio_session.hpp"
#include "yrpp/Randomizer.h"
#include <algorithm>
#include <cmath>
#if !defined(RA2_YRPP_GAME)
void YRPP_FASTCALL VocClass::PlayGlobal(int index_value, int pan, float volume, AudioController* controller) {
#if defined(RA2_AUDIO_GAME)
    reinterpret_cast<void(YRPP_FASTCALL*)(int, int, float, AudioController*)>(0x00750920)(index_value, pan, volume,
                                                                                          controller);
#else
    auto* active = game::audio_session();
    if (!active) return;
    auto& session = *active;
    if (!session.ready || !VocClass::VoicesEnabled || Audio::Quiet || !std::isfinite(volume)) return;
    auto* type = VocClass::Get(index_value);
    auto* existing = controller ? static_cast<game::AudioEventTag*>(controller->GetEvent()) : nullptr;
    if (existing && existing->type != type) {
        existing->Release();
        existing = nullptr;
    }
    if (!type || !type->SamplesOK || !type->NumSamples) {
        if (controller) controller->SetEvent(nullptr, type);
        return;
    }
    if (existing) {
        AudioController::AdjustAudioEventVolume(existing, unsigned(std::clamp(volume, 0.0f, 1.0f) * 0x4000));
        AudioController::AdjustAudioEventPanning(existing, unsigned(std::clamp(pan, 0, 0x4000)));
        return;
    }
    try {
        game::AudioEventTag* available = nullptr;
        for (auto& entry : session.events)
            if (!entry.alive && !entry.playback.source) {
                available = &entry;
                break;
            }
        // 0x00405190 allocates event records independently of device voices.
        if (!available) return;
        const auto slot = available->playback.slot;
        *available = {};
        available->owner = &session;
        available->playback.slot = slot;
        auto& entry = *available;
        entry.type = type;
        entry.stamp = session.stamp();
        entry.alive = true;
        entry.order = ++session.next_event_order;
        const auto choose = [](int low, int high) {
            return low < high ? Randomizer::Global.RandomRanged(low, high) : low;
        };
        if (type->NumSamples <= type->Attack + type->Decay) {
            entry.Release();
            return;
        }
        entry.PitchPercent = 100 + choose(type->MinFDelta, type->MaxFDelta);
        entry.playback.pitch = std::clamp(entry.PitchPercent / 100.0f, 0.01f, 4.0f);
        entry.VolumeVariation = std::clamp(choose(0, std::max(0, type->VShift)), 0, 100);
        AudioController::AdjustAudioEventVolume(&entry, unsigned(std::clamp(volume, 0.0f, 1.0f) * 0x4000));
        AudioController::AdjustAudioEventPanning(&entry, unsigned(std::clamp(pan, 0, 0x4000)));
        entry.deadline = Audio::GetTime();
        if ((unsigned(type->Control) & 0x88) && type->MaxDelay >= 33)
            entry.deadline +=
                std::max(0, choose((unsigned(type->Control) & 0x80) ? 33 : type->MinDelay, type->MaxDelay));
        if (controller) controller->SetEvent(&entry, type);
        return;
    } catch (...) {
        ++session.failures;
        session.fail("Could not prepare sound event");
        return;
    }
#endif
}
#endif
