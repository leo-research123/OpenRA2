#pragma once
#include "api/audio_backend.hpp"
#include "audio_mixer.hpp"
#include "yrpp/Audio.h"
#include "yrpp/VocClass.h"
#include "yrpp/MixFileClass.h"
#include <array>
#include <memory>
#include <unordered_map>

namespace game {
struct AudioSession;
struct AudioEventTag;
using AudioEventHandle = AudioEventTag*;
struct AudioPlaybackTag {
    unsigned slot = 0, stamp = 0;
    bool active = false, stop_requested = false, loop = false;
    bool parameters_dirty = false, pause_dirty = false, paused = false;
    float gain = 1, pan = 0.5f, pitch = 1;
    std::shared_ptr<AudioSource> source;
};
struct AudioEventTag : AudioEvent {
    AudioSession* owner = nullptr;
    int sample_indices[32]{};
    AudioPlaybackTag playback;
    bool alive = false, stopping = false;
    std::uint64_t order = 0;
    int device = -1;
    std::shared_ptr<AudioSource> prepared;
    std::shared_ptr<AudioSource> pending_decay;
    int pending = -2;
};
struct AudioNativeStream {
    AudioStream identity;
    AudioPlaybackTag playback;
    bool destroyed = false;
    VolumeStruct volume{};
    AudioVolumeControl* primary = nullptr;
    AudioVolumeControl* secondary = nullptr;
};
struct AudioSession final {
    explicit AudioSession(unsigned rate);
    AudioMixer mixer;
    ResourceHandle* resources = nullptr;
    bool ready = false, suspended = false;
    bool updating = false;
    float effects = 1, speech = 1, music = 1, movie_stream_gain = 1;
    VolumeStruct master_control{}, speech_control{}, effects_control{}, music_control{};
    VolumeStruct load_gain{};
    std::array<AudioEventTag*, 16> devices{};
    std::uint64_t next_event_order = 0;
    unsigned next_stamp = 0, missing = 0, failures = 0, overflows = 0;
    unsigned long long last_update = 0, suspended_at = 0;
    char error[256]{};
    std::array<AudioEventTag, 300> events{};
    std::array<std::unique_ptr<AudioNativeStream>, 20> streams{};
    std::unique_ptr<MixFileClass> audio_mix, theme_mix;
    std::unique_ptr<AudioIDXData> index;
    std::unordered_map<int, std::shared_ptr<AudioSource>> samples;
    std::size_t sample_bytes = 0;
    std::size_t sample_budget = 128 * 1024 * 1024;
    bool sample_pressure = false;
    struct Retired {
        std::uint64_t sequence;
        std::shared_ptr<AudioSource> source;
    };
    std::vector<Retired> retired;
    unsigned stamp() noexcept;
    void fail(const char*) noexcept;
    bool command(AudioMixCommand, std::uint64_t&) noexcept;
    bool start(AudioPlaybackTag&, std::shared_ptr<AudioSource>, bool loop = false) noexcept;
    bool halt(AudioPlaybackTag&) noexcept;
    void parameters(AudioPlaybackTag&) noexcept;
    void pause(AudioPlaybackTag&, bool paused) noexcept;
    void collect() noexcept;
    std::shared_ptr<AudioSource> sample(int);
    AudioEventTag* event(AudioEventHandle) noexcept;
    AudioNativeStream* stream(const AudioStream&) noexcept;
    void discard_unused_samples() noexcept;
};
AudioSession* audio_session() noexcept;
AudioEventTag* audio_event(void* event) noexcept;
bool native_audio_ready() noexcept;
void suspend_native_audio(bool suspended) noexcept;
// Once per host update: suspension, buffer collection, parameter retry and stream refill.
void update_native_audio(bool focused, bool paused) noexcept;
// The audio AI of the original service pump (Game::CallBack, 0x00406F70), gated to 33 ms:
// sound events, positioned sounds, speech and the theme. Runs once per main-loop iteration and
// does nothing while output is suspended or when no session is active.
void run_native_audio_ai() noexcept;
void stop_native_audio() noexcept;
void stop_native_sound_effects() noexcept;
}
