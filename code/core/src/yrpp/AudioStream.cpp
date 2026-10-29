#include "audio_session.hpp"
#include "yrpp/CCFileClass.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
bool Audio::IsAvailable() noexcept { return reinterpret_cast<int(YRPP_CDECL*)()>(0x00407000)() != 0; }
AudioStream* AudioStream::Create(int low, int high) noexcept {
    const auto duration = (static_cast<unsigned long long>(unsigned(high)) << 32) | unsigned(low);
    return reinterpret_cast<AudioStream*(YRPP_THISCALL*)(void*, unsigned long long)>(0x00407860)(
        *reinterpret_cast<void**>(0x0087E728), duration);
}
bool YRPP_FASTCALL AudioStream::PlayWAV(const char* filename, bool resource) {
    return reinterpret_cast<int(YRPP_FASTCALL*)(AudioStream*, const char*, int)>(0x00407B60)(this, filename,
                                                                                             resource) != 0;
}
void AudioStream::Destroy() noexcept { reinterpret_cast<void(YRPP_THISCALL*)(AudioStream*)>(0x00407A90)(this); }
void AudioStream::Stop() noexcept { reinterpret_cast<void(YRPP_THISCALL*)(AudioStream*)>(0x00407F40)(this); }
void AudioStream::Pause() noexcept { reinterpret_cast<void(YRPP_THISCALL*)(AudioStream*)>(0x00407FB0)(this); }
void AudioStream::Resume() noexcept { reinterpret_cast<void(YRPP_THISCALL*)(AudioStream*)>(0x00408000)(this); }
void AudioStream::RestoreVolume() noexcept { reinterpret_cast<void(YRPP_THISCALL*)(AudioStream*)>(0x004080B0)(this); }
void AudioStream::Fade() noexcept { reinterpret_cast<void(YRPP_THISCALL*)(AudioStream*)>(0x004080C0)(this); }
bool AudioStream::IsPlaying() const noexcept {
    return reinterpret_cast<int(YRPP_THISCALL*)(const AudioStream*)>(0x00408070)(this) != 0;
}
bool AudioStream::IsFading() const noexcept {
    return reinterpret_cast<int(YRPP_THISCALL*)(const AudioStream*)>(0x004080D0)(this) != 0;
}
unsigned long long AudioStream::EndTime() const noexcept {
    return reinterpret_cast<unsigned long long(YRPP_THISCALL*)(const AudioStream*)>(0x00408140)(this);
}
void YRPP_FASTCALL AudioStream::SetName(const char* name) noexcept {
    reinterpret_cast<void(YRPP_FASTCALL*)(AudioStream*, const char*)>(0x00408080)(this, name);
}
void YRPP_FASTCALL AudioStream::SetVolumeControl(AudioVolumeControl* control) noexcept {
    reinterpret_cast<void(YRPP_FASTCALL*)(AudioStream*, AudioVolumeControl*)>(0x00407B40)(this, control);
}
void YRPP_FASTCALL AudioStream::SetSecondaryVolumeControl(AudioVolumeControl* control) noexcept {
    reinterpret_cast<void(YRPP_FASTCALL*)(AudioStream*, AudioVolumeControl*)>(0x00407B50)(this, control);
}
#else
namespace {
AudioStream* audio_stream = nullptr;
}
AudioStream*& AudioStream::Instance = audio_stream;

namespace {
game::AudioNativeStream* storage(const AudioStream& stream) noexcept {
    auto* session = game::audio_session();
    return session ? session->stream(stream) : nullptr;
}
}
bool Audio::IsAvailable() noexcept { return game::audio_session() != nullptr; }
AudioStream* AudioStream::Create(int low, int high) noexcept {
    auto* session = game::audio_session();
    if (!session) return nullptr;
    const auto duration = (std::uint64_t(unsigned(high)) << 32) | unsigned(low);
    if (duration > 3600000) return nullptr;
    try {
        for (unsigned i = 0; i < session->streams.size(); ++i)
            if (!session->streams[i]) {
                auto entry = std::make_unique<game::AudioNativeStream>();
                entry->playback.slot = unsigned(session->events.size()) + i;
                entry->volume.InitializeAt(0x4000, Audio::GetTime());
                entry->identity.Duration = std::max(duration, std::uint64_t(7000));
                entry->identity.Playback = &entry->playback;
                entry->primary = reinterpret_cast<AudioVolumeControl*>(&session->effects_control);
                auto* result = &entry->identity;
                session->streams[i] = std::move(entry);
                return result;
            }
    } catch (...) {
        session->fail("Cannot allocate audio stream");
    }
    return nullptr;
}
void AudioStream::Destroy() noexcept {
    auto* session = game::audio_session();
    if (!session) return;
    for (auto& entry : session->streams)
        if (entry && &entry->identity == this) {
            Stop();
            if (session->halt(entry->playback))
                entry.reset();
            else
                entry->destroyed = true;
            return;
        }
}
void YRPP_FASTCALL AudioStream::SetName(const char* name) noexcept { std::snprintf(Name, 80, "%s", name ? name : ""); }
void YRPP_FASTCALL AudioStream::SetVolumeControl(AudioVolumeControl* control) noexcept {
    if (auto* entry = storage(*this)) {
        entry->primary = control;
        entry->playback.gain = GetGain();
        game::audio_session()->parameters(entry->playback);
    }
}
void YRPP_FASTCALL AudioStream::SetSecondaryVolumeControl(AudioVolumeControl* control) noexcept {
    if (auto* entry = storage(*this)) {
        entry->secondary = control;
        entry->playback.gain = GetGain();
        game::audio_session()->parameters(entry->playback);
    }
}
float AudioStream::GetGain() const noexcept {
    auto* entry = storage(*this);
    if (!entry) return 0;
    const auto& session = *game::audio_session();
    const auto gain = [](const AudioVolumeControl* control) {
        return control ? reinterpret_cast<const VolumeStruct*>(control)->GetVolume() / float(0x4000) : 1.0f;
    };
    return gain(entry->primary) * gain(entry->secondary) * session.movie_stream_gain *
           entry->volume.GetCurrentVolume() / float(0x4000);
}
void AudioStream::Stop() noexcept {
    if (Busy) return;
    if (auto* entry = storage(*this)) {
        if (Flags & 3) Ended = Audio::GetTime();
        Flags &= ~3u;
        PauseCount = 0;
        File = nullptr;
        game::audio_session()->halt(entry->playback);
    }
}
void AudioStream::Pause() noexcept {
    if (auto* entry = storage(*this)) {
        if (!PauseCount) {
            game::audio_session()->pause(entry->playback, true);
            if (Flags & 1) Flags = (Flags & ~1u) | 2;
        }
        if (PauseCount != 0xFFFFFFFFu) ++PauseCount;
    }
}
void AudioStream::Resume() noexcept {
    if (auto* entry = storage(*this)) {
        if (PauseCount && --PauseCount == 0) {
            game::audio_session()->pause(entry->playback, false);
            if (Flags & 2) Flags = (Flags & ~2u) | 1;
        }
    }
}
bool AudioStream::IsPlaying() const noexcept {
    auto* entry = storage(*this);
    if (!entry) return false;
    const auto& voice = entry->playback;
    if (voice.active && game::audio_session()->mixer.finished(voice.slot) == voice.stamp) {
        auto& stream = *const_cast<AudioStream*>(this);
        if (stream.Flags & 3) stream.Ended = Audio::GetTime();
        stream.Flags &= ~3u;
        stream.PauseCount = 0;
        stream.File = nullptr;
        game::audio_session()->halt(entry->playback);
    }
    return (Flags & 1) != 0;
}
unsigned long long AudioStream::EndTime() const noexcept {
    (void)IsPlaying();
    return Ended;
}
void AudioStream::RestoreVolume() noexcept {
    if (auto* entry = storage(*this)) entry->volume.SetTarget(0x4000);
}
void AudioStream::Fade() noexcept {
    if (auto* entry = storage(*this)) entry->volume.SetTarget(0);
}
bool AudioStream::IsFading() const noexcept {
    auto* entry = storage(*this);
    return entry && IsPlaying() && entry->volume.unknown_4 != unsigned(entry->volume.unknown_int_8);
}
bool YRPP_FASTCALL AudioStream::PlayWAV(const char* filename, bool resource) {
    auto* entry = storage(*this);
    auto* session = game::audio_session();
    if (!entry || Busy || !filename || !*filename) return false;
    Stop();
    if (entry->playback.stop_requested) return false;
    try {
        auto source = std::make_shared<game::AudioSource>();
        if (!resource) {
            auto file = std::make_unique<RawFileClass>(filename);
            if (file->Exists()) source->file = std::move(file);
        }
        if (!source->file) source->file = std::make_unique<CCFileClass>(filename);
        AudioSampleData info;
        int bytes = 0;
        if (!source->file->Open(FileAccessMode::Read) || !Audio::ReadWAVFile(source->file.get(), &info, &bytes) ||
            !source->decoder.open(*source->file, info, bytes)) {
            ++session->failures;
            session->fail("Cannot open or decode WAV stream");
            return false;
        }
        source->rate = info.SampleRate;
        source->streaming = true;
        source->pcm.resize(game::audio_ring_capacity);
        if (!source->fill() || !source->written.load(std::memory_order_acquire)) {
            ++session->failures;
            session->fail("WAV stream has invalid or empty sample data");
            return false;
        }
        entry->volume.InitializeAt(0x4000, Audio::GetTime());
        entry->playback.gain = GetGain();
        auto* file = source->file.get();
        if (!session->start(entry->playback, std::move(source))) return false;
        Sample = info;
        File = file;
        Flags = (Flags & ~3u) | 1;
        PauseCount = 0;
        Started = Audio::GetTime();
        return true;
    } catch (...) {
        ++session->failures;
        session->fail("WAV stream allocation/read failed");
        return false;
    }
}
void AudioStream::Update(unsigned long long now) noexcept {
    auto* entry = storage(*this);
    if (!entry) return;
    auto& session = *game::audio_session();
    auto& voice = entry->playback;
    if (voice.stop_requested) {
        session.halt(voice);
        return;
    }
    (void)IsPlaying();
    if (!voice.active) return;
    if (voice.source && !voice.source->fill()) {
        ++session.failures;
        session.fail("WAV stream read failed");
        Stop();
        return;
    }
    entry->volume.UpdateAt(now);
    const float gain = GetGain();
    if (gain != voice.gain) {
        voice.gain = gain;
        session.parameters(voice);
    }
}
#endif
