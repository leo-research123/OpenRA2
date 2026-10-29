#include "audio_session.hpp"
#include "api/filesystem.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/ThemeClass.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace game {
namespace {
AudioSession* active = nullptr;
}
AudioSession::AudioSession(unsigned rate) : mixer(rate) {
    master_control.SetVolume(0x4000);
    speech_control.SetVolume(0x4000);
    effects_control.SetVolume(0x4000);
    music_control.SetVolume(0x4000);
    load_gain.InitializeAt(0x4000, Audio::GetTime());
    retired.reserve(8192);
    for (unsigned i = 0; i < events.size(); ++i) {
        events[i].playback.slot = i;
        events[i].owner = this;
    }
}
unsigned AudioSession::stamp() noexcept {
    if (!++next_stamp) ++next_stamp;
    return next_stamp;
}
void AudioSession::fail(const char* message) noexcept { std::snprintf(error, sizeof(error), "%s", message); }
bool AudioSession::command(AudioMixCommand cmd, std::uint64_t& sequence) noexcept {
    if (mixer.submit(cmd, sequence)) return true;
    ++overflows;
    fail("Audio command queue is full");
    return false;
}
void AudioSession::collect() noexcept {
    const auto consumed = mixer.consumed();
    std::erase_if(retired, [consumed](const Retired& item) { return item.sequence <= consumed; });
}
bool AudioSession::halt(AudioPlaybackTag& voice) noexcept {
    if (!voice.source) {
        voice.active = false;
        voice.stop_requested = false;
        return true;
    }
    voice.stop_requested = true;
    collect();
    if (retired.size() == retired.capacity()) {
        fail("Audio consumer is not draining retired buffers");
        return false;
    }
    std::uint64_t sequence = 0;
    if (!command({AudioMixOperation::stop, voice.slot, voice.stamp}, sequence)) return false;
    retired.push_back({sequence, std::move(voice.source)});
    voice.active = false;
    voice.stop_requested = false;
    voice.parameters_dirty = false;
    voice.pause_dirty = voice.paused = false;
    return true;
}
bool AudioSession::start(AudioPlaybackTag& voice, std::shared_ptr<AudioSource> source, bool loop) noexcept {
    if (!source ||
        (source->sequence ? !source->sequence->written.load(std::memory_order_acquire) : source->pcm.empty()) ||
        !halt(voice))
        return false;
    const unsigned serial = stamp();
    std::uint64_t sequence = 0;
    if (!command({AudioMixOperation::start, voice.slot, serial, source.get(), voice.gain, voice.pan, voice.pitch, loop},
                 sequence))
        return false;
    voice.source = std::move(source);
    voice.stamp = serial;
    voice.active = true;
    voice.loop = loop;
    voice.parameters_dirty = false;
    voice.pause_dirty = voice.paused = false;
    return true;
}
void AudioSession::parameters(AudioPlaybackTag& voice) noexcept {
    if (!voice.active) return;
    voice.parameters_dirty = true;
    std::uint64_t sequence = 0;
    if (command({AudioMixOperation::parameters, voice.slot, voice.stamp, nullptr, voice.gain, voice.pan, voice.pitch,
                 voice.loop},
                sequence))
        voice.parameters_dirty = false;
}
void AudioSession::pause(AudioPlaybackTag& voice, bool paused) noexcept {
    voice.paused = paused;
    voice.pause_dirty = voice.active;
    if (!voice.active) return;
    std::uint64_t sequence = 0;
    AudioMixCommand cmd{AudioMixOperation::pause, voice.slot, voice.stamp};
    cmd.flag = paused;
    if (command(cmd, sequence)) voice.pause_dirty = false;
}
bool create_audio_session(unsigned rate, AudioSession*& output) noexcept {
    if (output || active || rate < 8000 || rate > 192000) return false;
    try {
        auto session = std::make_unique<AudioSession>(rate);
        output = session.release();
        active = output;
        Audio::MasterVolume = reinterpret_cast<AudioVolumeControl*>(&output->master_control);
        Audio::SpeechVolume = reinterpret_cast<AudioVolumeControl*>(&output->speech_control);
        return true;
    } catch (...) {
        return false;
    }
}
AudioNativeStream* AudioSession::stream(const AudioStream& identity) noexcept {
    for (auto& entry : streams)
        if (entry && !entry->destroyed && &entry->identity == &identity) return entry.get();
    return nullptr;
}
AudioSession* audio_session() noexcept { return active; }
AudioEventTag* AudioSession::event(AudioEventHandle handle) noexcept {
    if (!handle) return nullptr;
    const auto address = reinterpret_cast<std::uintptr_t>(handle);
    const auto base = reinterpret_cast<std::uintptr_t>(events.data());
    if (address < base) return nullptr;
    const auto offset = address - base;
    if (offset >= sizeof(events) || offset % sizeof(AudioEventTag)) return nullptr;
    auto& entry = events[offset / sizeof(AudioEventTag)];
    return entry.alive ? &entry : nullptr;
}
AudioEventTag* audio_event(void* handle) noexcept {
    return active ? active->event(static_cast<AudioEventHandle>(handle)) : nullptr;
}
bool native_audio_ready() noexcept { return active && active->ready; }
const char* audio_session_error(const AudioSession& session) noexcept { return session.error; }
bool get_audio_statistics(const AudioSession& s, AudioStatistics& output) noexcept {
    if (active != &s) return false;
    AudioStatistics result;
    for (const auto& event : s.events)
        if (event.alive) ++result.active_events;
    for (const auto& stream : s.streams)
        if (stream && stream->playback.active) ++result.active_streams;
    result.sound_definitions = unsigned(VocClass::Array.Count);
    result.missing_samples = s.missing;
    result.decode_failures = s.failures;
    result.command_overflows = s.overflows;
    result.underruns = s.mixer.underruns();
    result.resources_ready = s.ready;
    output = result;
    return true;
}
bool set_audio_levels(AudioSession& s, float effects, float speech, float music) noexcept {
    if (active != &s || !std::isfinite(effects) || !std::isfinite(speech) || !std::isfinite(music) || effects < 0 ||
        effects > 1 || speech < 0 || speech > 1 || music < 0 || music > 1)
        return false;
    s.effects = effects;
    s.speech = speech;
    s.music = music;
    s.speech_control.SetVolume(int(speech * 0x4000));
    s.effects_control.SetVolume(int(effects * 0x4000));
    s.music_control.SetVolume(int(music * 0x4000));
    ThemeClass::Instance.Volume = int(music * 255);
    for (auto& event : s.events)
        if (event.alive) event.ApplyGain();
    return true;
}
bool get_audio_levels(const AudioSession& s, float& effects, float& speech, float& music) noexcept {
    if (active != &s) return false;
    effects = s.effects;
    speech = s.speech;
    music = s.music;
    return true;
}
bool load_audio_session(AudioSession& s, ResourceHandle& resources) noexcept {
    if (active != &s || (s.resources && s.resources != &resources)) return false;
    if (s.ready) return true;
    bool loaded = false;
    try {
        std::string error;
        struct Context {
            AudioSession& s;
            bool& loaded;
        } context{s, loaded};
        const bool entered = with_resources(
            resources,
            [](void* pointer) {
                auto& c = *static_cast<Context*>(pointer);
                auto& s = c.s;
                CCFileClass audio("AUDIOMD.MIX");
                s.audio_mix = std::make_unique<MixFileClass>(audio.Exists() ? "AUDIOMD.MIX" : "AUDIO.MIX");
                if (!s.audio_mix->CountFiles) {
                    s.fail("Audio MIX is unavailable");
                    return;
                }
                s.index.reset(AudioIDXData::Create("audio", nullptr));
                if (!s.index) {
                    s.fail("AUDIO.IDX/AUDIO.BAG is unavailable");
                    return;
                }
                AudioIDXData::Instance = s.index.get();
                // Music packages are optional for native hosts. Compact resource
                // installations may contain an empty placeholder; effects/EVA must
                // remain usable when this independent package cannot be mounted.
                try {
                    CCFileClass music("THEMEMD.MIX");
                    if (music.Exists())
                        s.theme_mix = std::make_unique<MixFileClass>("THEMEMD.MIX");
                    else {
                        CCFileClass fallback("THEME.MIX");
                        if (fallback.Exists()) s.theme_mix = std::make_unique<MixFileClass>("THEME.MIX");
                    }
                } catch (...) {
                    s.fail("Music MIX is unavailable; sound effects and speech remain enabled");
                }
                CCINIClass sound, eva, theme, options;
                const auto read = [](CCINIClass& ini, const char* md, const char* base) {
                    CCFileClass file(md);
                    return ini.LoadFromFile(file.Exists() ? md : base) == 1;
                };
                if (!read(sound, "SOUNDMD.INI", "SOUND.INI") || !read(eva, "EVAMD.INI", "EVA.INI") ||
                    !read(theme, "THEMEMD.INI", "THEME.INI")) {
                    s.fail("Audio definition INI is unavailable");
                    return;
                }
                if (!VocClass::LoadAllFromINI(&sound) || !VoxClass::LoadAllFromINI(&eva) ||
                    !ThemeClass::Instance.LoadFromINI(&theme) || !VoxClass::Initialize()) {
                    s.fail("Could not initialize audio definitions and streams");
                    return;
                }
                ThemeClass::Instance.Scan();
                if (read(options, "RA2MD.INI", "RA2.INI")) {
                    set_audio_levels(s, float(std::clamp(options.ReadDouble("Options", "SoundVolume", 0.8), 0.0, 1.0)),
                                     float(std::clamp(options.ReadDouble("Options", "VoiceVolume", 0.8), 0.0, 1.0)),
                                     float(std::clamp(options.ReadDouble("Options", "ScoreVolume", 0.8), 0.0, 1.0)));
                    ThemeClass::Instance.IsScoreRepeat = options.ReadBool("Options", "IsScoreRepeat", false);
                    ThemeClass::Instance.IsScoreShuffle = options.ReadBool("Options", "IsScoreShuffle", true);
                }
                c.loaded = true;
            },
            &context, error);
        if (!entered) s.fail(error.c_str());
    } catch (...) {
        s.fail("Audio resource initialization failed");
    }
    s.resources = &resources;
    s.ready = loaded;
    if (!loaded) {
        VoxClass::DeleteAll();
        VoxClass::Shutdown();
        ThemeClass::Instance.Clear();
        VocClass::DeleteAll();
        AudioIDXData::Instance = nullptr;
        s.index.reset();
        s.theme_mix.reset();
        s.audio_mix.reset();
    }
    return loaded;
}
void stop_audio_session(AudioSession& s) noexcept {
    if (active != &s) return;
    VoxClass::Reset();
    ThemeClass::Instance.Stop();
    VocClass::ClearPositionedSounds();
    for (auto& event : s.events)
        if (event.alive) event.Release();
    s.load_gain.InitializeAt(0x4000, Audio::GetTime());
    for (auto& stream : s.streams)
        if (stream) {
            stream->identity.Busy = 0;
            stream->identity.Stop();
        }
}
void stop_native_sound_effects() noexcept {
    if (active)
        for (auto& event : active->events)
            if (event.alive) event.Release();
}
void stop_native_audio() noexcept {
    if (active) stop_audio_session(*active);
}
void destroy_audio_session(AudioSession*& s) noexcept {
    if (!s || active != s) return;
    // The caller has joined the output consumer; buffers can now be freed even
    // when its final stop commands have not been consumed.
    stop_audio_session(*s);
    VoxClass::DeleteAll();
    VoxClass::Shutdown();
    ThemeClass::Instance.Clear();
    VocClass::DeleteAll();
    AudioIDXData::Instance = nullptr;
    Audio::MasterVolume = Audio::SpeechVolume = nullptr;
    active = nullptr;
    delete s;
    s = nullptr;
}
void suspend_audio_session(AudioSession& s, bool suspended) noexcept {
    // An unchanged host state must not sample the original audio clock.
    // The regular update owns its single scheduling sample at 0x00406F70.
    if (active != &s || suspended == s.suspended) return;
    const auto now = Audio::GetTime();
    if (suspended != s.suspended) {
        std::uint64_t sequence = 0;
        AudioMixCommand cmd;
        cmd.operation = AudioMixOperation::suspend;
        cmd.flag = suspended;
        if (s.command(cmd, sequence)) {
            if (suspended)
                s.suspended_at = now;
            else {
                const auto elapsed = now - s.suspended_at;
                const auto shift_gain = [elapsed](VolumeStruct& value) {
                    const auto time =
                        ((std::uint64_t(unsigned(value.unknown_int_1C)) << 32) | value.nknown_18) + elapsed;
                    value.nknown_18 = unsigned(time);
                    value.unknown_int_1C = int(time >> 32);
                };
                shift_gain(s.load_gain);
                for (auto& event : s.events)
                    if (event.alive) {
                        event.deadline += elapsed;
                        shift_gain(event.gain);
                        shift_gain(event.pan_gain);
                    }
                for (auto& stream : s.streams)
                    if (stream) {
                        if (stream->identity.Ended) stream->identity.Ended += elapsed;
                        shift_gain(stream->volume);
                    }
            }
            s.suspended = suspended;
        }
    }
}
namespace {
// Host-frame upkeep. It runs once per host update, whatever number of logic
// iterations the frame contained, without advancing scenario logic.
void maintain_audio_session(AudioSession& s, bool paused) noexcept {
    suspend_audio_session(s, paused);
    s.collect();
    // Desired parameters remain dirty until accepted by the consumer queue.
    // Retry even while paused and independently of the 33 ms event scheduler.
    for (auto& event : s.events)
        if (event.stopping)
            event.Release();
        else if (event.alive && event.playback.parameters_dirty)
            s.parameters(event.playback);
    for (auto& stream : s.streams)
        if (stream) {
            if (stream->playback.parameters_dirty) s.parameters(stream->playback);
            if (stream->playback.pause_dirty) s.pause(stream->playback, stream->playback.paused);
        }
    // Stream refills follow the host's output rate, not the logic rate, and so stay
    // with the host update rather than the audio AI.
    if (!s.suspended) {
        const auto now = Audio::GetTime();
        for (auto& stream : s.streams)
            if (stream) {
                if (stream->destroyed) {
                    if (s.halt(stream->playback)) stream.reset();
                } else
                    stream->identity.Update(now);
            }
    }
}
}
void update_audio_session(AudioSession& s, bool focused, bool paused) noexcept {
    if (active != &s || s.updating) return;
    // Focus belongs to host input policy; background simulation retains audio.
    (void)focused;
    maintain_audio_session(s, paused);
    Audio::AI();
}

void suspend_native_audio(bool suspended) noexcept {
    if (active) suspend_audio_session(*active, suspended);
}
void update_native_audio(bool focused, bool paused) noexcept {
    // Focus belongs to host input policy; background simulation retains audio.
    (void)focused;
    if (active && !active->updating) maintain_audio_session(*active, paused);
}
void run_native_audio_ai() noexcept {
    if (active) Audio::AI();
}
void mix_audio_session(AudioSession& s, float* output, unsigned frames) noexcept { s.mixer.mix(output, frames); }

}
