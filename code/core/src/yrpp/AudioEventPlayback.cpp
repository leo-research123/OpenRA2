#include "audio_session.hpp"
#include "yrpp/Randomizer.h"
#include <algorithm>
#include <cmath>
#if !defined(RA2_AUDIO_GAME) && !defined(RA2_YRPP_GAME)
namespace {
int choose_sample(void*, int low, int high) { return Randomizer::Global.RandomRanged(low, high); }
}
void AudioEvent::Release() noexcept {
    auto& entry = *static_cast<game::AudioEventTag*>(this);
    auto& session = *entry.owner;
    if (entry.controller && entry.controller->Event == &entry && entry.controller->Stamp == entry.stamp)
        entry.controller->Event = nullptr;
    entry.controller = nullptr;
    entry.alive = false;
    entry.stamp = 0;
    entry.State = 4;
    entry.Flags |= 1;
    if (!session.halt(entry.playback)) {
        entry.stopping = true;
        return;
    }
    if (entry.device >= 0 && session.devices[unsigned(entry.device)] == &entry)
        session.devices[unsigned(entry.device)] = nullptr;
    entry.device = -1;
    entry.stopping = false;
    entry.alive = false;
    entry.stamp = 0;
    entry.type = nullptr;
    entry.Playback = nullptr;
    entry.SampleCount = 0;
    std::fill_n(entry.Samples, 32, nullptr);
    entry.prepared.reset();
    entry.pending_decay.reset();
}
bool AudioEvent::StartSample() noexcept {
    auto& entry = *static_cast<game::AudioEventTag*>(this);
    auto& session = *entry.owner;
    session.sample_pressure = false;
    try {
        auto& state = entry;
        const auto& type = *entry.type;
        if (type.MaxDelay < 33) {
            if (!entry.prepared) {
                state.PrepareSamples(entry.sample_indices, choose_sample, nullptr);
                auto source = std::make_shared<game::AudioSource>();
                source->rate = session.mixer.output_rate();
                source->sequence = std::make_unique<game::AudioEventPlaylist>();
                auto& sequence = *source->sequence;
                sequence.event = &entry;
                sequence.rate = source->rate;
                sequence.pitch = entry.playback.pitch;
                for (int slot = 0; slot < state.SampleCount; ++slot) {
                    sequence.parts[slot] = session.sample(type.SampleIndex[entry.sample_indices[slot]]);
                    if (!sequence.parts[slot]) return false;
                    entry.Samples[slot] = sequence.parts[slot].get();
                }
                if (!source->fill() || !sequence.written.load(std::memory_order_acquire)) return false;
                entry.prepared = std::move(source);
            }
            if (session.start(entry.playback, entry.prepared)) return true;
        } else {
            // Deliberate Delay sounds retain the original owner-clock wait.
            // Only gapless callbacks are moved into the sample playlist.
            if (entry.pending == -2) {
                state.PrepareSamples(entry.sample_indices, choose_sample, nullptr);
                if (!state.FirstSample(choose_sample, nullptr, entry.pending)) entry.pending = -1;
            }
            if (entry.pending < 0) {
                entry.Release();
                return false;
            }
            auto source = session.sample(type.SampleIndex[entry.sample_indices[entry.pending]]);
            if (!source) return false;
            if (session.start(entry.playback, std::move(source))) {
                entry.pending = -2;
                state.Flags |= 8;
                return true;
            }
        }
    } catch (...) {
        ++session.failures;
        session.fail("Could not prepare audio sample sequence");
    }
    return false;
}
bool AudioEvent::Finish() noexcept {
    auto& entry = *static_cast<game::AudioEventTag*>(this);
    auto& session = *entry.owner;
    try {
        std::shared_ptr<game::AudioSource> decay;
        if (!entry.pending_decay && entry.playback.source && (unsigned(entry.type->Control) & 0x40)) {
            if (entry.prepared && entry.prepared->sequence) {
                const auto& sequence = *entry.prepared->sequence;
                if (sequence.decay_part > sequence.consumed.load(std::memory_order_acquire))
                    decay = sequence.parts[unsigned(sequence.event->SampleCount - 1)];
            } else if (!(entry.Flags & 0x10) && entry.SampleCount) {
                const int slot = entry.SampleCount - 1;
                decay = session.sample(entry.type->SampleIndex[entry.sample_indices[slot]]);
            }
        }
        // 0x004055C0 stops the current body and starts the decay once. An End
        // during the decay stops it; it must not drain queued loop bodies.
        if (decay) entry.pending_decay = std::move(decay);
        if (entry.pending_decay) {
            // Keep the selected tail across either a rejected stop or start.
            // A queued stop can retire the body before the start has space.
            if (!session.start(entry.playback, entry.pending_decay)) return false;
            entry.Flags |= 0x30;
            entry.prepared.reset();
            entry.pending_decay.reset();
            return true;
        }
    } catch (...) {
        ++session.failures;
    }
    entry.Release();
    return false;
}
void AudioEvent::ApplyGain() noexcept {
    auto& entry = *static_cast<game::AudioEventTag*>(this);
    auto& session = *entry.owner;
    const float gain = entry.gain.GetCurrentVolume() / float(0x4000) * session.effects *
                       (1.0f - entry.VolumeVariation / 100.0f) * session.load_gain.GetCurrentVolume() / float(0x4000);
    const float pan = entry.pan_gain.GetCurrentVolume() / float(0x4000);
    if (entry.playback.gain != gain || entry.playback.pan != pan) {
        entry.playback.gain = gain;
        entry.playback.pan = pan;
        session.parameters(entry.playback);
    }
}
bool AudioEvent::AcquireDevice() noexcept {
    auto& entry = *static_cast<game::AudioEventTag*>(this);
    auto& session = *entry.owner;
    if (entry.device >= 0) return true;
    unsigned chosen = 0;
    if (!Audio::SelectVoice(
            unsigned(session.devices.size()), &session,
            [](void* context, unsigned i, unsigned& flags, unsigned& level, int& priority) noexcept {
                const auto& session = *static_cast<game::AudioSession*>(context);
                if (i >= session.devices.size()) return false;
                flags = level = 0;
                priority = 0;
                if (const auto* owner = session.devices[i]) {
                    flags = owner->playback.active ? 1u : 0x10u;
                    level = unsigned(std::clamp(owner->playback.gain, 0.0f, 1.0f) * 0x4000) << 16;
                    priority = int(owner->type->Priority);
                }
                return true;
            },
            int(entry.type->Priority), chosen))
        return false;
    if (session.devices[chosen]) {
        session.devices[chosen]->Release();
        if (session.devices[chosen]) return false;
    }
    entry.Playback = &entry.playback;
    entry.device = int(chosen);
    session.devices[chosen] = &entry;
    return true;
}
void AudioEvent::Complete(unsigned long long now) noexcept {
    auto& entry = *static_cast<game::AudioEventTag*>(this);
    auto& session = *entry.owner;
    auto& voice = entry.playback;
    if (!voice.active || session.mixer.finished(voice.slot) != voice.stamp) return;
    if (!session.halt(voice)) return;
    // 0x00405A00 publishes terminal state before 0x004041D0 applies Limit.
    // A completed prepared sequence/decay is terminal; a delayed body may
    // still own its event while waiting for the next sample.
    if (entry.prepared || (entry.Flags & 0x20)) {
        entry.Release();
        return;
    }
    if (entry.type->MinDelay < 33) {
        if (!entry.NextSample(choose_sample, nullptr, entry.pending)) entry.pending = -1;
        if (entry.pending < 0) {
            entry.Release();
            return;
        }
    }
    const int delay = choose_sample(nullptr, entry.type->MinDelay, entry.type->MaxDelay);
    entry.deadline = now + (delay >= 33 ? unsigned(delay) : 0u);
}
void Audio::UpdateEvents(unsigned long long now) noexcept {
    auto* active = game::audio_session();
    if (!active) return;
    auto& session = *active;
    std::array<game::AudioEventTag*, 300> ordered{};
    std::array<AudioEvent*, 300> original{};
    unsigned count = 0;
    for (auto& entry : session.events)
        if (entry.alive) {
            entry.Complete(now);
            if (entry.alive) ordered[count++] = &entry;
        }
    std::sort(ordered.begin(), ordered.begin() + count,
              [](const auto* a, const auto* b) { return a->order < b->order; });
    for (unsigned i = 0; i < count; ++i) {
        auto& entry = *ordered[i];
        entry.gain.UpdateAt(now);
        entry.pan_gain.UpdateAt(now);
        if (entry.State != 0) entry.State = entry.playback.active ? 3u : 2u;
        original[i] = &entry;
    }
    Audio::EnforceEventLimits(original.data(), count);
    for (unsigned i = 0; i < count; ++i)
        if (ordered[i]->State == 4) ordered[i]->Release();
    // Advance existing events and reserve the original sixteen standard device
    // channels before building definition buckets and attempting cache loads.
    for (unsigned i = 0; i < count; ++i) {
        auto& entry = *ordered[i];
        if (!entry.alive) continue;
        auto& voice = entry.playback;
        if (entry.stopping) {
            entry.Release();
            continue;
        }
        if (entry.Flags & 0x20) {
            if (entry.Flags & 0x40) {
                if (entry.Finish()) entry.Flags &= ~0x40u;
            } else if (!voice.active || session.mixer.finished(voice.slot) == voice.stamp)
                entry.Release();
            continue;
        }
        if (voice.stop_requested) {
            entry.Release();
            continue;
        }
        if (voice.active) {
            if (voice.source->sequence && !voice.source->fill()) {
                ++session.failures;
                session.fail("Could not refill audio sample sequence");
                entry.Release();
            }
            continue;
        }
        if (entry.State == 0) {
            if (!entry.AcquireDevice()) {
                entry.Release();
                continue;
            }
            entry.State = 1;
        }
    }
    for (unsigned i = 0; i < count; ++i) {
        auto& entry = *ordered[i];
        entry.State = entry.alive ? (entry.playback.active ? 3u : 1u) : 4u;
    }
    session.load_gain.SetTargetAt(Audio::BuildPriorityBuckets(original.data(), count), now);
    session.load_gain.UpdateAt(now);
    for (unsigned i = 0; i < count; ++i) {
        auto& entry = *ordered[i];
        if (!entry.alive) continue;
        entry.ApplyGain();
        if (entry.playback.active || (entry.Flags & 0x20) || entry.playback.stop_requested || now < entry.deadline)
            continue;
        bool started = entry.StartSample();
        while (!started && session.sample_pressure) {
            VocClass* victim = nullptr;
            if (!Audio::FindLowerPriority(int(entry.type->Priority), victim)) break;
            Audio::RemovePriority(*victim);
            bool removed = false;
            for (auto& event : session.events)
                if (event.alive && event.type == victim) {
                    event.Release();
                    removed = true;
                }
            if (!removed) break;
            session.discard_unused_samples();
            started = entry.StartSample();
        }
        if (!started && entry.alive) entry.Release();
    }
}
#endif
