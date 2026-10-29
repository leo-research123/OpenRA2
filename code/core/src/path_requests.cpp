#include "path_requests.hpp"
#include "path_snapshot.hpp"
#include "yrpp/FootClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <unordered_map>
#include <list>
#include <cstdlib>
#include <array>
#include <chrono>

#if !defined(RA2_YRPP_GAME) && !defined(RA2_IMAGE_GAME)
namespace game {
static constexpr unsigned minimum_replan_frames = 4;
static constexpr unsigned maximum_failures = 4;
struct PathRequestState {
    CellStruct destination{};
    unsigned frame = 0, delay = 0, failures = 0;
    bool started = false, pending = false, failed_result = false, blocked_retry = false, blocked_retry_straight = false;
};
static std::unordered_map<const FootClass*, PathRequestState> requests;
static PathRequestCounters counters;
struct ScheduledPath {
    FootClass* foot;
    AbstractClass* order;
    CoordStruct order_position;
    CellStruct destination;
    int offset, mode;
    Mission mission;
    unsigned submitted;
    int prefix[24];
    int snapshot = -1;
};
static std::list<ScheduledPath> queue;
static std::unordered_map<FootClass*, std::list<ScheduledPath>::iterator> queued;
static FootClass* executing = nullptr;
static bool serviced_frame = false;
static unsigned last_service_frame = 0;
static unsigned worker_count = 4;
static bool budget_deferred = false;
bool path_budget_deferred() noexcept { return budget_deferred; }
bool admit_serial_path() noexcept {
    if (!executing) return true;
    if (counters.frame_serial < serial_paths_per_frame) {
        ++counters.frame_serial;
        return true;
    }
    budget_deferred = true;
    return false;
}
void set_path_worker_count(unsigned count) noexcept { worker_count = std::clamp(count, 1u, 4u); }
static std::array<PathSnapshot, path_requests_per_frame> snapshots;
static std::array<PathSnapshotResult, path_requests_per_frame> snapshot_results;
static std::array<FootClass*, path_requests_per_frame> snapshot_feet;
static std::array<int, path_requests_per_frame> snapshot_ids;

void cancel_scheduled_path(FootClass& foot) noexcept {
    const auto state = requests.find(&foot);
    if (state != requests.end()) {
        state->second.failed_result = false;
        if (executing != &foot) state->second.pending = false;
    }
    // The executing request owns its node until the result is committed.
    // Its normal destination substitution must not drop budget-deferred work.
    if (executing == &foot) return;
    if (state != requests.end()) state->second.blocked_retry = false;
    const auto found = queued.find(&foot);
    if (found == queued.end()) return;
    queue.erase(found->second);
    queued.erase(found);
    ++counters.cancelled;
    counters.queued = queue.size();
}
PathRequestAdmission schedule_path_request(FootClass& foot, CellStruct destination, int offset, int mode) noexcept {
    if (executing == &foot) return PathRequestAdmission::Execute;
    const auto state = requests.find(&foot);
    if (state != requests.end() && state->second.failed_result) {
        state->second.failed_result = false;
        if (state->second.destination == destination) return PathRequestAdmission::Failed;
    }
    const auto found = queued.find(&foot);
    if (found != queued.end()) {
        // Retain queue position on repeated requests. New commands cancel explicitly.
        found->second->destination = destination;
        found->second->offset = offset;
        found->second->mode = mode;
        std::copy_n(foot.PathDirections, offset, found->second->prefix);
    } else {
        try {
            ScheduledPath request{&foot, foot.Destination,
                foot.Destination ? foot.Destination->GetDestination(&foot) : CoordStruct::Empty,
                destination, offset, mode, foot.CurrentMission, unsigned(Unsorted::CurrentFrame), {}};
            std::copy_n(foot.PathDirections, offset, request.prefix);
            queue.push_back(request);
            try {
                queued.emplace(&foot, std::prev(queue.end()));
            } catch (...) {
                queue.pop_back();
                throw;
            }
            ++counters.submitted;
            counters.queued = queue.size();
            counters.peak_queued = std::max(counters.peak_queued, counters.queued);
        } catch (...) {
            // Keep the command pending; allocation failure must not bypass the frame cap.
            try { requests[&foot].pending = true; } catch (...) { std::abort(); }
        }
    }
    foot.PathDelayTimer.Start(1);
    return PathRequestAdmission::Pending;
}
void service_path_requests() noexcept {
    const auto frame = unsigned(Unsorted::CurrentFrame);
    if (serviced_frame && last_service_frame == frame) return;
    serviced_frame = true;
    last_service_frame = frame;
    counters.frame_serviced = 0;
    counters.frame_serial = 0;
    counters.frame_budget_deferred = 0;
    // Bound queue inspection too. New requests and recursive follower requests
    // cannot run in this batch. Rotation is based on counts, never wall time.
    const auto visits = std::min(queue.size(), std::size_t(path_requests_per_frame * 4));
    unsigned prepared = 0, admitted = 0;
    const auto capture_started = std::chrono::steady_clock::now();
    std::size_t inspected = 0;
    for (auto& request : queue) {
        if (inspected++ == visits || admitted == path_requests_per_frame) break;
        request.snapshot = -1;
        auto& foot = *request.foot;
        const auto throttle = requests.find(&foot);
        const auto delay = throttle == requests.end() ? 0u :
            throttle->second.destination == request.destination ? throttle->second.delay :
            std::min(throttle->second.delay, minimum_replan_frames);
        if (request.submitted == frame || (throttle != requests.end() && throttle->second.started &&
            frame - throttle->second.frame < delay)) continue;
        if (!foot.IsAlive || foot.InLimbo || foot.Destination != request.order || foot.CurrentMission != request.mission ||
            (foot.Destination && foot.Destination->GetDestination(&foot) != request.order_position) ||
            !std::equal(request.prefix, request.prefix + request.offset, foot.PathDirections)) continue;
        ++admitted;
        if (capture_path_snapshot(foot, request.destination, request.offset, request.mode, snapshots[prepared])) {
            request.snapshot = int(prepared);
            snapshot_feet[prepared] = &foot;
            snapshot_ids[prepared] = foot.UniqueID;
            ++prepared;
        }
    }
    const auto workers_started = std::chrono::steady_clock::now();
    if (prepared) solve_path_snapshot_batch(snapshots.data(), snapshot_results.data(), prepared, worker_count);
    const auto workers_finished = std::chrono::steady_clock::now();
    counters.snapshot_capture_us += std::chrono::duration_cast<std::chrono::microseconds>(workers_started - capture_started).count();
    counters.snapshot_worker_us += std::chrono::duration_cast<std::chrono::microseconds>(workers_finished - workers_started).count();
    counters.snapshot_prepared += prepared;
    for (unsigned i = 0; i < prepared; ++i) {
        counters.snapshot_solved += snapshot_results[i].found;
        counters.snapshot_expanded += snapshot_results[i].expanded;
    }
    unsigned attempted = 0;
    for (std::size_t i = 0; i < visits && !queue.empty() && attempted < path_requests_per_frame; ++i) {
        const auto request = queue.front();
        auto& foot = *request.foot;
        const auto throttle = requests.find(&foot);
        const auto delay = throttle == requests.end() ? 0u :
            throttle->second.destination == request.destination ? throttle->second.delay :
            std::min(throttle->second.delay, minimum_replan_frames);
        const bool waiting = throttle != requests.end() && throttle->second.started &&
                             frame - throttle->second.frame < delay;
        if (request.submitted == frame || waiting) {
            queue.splice(queue.end(), queue, queue.begin());
            continue;
        }
        if (!foot.IsAlive || foot.InLimbo || foot.Destination != request.order || foot.CurrentMission != request.mission ||
            (foot.Destination && foot.Destination->GetDestination(&foot) != request.order_position) ||
            !std::equal(request.prefix, request.prefix + request.offset, foot.PathDirections)) {
            cancel_scheduled_path(foot);
            foot.PathDelayTimer.Start(0);
            continue;
        }
        ++attempted;
        const int snapshot = request.snapshot;
        const bool ready = snapshot >= 0 && unsigned(snapshot) < prepared &&
            snapshot_feet[snapshot] == &foot && snapshot_ids[snapshot] == foot.UniqueID &&
            snapshot_results[snapshot].found;
        if (!ready && counters.frame_serial == serial_paths_per_frame) {
            ++counters.frame_budget_deferred;
            queue.splice(queue.end(), queue, queue.begin());
            continue;
        }
        const bool had_state = throttle != requests.end();
        const auto previous_state = had_state ? throttle->second : PathRequestState{};
        int previous_path[24];
        unsigned previous_length = 0;
        do {
            previous_path[previous_length] = foot.PathDirections[previous_length];
        } while (previous_path[previous_length++] != -1 && previous_length < 24);
        executing = &foot;
        budget_deferred = false;
        if (ready) activate_path_snapshot(&foot, &snapshots[snapshot], &snapshot_results[snapshot]);
        try {
            const bool success = foot.UpdatePathfinding(request.destination, request.offset, request.mode);
            if (budget_deferred) {
                // Budget exhaustion is not a navigation failure or a new search.
                auto state = requests.find(&foot);
                if (state != requests.end()) {
                    state->second = previous_state;
                    state->second.pending = true;
                }
                --counters.searches;
                ++counters.frame_budget_deferred;
                const auto node = queued.find(&foot);
                if (node != queued.end()) {
                    auto& pending = *node->second;
                    pending.order = foot.Destination;
                    pending.order_position = foot.Destination ? foot.Destination->GetDestination(&foot) : CoordStruct::Empty;
                    if (pending.order != request.order && foot.Destination)
                        pending.destination = CellClass::Coord2Cell(pending.order_position);
                    std::copy_n(previous_path, previous_length, foot.PathDirections);
                    queue.splice(queue.end(), queue, node->second);
                }
            } else {
                ++counters.frame_serviced;
                ++counters.serviced;
                const auto age = frame - request.submitted;
                counters.serviced_wait_frames += age;
                counters.maximum_wait_frames = std::max(counters.maximum_wait_frames, age);
                const auto node = queued.find(&foot);
                if (node != queued.end()) { queue.erase(node->second); queued.erase(node); }
                counters.queued = queue.size();
            }
            if (success) {
                foot.PathWaitTimes = 10;
                // Walking normally runs this handoff when a synchronous path arrives.
                if (foot.WhatAmI() == AbstractType::Infantry) foot.vt_entry_4F8();
            }
            const auto state = requests.find(&foot);
            if (!success && state != requests.end() && !state->second.pending)
                state->second.failed_result = true;
        } catch (...) {
            // Original movement callbacks have no recoverable exception contract.
            std::abort();
        }
        executing = nullptr;
        budget_deferred = false;
        activate_path_snapshot(nullptr, nullptr, nullptr);
    }
}
bool begin_path_request(FootClass& foot, CellStruct destination) noexcept {
    try {
        auto& state = requests[&foot];
        const auto frame = unsigned(Unsorted::CurrentFrame);
        if (destination != state.destination) {
            state.destination = destination;
            state.failures = 0;
            // A changed order resets failure backoff, but not the minimum interval.
            state.delay = std::min(state.delay, minimum_replan_frames);
        }
        const auto elapsed = frame - state.frame;
        if (state.started && elapsed < state.delay) {
            state.pending = true;
            foot.PathDelayTimer.Start(int(state.delay - elapsed));
            ++counters.deferred;
            return false;
        }
        state.started = true;
        state.pending = false;
        state.frame = frame;
        state.delay = minimum_replan_frames;
    } catch (...) {
        // Allocation failure preserves synchronous navigation semantics.
        requests.erase(&foot);
    }
    ++counters.searches;
    return true;
}
bool finish_path_request(FootClass& foot, bool success) noexcept {
    const auto found = requests.find(&foot);
    if (found == requests.end()) return false;
    auto& state = found->second;
    if (success) {
        state.failures = 0;
        state.pending = false;
        return false;
    }
    ++counters.failures;
    state.failures = std::min(state.failures + 1, maximum_failures);
    state.delay = minimum_replan_frames << state.failures;
    state.pending = state.failures < maximum_failures;
    foot.PathDelayTimer.Start(int(state.delay));
    return state.pending;
}
bool path_request_pending(const FootClass& foot) noexcept {
    if (queued.contains(const_cast<FootClass*>(&foot))) return true;
    const auto found = requests.find(&foot);
    return found != requests.end() && found->second.pending;
}
void forget_path_requests(FootClass& foot) noexcept {
    const auto found = queued.find(&foot);
    if (found != queued.end()) {
        queue.erase(found->second);
        queued.erase(found);
        ++counters.cancelled;
        counters.queued = queue.size();
    }
    requests.erase(&foot);
}
void defer_blocked_path_retry(FootClass& foot, bool forceStraight) noexcept {
    try {
        auto& state = requests[&foot];
        state.blocked_retry = true;
        state.blocked_retry_straight = forceStraight;
    } catch (...) { std::abort(); }
}
bool resume_blocked_path_retry(FootClass& foot, bool* forceStraight) noexcept {
    const auto state = requests.find(&foot);
    if (state == requests.end() || !state->second.blocked_retry || path_request_pending(foot)) return false;
    state->second.blocked_retry = false;
    if (forceStraight) *forceStraight = state->second.blocked_retry_straight;
    return true;
}
bool wait_for_moving_blocker(FootClass& foot) noexcept {
    if (!foot.IsWaitingBlockagePath) {
        foot.IsWaitingBlockagePath = true;
        foot.BlockagePathTimer.Start(int(minimum_replan_frames));
    }
    if (!foot.BlockagePathTimer.GetTimeLeft()) return false;
    ++counters.moving_waits;
    return true;
}
void read_path_request_counters(PathRequestCounters& out) noexcept {
    out = counters;
    out.snapshot_applied = path_snapshots_consumed();
    PathSnapshotCounters rejected;
    read_path_snapshot_counters(rejected);
    out.rejected_profile = rejected.profile;
    out.rejected_extent = rejected.extent;
    out.rejected_zone = rejected.zone;
    out.rejected_terrain = rejected.terrain;
    out.rejected_destination = rejected.destination;
}
}
#endif
