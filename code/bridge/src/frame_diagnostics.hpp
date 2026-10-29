#pragma once
#include "api/runtime_diagnostics.hpp"
#include <cstdio>
namespace game {
inline void record_core_frame_result(DrawingStatus status) noexcept {
    if (status != DrawingStatus::drawn && status != DrawingStatus::skipped)
        record_drawing_diagnostic(status, "core_draw", "advance_map_failed", "", __FILE__, __LINE__);
}
inline bool finish_frame_diagnostics(DrawingStatus status, bool prepared, const char* bridge_error,
                                     RenderFailureContext& context, DrawingFailureInfo& failure) noexcept {
    const bool core_ok = status == DrawingStatus::drawn || status == DrawingStatus::skipped;
    if (prepared && core_ok) return true;
    record_core_frame_result(status);
    if (!prepared)
        record_drawing_diagnostic(DrawingStatus::backend_failure, "bridge_end_frame", "frame_finalization_failed",
                                  bridge_error, __FILE__, __LINE__);
    get_drawing_failure(failure);
    context.core_status = status;
    context.core = core_ok ? 1 : 2;
    context.end_frame = prepared ? 1 : 2;
    std::snprintf(context.stage, sizeof(context.stage), "%s", failure.stage);
    std::snprintf(context.bridge_error, sizeof(context.bridge_error), "%s", bridge_error ? bridge_error : "");
    return false;
}
}
