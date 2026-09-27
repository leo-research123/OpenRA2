#pragma once

class AudioStream;
struct AudioController;
class VocClass;
struct AudioVolumeControl;

namespace game {

// Non-owning identities. These tags declare no event/playback/buffer layout;
// only the backend interprets them. A null event handle means no event.
struct AudioEventTag;
struct AudioPlaybackTag;
struct AudioBufferTag;
using AudioEventHandle = AudioEventTag*;
using AudioPlaybackHandle = AudioPlaybackTag*;
using AudioBufferHandle = AudioBufferTag*;

// Host execution interface for the existing Audio.h module. Implement this
// interface in a host class and keep host state in that class's own members.
// Original AudioStream/AudioController objects retain their existing layouts.
// AUD support and the legacy TEXT1/2/3.AUD score path are intentionally out of
// scope; do not add AUD loading, decoding or playback to this interface.
class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    // Stream playback. Filename is borrowed until return; false means no start.
    // resource_file=true uses resource lookup; false tries a raw file first.
    // The host must provide valid stream storage for its selected backend.
    virtual bool play_wav(AudioStream& stream, const char* filename,
        bool resource_file) noexcept = 0;

    // VocClass::PlayGlobal device/event boundary. An unavailable sound device
    // returns false, as in the original; arguments retain original units.
    virtual bool play_global(int index, int panning, float volume,
        AudioController* controller) noexcept { return false; }

    // Detach the controller from its event without freeing controller storage.
    virtual void destroy_controller(AudioController& controller) noexcept = 0;
    // Stop immediately, or allow the event to fade out.
    virtual void stop(AudioController& controller) noexcept = 0;
    virtual void end(AudioController& controller) noexcept = 0;
    // End looping immediately, or allow its fade-out behavior.
    virtual void stop_looping(AudioController& controller) noexcept = 0;
    virtual void end_looping(AudioController& controller) noexcept = 0;

    // The host maintains Event, EventType and Stamp in the original controller.
    virtual void set_event(AudioController& controller,
        AudioEventHandle event, VocClass* event_type) noexcept = 0;
    // All queries leave output unchanged on a miss.
    virtual bool get_event(AudioController& controller,
        AudioEventHandle& output) noexcept = 0;
    virtual bool get_controller_type(AudioController& controller,
        VocClass*& output) noexcept = 0;

    // Event operations. Volume and panning use original, unnormalized units.
    virtual void set_volume(AudioEventHandle event, unsigned int volume) noexcept = 0;
    virtual void set_panning(AudioEventHandle event, unsigned int pan) noexcept = 0;
    virtual bool get_event_type(AudioEventHandle event, VocClass*& output) noexcept = 0;

    // Low-level playback resources, owned and interpreted by the host backend.
    virtual void stop_playback(AudioPlaybackHandle playback) noexcept = 0;
    virtual void release_buffer(AudioBufferHandle buffer) noexcept = 0;

    // Original stream-device operations. All time values use Audio::GetTime's
    // millisecond basis; native providers must not introduce a platform clock.
    // Defaults describe a provider with no stream device. Failed queries and
    // creation preserve output. The provider owns/destroys stream identities.
    virtual bool stream_device_available() noexcept { return false; }
    virtual bool create_stream(int buffer_size, int flags, AudioStream*& output) noexcept { return false; }
    virtual void destroy_stream(AudioStream& stream) noexcept { }
    virtual void name_stream(AudioStream& stream, const char* name) noexcept { }
    virtual void bind_stream_volume(AudioStream& stream, AudioVolumeControl* control, bool secondary) noexcept { }
    virtual void stop_stream(AudioStream& stream) noexcept { }
    virtual void pause_stream(AudioStream& stream) noexcept { }
    virtual void resume_stream(AudioStream& stream) noexcept { }
    virtual bool stream_is_playing(const AudioStream& stream, bool& output) noexcept { return false; }
    virtual bool stream_end_time(const AudioStream& stream, unsigned long long& output) noexcept { return false; }
};

// Borrow a backend; core never copies or deletes it. Keep it alive until reset
// or replacement. Register/reset on the game thread after all calls, controller
// lifetimes, playback and worker callbacks using the previous backend have ended.
void set_audio_backend(AudioBackend& backend) noexcept;
// Restore the link-selected default: unavailable in standalone core, YR in compat.
void reset_audio_backend() noexcept;
// Borrow the active backend. If unavailable, return false and preserve output.
bool get_audio_backend(AudioBackend*& output) noexcept;

}
