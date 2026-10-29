#pragma once
#include <cstdint>
#include "api/drawing_diagnostics.hpp"

namespace game {
// Optional process-wide crash context. Paths are borrowed only during this call.
// A null path closes the current log. Failure disables logging; no exception
// crosses this boundary. Configure during host startup, before loading a map.
bool configure_runtime_log(const char* path, const char* resource_directory) noexcept;
// Signal ownership is explicitly requested by the host, never by loading the core.
// The previous host handlers are restored on shutdown and receive the fatal signal
// after the independent record is saved. Requires a configured runtime session.
bool configure_runtime_crash_capture(const char* reporter_path = nullptr) noexcept;
void stop_runtime_crash_capture() noexcept;
enum class RuntimePhase : std::uint32_t {
    idle,
    map_load,
    input,
    render,
    logic,
    events,
    cleanup,
    bridge_query,
    bridge_repair,
    trigger
};
// Copied scalar context only; the fatal handler never follows game pointers.
void runtime_crash_checkpoint(RuntimePhase phase, int frame, std::uintptr_t subject = 0, std::uint32_t id = 0) noexcept;
}

namespace game {
// Values copied by the host before input reset, resource retirement or queuing.
// Stage values: 0 not executed, 1 succeeded, 2 failed. GPU completion remains
// unknown unless the device reports it; CPU submission is not GPU completion.
struct RenderFailureContext {
    std::uint64_t world = 0, generation = 0, tick = 0, render_sequence = 0;
    std::int32_t frame = 0, width = 0, height = 0, camera_x = 0, camera_y = 0;
    DrawingStatus core_status = DrawingStatus::skipped;
    std::uint32_t core = 0, end_frame = 0, enqueue = 0, gpu_prepare = 0, gpu_submit = 0, gpu_complete = 0;
    std::uint32_t commands = 0, resources = 0;
    char map[260]{}, run[96]{}, stage[64]{}, core_error[1024]{}, bridge_error[1024]{};
};
// Copies current log identity for an asynchronously queued frame.
void capture_render_failure_context(RenderFailureContext& output) noexcept;
// Writes a failure event and an independent session snapshot. Output is an
// optional caller-owned UTF-8 path buffer; it is empty if writing fails.
// Logging failure never changes the supplied failure or drawing result.
bool write_render_failure(const RenderFailureContext&, const DrawingFailureInfo&, char* path,
                          std::uint32_t path_capacity) noexcept;
}
