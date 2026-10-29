#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include "audio_decoder.hpp"
#include "yrpp/VocClass.h"

namespace game {
using AudioRandomRange = AudioEvent::RandomRange;

struct AudioSource;
// Single owner/consumer playlist. Only the owner selects samples and retains
// their storage; the callback reads immutable PCM through published descriptors.
struct AudioEventPlaylist {
    static constexpr unsigned capacity = 64;
    struct Part {
        const AudioSource* source = nullptr;
        int slot = -1;
    };
    AudioEvent* event = nullptr;
    std::array<std::shared_ptr<AudioSource>, 32> parts{};
    std::array<Part, capacity> queue{};
    std::atomic<std::uint64_t> consumed{0}, written{0};
    std::atomic<bool> eof{false};
    unsigned rate = 0;
    float pitch = 1;
    AudioRandomRange random = nullptr;
    void* random_context = nullptr;
    // Owner-only index of the first decay descriptor, or no scheduled decay.
    std::uint64_t decay_part = UINT64_MAX;
    bool fill() noexcept;

private:
    bool started = false;
};
}
