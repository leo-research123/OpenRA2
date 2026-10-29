// Event arbitration from 0x004041D0/0x00404E20; device selection from
// 0x004035F0/0x00405B50. Playback and PCM ownership belong to the caller.
#include "yrpp/Audio.h"
#include "yrpp/VocClass.h"
#include <algorithm>
#include <cstddef>

namespace {
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
auto& buckets = *reinterpret_cast<AudioLink (*)[70]>(0x87DE38);
auto& limit_pass = *reinterpret_cast<unsigned*>(0x87E2AC);
auto& priority_pass = *reinterpret_cast<unsigned*>(0x87E2B0);
#else
AudioLink buckets[70]{};
unsigned limit_pass = 0, priority_pass = 0;
#endif
void initialize(AudioLink& link) noexcept { link.Next = link.Previous = link.Sentinel = &link; }
void insert_before(AudioLink& position, AudioLink& link) noexcept {
    link.Next = &position;
    link.Previous = position.Previous;
    link.Sentinel = nullptr;
    position.Previous->Next = &link;
    position.Previous = &link;
}
void unlink(AudioLink& link) noexcept {
    link.Previous->Next = link.Next;
    link.Next->Previous = link.Previous;
    link.Next = link.Previous = &link;
}
AudioEvent& event_from_link(AudioLink* link) noexcept {
    return *reinterpret_cast<AudioEvent*>(reinterpret_cast<char*>(link) - offsetof(AudioEvent, DefinitionLink));
}
VocClass& type_from_link(AudioLink* link) noexcept {
    return *reinterpret_cast<VocClass*>(reinterpret_cast<char*>(link) - offsetof(VocClass, PriorityLink));
}
}

void Audio::EnforceEventLimits(AudioEvent* const* events, unsigned count) noexcept {
    if (!++limit_pass) {
        for (unsigned i = 0; i < count; ++i)
            if (events[i] && events[i]->type) events[i]->type->LimitPass = 0;
        ++limit_pass;
    }
    for (unsigned i = 0; i < count; ++i) {
        auto* event = events[i];
        if (!event || !event->type) continue;
        auto& type = *event->type;
        if (type.LimitPass != limit_pass) {
            initialize(type.EventList);
            type.EventCount = 0;
            type.LimitPass = limit_pass;
        }
        if (event->State == 4) continue;
        const unsigned volume = event->gain.GetCurrentVolume();
        const unsigned tolerance = unsigned(type.Volume.GetVolume()) / 10;
        auto* position = type.EventList.Next;
        for (; position != &type.EventList; position = position->Next) {
            const auto& old = event_from_link(position);
            const unsigned old_volume = old.gain.GetCurrentVolume();
            const unsigned difference = old_volume > volume ? old_volume - volume : volume - old_volume;
            if (difference >= tolerance) {
                if (old_volume < volume) break;
            } else if ((unsigned(type.Control) & 0x10) ? old.State != 0 : old.State == 0)
                break;
        }
        insert_before(*position, event->DefinitionLink);
        ++type.EventCount;
        if (type.Limit && int(type.EventCount) > type.Limit) {
            auto& victim = event_from_link(type.EventList.Previous);
            unlink(victim.DefinitionLink);
            victim.State = 4;
            --type.EventCount;
        }
    }
}

void Audio::ClearPriorityBuckets() noexcept {
    for (auto& bucket : buckets)
        initialize(bucket);
}

unsigned Audio::BuildPriorityBuckets(AudioEvent* const* events, unsigned count) noexcept {
    ClearPriorityBuckets();
    if (!++priority_pass) {
        for (unsigned i = 0; i < count; ++i)
            if (events[i] && events[i]->type) events[i]->type->PriorityPass = 0;
        ++priority_pass;
    }
    unsigned load = 0;
    for (unsigned i = 0; i < count; ++i) {
        const auto* event = events[i];
        if (!event || event->State == 4 || !event->type) continue;
        auto& type = *event->type;
        if (!event->PauseCount) load += (20u * unsigned(type.Volume.GetVolume())) >> 14;
        const int priority = int(type.Priority) + event->PriorityBias;
        const unsigned bucket = event->gain.GetCurrentVolume() / 1638;
        // Full volume occupies bucket 10, which aliases the next priority row.
        // Reject malformed native input outside the original seven-row table.
        if (priority < 0 || priority >= 7 || bucket >= 70u - unsigned(priority) * 10) continue;
        if (type.PriorityPass == priority_pass) {
            if (priority < type.EffectivePriority ||
                (priority == type.EffectivePriority && bucket <= type.VolumeBucket))
                continue;
            unlink(type.PriorityLink);
        } else
            type.PriorityPass = priority_pass;
        type.EffectivePriority = priority;
        type.VolumeBucket = bucket;
        insert_before(buckets[unsigned(priority) * 10 + bucket], type.PriorityLink);
    }
    return load > 100 ? 0x190000u / load : 0x4000u;
}

bool Audio::FindLowerPriority(int priority, VocClass*& output) noexcept {
#if defined(RA2_AUDIO_GAME) || defined(RA2_YRPP_GAME)
    auto* type = reinterpret_cast<VocClass*(YRPP_FASTCALL*)(int)>(0x404E20)(priority);
    if (!type) return false;
    output = type;
    return true;
#else
    const int end = std::clamp(priority, 0, 7) * 10;
    for (int i = 0; i < end; ++i)
        if (buckets[i].Next && buckets[i].Next != &buckets[i]) {
            output = &type_from_link(buckets[i].Next);
            return true;
        }
    return false;
#endif
}

void Audio::RemovePriority(VocClass& type) noexcept {
    if (type.PriorityLink.Next && type.PriorityLink.Previous) unlink(type.PriorityLink);
}

bool Audio::SelectVoice(unsigned count, void* context, ReadVoice read, int priority, unsigned& output) noexcept {
    if (!read) return false;
    unsigned playing = 0, idle = 0, level = 0xFFFFFFFFu;
    bool has_playing = false, has_idle = false;
    int playing_priority = 127, idle_priority = 127;
    for (unsigned i = 0; i < count; ++i) {
        unsigned flags = 0, candidate_level = 0;
        int candidate_priority = 0;
        if (!read(context, i, flags, candidate_level, candidate_priority)) return false;
        if (!(flags & 0x15)) {
            output = i;
            return true;
        }
        if (flags & 1) {
            if (candidate_priority < playing_priority ||
                (candidate_priority == playing_priority && candidate_level < level &&
                 level - candidate_level >= 0x666)) {
                playing = i;
                has_playing = true;
                playing_priority = candidate_priority;
                level = candidate_level;
            }
        } else if (candidate_priority < idle_priority) {
            idle = i;
            has_idle = true;
            idle_priority = candidate_priority;
        }
    }
    if (has_idle && idle_priority <= playing_priority) {
        if (idle_priority > priority) return false;
        output = idle;
        return true;
    }
    if (!has_playing || playing_priority > priority) return false;
    output = playing;
    return true;
}
