#pragma once
#include "api/audio_backend.hpp"
#include "api/movie_decoder.hpp"
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>
#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <memory>
#include <atomic>
#include <cstdint>

struct RA2AudioLink;
class RA2AudioPlayback : public godot::AudioStreamPlaybackResampled {
    GDCLASS(RA2AudioPlayback, godot::AudioStreamPlaybackResampled)
    friend class RA2AudioStream;
    std::shared_ptr<RA2AudioLink> link_;
    std::atomic<bool> playing_{false};

protected:
    static void _bind_methods() {}

public:
    void _start(double from_position) override;
    void _stop() override;
    bool _is_playing() const override;
    float _get_stream_sampling_rate() const override;
    int32_t _mix_resampled(godot::AudioFrame* buffer, int32_t frames) override;
};
class RA2AudioStream : public godot::AudioStream {
    GDCLASS(RA2AudioStream, godot::AudioStream)
    friend class RA2AudioOutput;
    std::shared_ptr<RA2AudioLink> link_;

protected:
    static void _bind_methods() {}

public:
    godot::Ref<godot::AudioStreamPlayback> _instantiate_playback() const override;
};
class RA2AudioOutput {
public:
    // The original 0x00406B10 device rate; Godot resamples the combined stream.
    static constexpr unsigned sample_rate = 22050;
    bool open(game::AudioSession&, game::MovieSession*) noexcept;
    std::uint64_t playback_id() const noexcept;
    ~RA2AudioOutput();

private:
    godot::ObjectID player_id_;
    godot::Ref<RA2AudioStream> stream_;
};
