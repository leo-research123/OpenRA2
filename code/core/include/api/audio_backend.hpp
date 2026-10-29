#pragma once

namespace game {

class ResourceHandle;
struct AudioSession;
struct AudioStatistics {
    unsigned active_events = 0;
    unsigned active_streams = 0;
    unsigned sound_definitions = 0;
    unsigned missing_samples = 0;
    unsigned decode_failures = 0;
    unsigned command_overflows = 0;
    unsigned underruns = 0;
    bool resources_ready = false;
};

// Native PCM provider. The owner serializes creation, loading, updates and
// destruction with all game operations. A single session may be installed.
// Creation installs the session but does not open resources or start a thread.
// The host must stop and join its mixer before destruction. False preserves
// output; no exception crosses these boundaries. The resource handle is borrowed
// until destruction and must outlive the session and every sound controller.
bool create_audio_session(unsigned output_rate, AudioSession*& output) noexcept;
bool load_audio_session(AudioSession&, ResourceHandle&) noexcept;
void stop_audio_session(AudioSession&) noexcept;
void destroy_audio_session(AudioSession*&) noexcept;
// Focus does not suspend native output. Paused controls explicit suspension;
// map hosts supply their combined host and scenario pause state.
void update_audio_session(AudioSession&, bool focused, bool paused) noexcept;
// Immediate output suspension for hosts whose update callback stops on pause.
// This operation does not read resources or run game callbacks.
void suspend_audio_session(AudioSession&, bool suspended) noexcept;
bool set_audio_levels(AudioSession&, float effects, float speech, float music) noexcept;
bool get_audio_levels(const AudioSession&, float& effects, float& speech, float& music) noexcept;
bool get_audio_statistics(const AudioSession&, AudioStatistics& output) noexcept;
const char* audio_session_error(const AudioSession&) noexcept;

// One audio consumer, concurrent with the serialized owner above. Interleaved
// stereo float output is borrowed only for this call. This path performs no
// allocation, file access, game callbacks or blocking synchronization. It does
// not advance the authoritative game clock. Unavailable data produces silence.
void mix_audio_session(AudioSession&, float* stereo, unsigned frames) noexcept;

}
