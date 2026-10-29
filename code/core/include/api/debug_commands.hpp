#pragma once

#include "api/map_objects.hpp"
#include <cstdint>

namespace game {

enum class DebugCommandId : std::uint32_t {
    none = 0,
    power = 1,
    level = 2,
    hp = 3,
    life = hp,
    fog = 4,
    money = 5,
    tp = 6,
    god = 7,
    gamespeed = 8,
    xp = 9,
    atk = 10,
    select = 11,
    control = 12,
    perf = 13,
    radar = 14,
    mindcontrol = 15
};
enum class DebugCommandStatus : std::uint32_t {
    success = 0,
    partial_success = 1,
    invalid_command = 2,
    invalid_argument = 3,
    map_not_ready = 4,
    stale_session = 5,
    no_targets = 6,
    resource_failure = 7,
    execution_failure = 8
};
enum class DebugArgumentKind : std::uint32_t {
    none = 0,
    integer = 1,
    real = 2,
    optional_integer = 3,
    integer_or_reset = 4
};
enum class DebugTargetFailure : std::uint32_t {
    none = 0,
    unsupported_type = 1,
    dead_object = 2,
    expired_object = 3,
    unavailable_player = 4,
    exception = 5,
    invalid_state = 6,
    blocked_destination = 7,
    value_out_of_range = 8
};

struct DebugCommandDescriptor {
    DebugCommandId id = DebugCommandId::none;
    DebugArgumentKind argument = DebugArgumentKind::none;
    std::int32_t minimum = 0, maximum = 0;
    char name[24]{}, usage[48]{}, description[128]{}, success_message[128]{};
};

struct DebugCommandResult {
    std::uint64_t request = 0, world = 0;
    DebugCommandId command = DebugCommandId::none;
    std::int32_t frame = -1, value = 0;
    std::uint32_t candidates = 0, applied = 0, skipped = 0;
    DebugTargetFailure first_failure = DebugTargetFailure::none;
    MapObjectId first_failed_target{};
};

// Additional feedback uses a separate query to preserve the existing result ABI.
struct DebugCommandFeedback {
    std::uint64_t request = 0;
    double number = 0;
    char message[2048]{};
};

// Host measurements are CPU durations, never GPU completion times. The world
// token prevents render-thread results from leaking into a replacement session.
struct DebugHostPerformance {
    std::uint64_t sequence = 0;
    double frame_seconds = 0, render_cpu_ms = 0;
};
struct DebugModes {
    bool fog_visible = true, teleport = false;
    std::int32_t selection = 1, control = 1;
};
struct DebugPerformanceSnapshot {
    bool visible = false;
    char message[1024]{};
};
// Read-only, session-bound overlay query. Failure leaves output unchanged;
// no exception may cross the core/host boundary.
bool get_debug_performance(MapViewHandle&, std::uint64_t expected_world, DebugPerformanceSnapshot& output) noexcept;
bool get_debug_modes(const MapViewHandle&, DebugModes& output) noexcept;
bool get_debug_command_feedback(MapViewHandle&, std::uint64_t request, DebugCommandFeedback& output) noexcept;
bool record_debug_host_performance(MapViewHandle&, std::uint64_t expected_world, const DebugHostPerformance&) noexcept;

inline constexpr std::uint32_t debug_command_max_bytes = 512;

// Immutable metadata; strings are NUL-terminated UTF-8 with English fallbacks
// for host localization. Invalid indexes leave output unchanged. Returned status
// and failure names are borrowed static strings, valid for the module lifetime.
std::uint32_t debug_command_count() noexcept;
bool get_debug_command_descriptor(std::uint32_t index, DebugCommandDescriptor& output) noexcept;
const char* debug_command_status_name(DebugCommandStatus) noexcept;
const char* debug_target_failure_name(DebugTargetFailure) noexcept;

// The host serializes this synchronous operation with loading, logic and teardown.
// A null view records and rejects unbound requests as map_not_ready.
// expected_world must be a nonzero token obtained from get_debug_command_world.
// Parsing accepts ASCII case-insensitive names and space/tab/CR/LF separators.
// Text is length-delimited; embedded NUL, overflow and extra arguments are rejected.
// Output is always initialized. Preflight failure has no game-state side effects.
// Targets are fixed before writes using the existing IsSelected semantics.
// Partial application is reported, never rolled back; applied counts completed
// operations (including idempotent writes), skipped counts all other candidates.
// An interrupted target can already have side effects; failure does not undo them.
// hp 0 retires original objects through the debug path, not combat damage.
// Recoverable exceptions are contained here. Original callbacks which terminate
// on failure retain that contract and cannot be translated into a status.
DebugCommandStatus execute_debug_command(MapViewHandle*, std::uint64_t expected_world, const char* text,
                                         std::uint32_t text_length, DebugCommandResult& output) noexcept;
bool get_debug_command_world(MapViewHandle&, std::uint64_t& output) noexcept;

}
