#pragma once
// Private renderer storage. Allocations survive queued/coalesced frames through
// their owners; an address is reused only after the last owner releases it.
#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace render {
class DrawResourcePool {
    struct Range {
        std::uint32_t first, count;
    };
public:
    struct Statistics {
        std::uint32_t free_words = 0, largest_hole = 0, free_slots = 0;
        std::uint64_t releases = 0, released_words = 0;
    };
private:
    struct State {
        std::mutex mutex;
        std::vector<Range> holes;
        std::vector<std::uint32_t> slots;
        std::uint64_t serial = 0, releases = 0, released_words = 0;
        std::uint32_t next_slot = 0, max_slots = 0;
        State(std::uint32_t words, std::uint32_t descriptors) : max_slots(descriptors) {
            // At most one hole per live allocation plus the tail. Release
            // never allocates, including on the render thread.
            holes.reserve(std::size_t(descriptors) + 1);
            slots.reserve(descriptors);
            holes.push_back({0, words});
        }
        void release(std::uint32_t slot, Range range) noexcept {
            const std::lock_guard lock(mutex);
            ++releases;
            released_words += range.count;
            slots.push_back(slot);
            auto at = std::lower_bound(holes.begin(), holes.end(), range.first,
                                       [](const Range& a, std::uint32_t first) { return a.first < first; });
            at = holes.insert(at, range);
            if (at != holes.begin() && (at - 1)->first + (at - 1)->count == at->first) {
                (at - 1)->count += at->count;
                at = holes.erase(at) - 1;
            }
            if (at + 1 != holes.end() && at->first + at->count == (at + 1)->first) {
                at->count += (at + 1)->count;
                holes.erase(at + 1);
            }
        }
    };

public:
    static constexpr std::uint32_t PageSide = 1024, PageWords = PageSide * PageSide;
    static constexpr std::uint32_t DefaultWords = 32 * PageWords, DefaultSlots = 65536;
    struct Allocation {
        std::uint64_t version = 0;
        std::uint32_t slot = 0, first = 0, count = 0;
        Allocation(const Allocation&) = delete;
        Allocation& operator=(const Allocation&) = delete;
        ~Allocation() {
            if (state) state->release(slot, {first, count});
        }

    private:
        friend class DrawResourcePool;
        Allocation() = default;
        std::shared_ptr<State> state;
    };
    explicit DrawResourcePool(std::uint32_t words = DefaultWords, std::uint32_t slots = DefaultSlots)
        : state_(std::make_shared<State>(words, slots)) {}
    std::shared_ptr<const Allocation> allocate(std::uint32_t words) {
        if (!words) throw std::invalid_argument("empty drawing resource");
        // Allocate ownership before mutating the free list.
        std::shared_ptr<Allocation> out(new Allocation);
        auto& state = *state_;
        const std::lock_guard lock(state.mutex);
        auto at = std::find_if(state.holes.begin(), state.holes.end(),
                               [words](const Range& range) { return range.count >= words; });
        if (at == state.holes.end() || (state.slots.empty() && state.next_slot == state.max_slots))
            throw std::length_error("drawing resource budget exhausted");
        out->first = at->first;
        out->count = words;
        out->slot = state.slots.empty() ? state.next_slot++ : state.slots.back();
        if (!state.slots.empty()) state.slots.pop_back();
        out->version = ++state.serial;
        out->state = state_;
        at->first += words;
        at->count -= words;
        if (!at->count) state.holes.erase(at);
        return out;
    }
    Statistics statistics() const {
        const std::lock_guard lock(state_->mutex);
        Statistics out;
        out.free_slots = std::uint32_t(state_->slots.size()) + state_->max_slots - state_->next_slot;
        out.releases = state_->releases;
        out.released_words = state_->released_words;
        for (const auto& hole : state_->holes) {
            out.free_words += hole.count;
            out.largest_hole = std::max(out.largest_hole, hole.count);
        }
        return out;
    }
    // Words not allocated, in all holes.
    std::uint32_t free_words() const {
        const std::lock_guard lock(state_->mutex);
        std::uint32_t total = 0;
        for (const auto& hole : state_->holes)
            total += hole.count;
        return total;
    }

private:
    std::shared_ptr<State> state_;
};
}
