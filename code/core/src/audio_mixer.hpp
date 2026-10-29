#pragma once
#include "audio_decoder.hpp"
#include "audio_playlist.hpp"
#include "yrpp/RawFileClass.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace game {
// Device parameters emitted by the original 0x40A6D0 DirectSound adapter.
int audio_device_volume_db(unsigned volume) noexcept;
int audio_device_pan_db(unsigned pan) noexcept;
void audio_device_gains(float volume, float pan, float& left, float& right) noexcept;
constexpr unsigned audio_voice_capacity = 320;
constexpr unsigned audio_ring_capacity = 65536;
struct AudioSource {
    unsigned rate = 0;
    bool streaming = false;
    std::vector<AudioPCMFrame> pcm;
    std::unique_ptr<RawFileClass> file;
    AudioDecoder decoder;
    std::unique_ptr<AudioEventPlaylist> sequence;
    std::atomic<std::uint64_t> read{0}, written{0};
    std::atomic<bool> eof{false};
    bool fill() noexcept;
};
enum class AudioMixOperation { start, stop, parameters, pause, suspend };
struct AudioMixCommand {
    AudioMixOperation operation = AudioMixOperation::stop;
    unsigned slot = 0, stamp = 0;
    AudioSource* source = nullptr;
    float gain = 1, pan = 0.5f, pitch = 1;
    bool flag = false;
};
class AudioMixer {
public:
    explicit AudioMixer(unsigned rate) noexcept : rate_(rate) {}
    bool submit(const AudioMixCommand&, std::uint64_t& sequence) noexcept;
    void mix(float* output, unsigned frames) noexcept;
    std::uint64_t consumed() const noexcept { return head_.load(std::memory_order_acquire); }
    unsigned finished(unsigned slot) const noexcept { return finished_[slot].load(std::memory_order_acquire); }
    unsigned underruns() const noexcept { return underruns_.load(std::memory_order_relaxed); }
    unsigned output_rate() const noexcept { return rate_; }

private:
    struct Voice {
        AudioSource* source = nullptr;
        unsigned stamp = 0;
        std::uint64_t cursor = 0;
        std::uint64_t part = 0;
        double fraction = 0;
        float gain = 1, pan = 0.5f, pitch = 1;
        float left = 1, right = 1;
        bool paused = false, loop = false;
    };
    static constexpr unsigned capacity = 8192;
    unsigned rate_;
    bool suspended_ = false;
    std::array<AudioMixCommand, capacity> commands_{};
    std::atomic<std::uint64_t> head_{0}, tail_{0};
    std::array<Voice, audio_voice_capacity> voices_{};
    std::array<std::atomic<unsigned>, audio_voice_capacity> finished_{};
    std::atomic<unsigned> underruns_{0};
};
}
