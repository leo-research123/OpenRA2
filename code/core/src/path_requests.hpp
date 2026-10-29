#pragma once
#include "yrpp/GeneralStructures.h"
#include <cstdint>
class FootClass;
namespace game {
struct PathRequestCounters {
    std::uint64_t searches = 0, deferred = 0, failures = 0, moving_waits = 0;
    std::uint64_t submitted = 0, serviced = 0, cancelled = 0, queued = 0, peak_queued = 0;
    std::uint64_t serviced_wait_frames = 0;
    unsigned maximum_wait_frames = 0;
    unsigned frame_serviced = 0;
    unsigned frame_serial = 0, frame_budget_deferred = 0;
    std::uint64_t snapshot_prepared = 0, snapshot_solved = 0, snapshot_expanded = 0;
    std::uint64_t snapshot_applied = 0;
    std::uint64_t snapshot_capture_us = 0, snapshot_worker_us = 0;
    std::uint64_t rejected_profile = 0, rejected_extent = 0, rejected_zone = 0, rejected_terrain = 0, rejected_destination = 0;
};
enum class PathRequestAdmission { Execute, Pending, Failed };
inline constexpr unsigned path_requests_per_frame = 512;
inline constexpr unsigned serial_paths_per_frame = 256;
// Internal scheduling state never crosses the original class ABI.
#if defined(RA2_YRPP_GAME) || defined(RA2_IMAGE_GAME)
inline bool begin_path_request(FootClass&, CellStruct) noexcept { return true; }
inline bool finish_path_request(FootClass&, bool) noexcept { return false; }
inline bool path_request_pending(const FootClass&) noexcept { return false; }
inline void forget_path_requests(FootClass&) noexcept {}
inline bool wait_for_moving_blocker(FootClass&) noexcept { return false; }
inline void read_path_request_counters(PathRequestCounters& out) noexcept { out = {}; }
inline PathRequestAdmission schedule_path_request(FootClass&, CellStruct, int, int) noexcept {
    return PathRequestAdmission::Execute;
}
inline void service_path_requests() noexcept {}
inline void cancel_scheduled_path(FootClass&) noexcept {}
inline void defer_blocked_path_retry(FootClass&, bool = false) noexcept {}
inline bool resume_blocked_path_retry(FootClass&, bool* = nullptr) noexcept { return false; }
inline void set_path_worker_count(unsigned) noexcept {}
inline bool admit_serial_path() noexcept { return true; }
inline bool path_budget_deferred() noexcept { return false; }
#else
bool admit_serial_path() noexcept;
bool path_budget_deferred() noexcept;
void set_path_worker_count(unsigned) noexcept;
PathRequestAdmission schedule_path_request(FootClass&, CellStruct, int offset, int mode) noexcept;
void service_path_requests() noexcept;
void cancel_scheduled_path(FootClass&) noexcept;
// Preserve the second pass of an original synchronous blocked-movement retry
// until its scheduled path request completes. New orders cancel the continuation.
void defer_blocked_path_retry(FootClass&, bool = false) noexcept;
bool resume_blocked_path_retry(FootClass&, bool* = nullptr) noexcept;
bool begin_path_request(FootClass&, CellStruct destination) noexcept;
bool finish_path_request(FootClass&, bool success) noexcept;
bool path_request_pending(const FootClass&) noexcept;
void forget_path_requests(FootClass&) noexcept;
bool wait_for_moving_blocker(FootClass&) noexcept;
void read_path_request_counters(PathRequestCounters&) noexcept;
#endif
}
