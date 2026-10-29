#include "audio_playlist.hpp"
#include "audio_mixer.hpp"
#include "yrpp/Randomizer.h"

namespace game {
namespace {
int random_range(void*, int low, int high) { return Randomizer::Global.RandomRanged(low, high); }
}
bool AudioEventPlaylist::fill() noexcept {
    if (eof.load(std::memory_order_relaxed)) return true;
    if (!event || !event->type || !rate || !(pitch > 0)) return false;
    const auto begin = consumed.load(std::memory_order_acquire);
    auto end = written.load(std::memory_order_relaxed);
    double queued = 0;
    // The active descriptor may be almost exhausted. Only future descriptors
    // count toward lookahead, so a long attack cannot starve short body samples.
    for (auto i = begin + 1; i < end; ++i) {
        const auto* source = queue[i % capacity].source;
        queued += double(source->pcm.size()) / source->rate;
    }
    // Keep roughly the former PCM-ring lookahead, plus an interpolation
    // neighbor. Very short samples are bounded by descriptor capacity.
    const double horizon = double(audio_ring_capacity) * pitch / rate;
    while (end - begin < capacity && (end - begin < 2 || queued < horizon)) {
        const auto previous = event->Flags;
        int slot = -1;
        const bool selected = started ? event->NextSample(random ? random : random_range, random_context, slot)
                                      : event->FirstSample(random ? random : random_range, random_context, slot);
        started = true;
        event->Flags |= 8;
        if (!selected) {
            eof.store(true, std::memory_order_release);
            break;
        }
        const auto* source = parts[unsigned(slot)].get();
        if (!source || source->pcm.empty() || !source->rate) return false;
        if (!(previous & 0x10) && (event->Flags & 0x10)) decay_part = end;
        queue[end % capacity] = {source, slot};
        if (end != begin) queued += double(source->pcm.size()) / source->rate;
        written.store(++end, std::memory_order_release);
    }
    return true;
}
}
