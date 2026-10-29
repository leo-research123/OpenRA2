#pragma once
#include "api/type_drawing.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdint>

namespace game {
// Copied failure data. No borrowed object/resource pointers survive a call.
struct DrawingFailureInfo {
    std::uint64_t id = 0, request = 0;
    std::int32_t frame = 0;
    DrawingStatus status = DrawingStatus::skipped;
    std::uint32_t line = 0;
    char stage[96]{}, condition[768]{}, source[192]{}, detail[2048]{}, object[512]{};
};
// The host serializes calls per drawing thread. Only an explicit new drawing
// operation clears the latch; queries and cleanup never clear it. No exceptions
// may cross these boundaries. Failure leaves query output unchanged.
void begin_drawing_diagnostics(std::int32_t copied_frame = -1) noexcept;
std::uint64_t next_drawing_request() noexcept;
std::int32_t drawing_diagnostic_frame() noexcept;
bool get_drawing_failure(DrawingFailureInfo& output) noexcept;
DrawingStatus record_drawing_diagnostic(DrawingStatus, const char* stage, const char* condition, const char* detail,
                                        const char* source, std::uint32_t line, std::uint64_t request = 0) noexcept;

// Inline request inspection runs only on failure. It copies scalar facts and
// pointer identities without following unvalidated storage or allocating memory.
template <class Request>
void describe_drawing_request(const Request& r, char* output, std::size_t capacity) noexcept {
    std::size_t used = 0;
    const auto add = [&](const char* format, auto... values) {
        if (used >= capacity) return;
        const int n = std::snprintf(output + used, capacity - used, format, values...);
        if (n > 0) used += static_cast<std::size_t>(n) < capacity - used ? n : capacity - used - 1;
    };
    if (capacity) output[0] = 0;
#define RA2_DIAG_NUMBER(field)                                                                                         \
    if constexpr (requires { static_cast<long long>(r.field); }) add(#field "=%lld ", static_cast<long long>(r.field));
#define RA2_DIAG_POINTER(field)                                                                                        \
    if constexpr (requires { static_cast<const void*>(r.field); })                                                     \
        add(#field "=0x%llX ", static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(r.field)));
    RA2_DIAG_NUMBER(frame);
    RA2_DIAG_NUMBER(depth_frame);
    RA2_DIAG_NUMBER(sub_tile);
    RA2_DIAG_NUMBER(width);
    RA2_DIAG_NUMBER(height);
    RA2_DIAG_NUMBER(pitch);
    RA2_DIAG_NUMBER(pixel_count);
    RA2_DIAG_NUMBER(byte_count);
    RA2_DIAG_NUMBER(count);
    RA2_DIAG_NUMBER(depth_mode);
    RA2_DIAG_NUMBER(blend_mode);
    RA2_DIAG_NUMBER(gradient);
    RA2_DIAG_NUMBER(absolute_depth);
    RA2_DIAG_NUMBER(intensity);
    RA2_DIAG_NUMBER(color);
    RA2_DIAG_NUMBER(line_rgb);
    RA2_DIAG_NUMBER(line_opacity);
    RA2_DIAG_NUMBER(line_channels);
    RA2_DIAG_NUMBER(line_weight);
    RA2_DIAG_NUMBER(line_neighbor_weight);
    RA2_DIAG_NUMBER(spotlight_flags);
    RA2_DIAG_NUMBER(wave_pixel_count);
    RA2_DIAG_NUMBER(original_line);
    RA2_DIAG_NUMBER(operation);
    RA2_DIAG_NUMBER(stage);
    RA2_DIAG_NUMBER(brightness);
    RA2_DIAG_NUMBER(initial_duration);
    RA2_DIAG_NUMBER(warp_step);
    RA2_DIAG_NUMBER(resource_revision);
    RA2_DIAG_NUMBER(content_identity);
    RA2_DIAG_NUMBER(content_version);
    RA2_DIAG_NUMBER(instance);
    RA2_DIAG_NUMBER(instance_slots);
    RA2_DIAG_NUMBER(instance_state);
    RA2_DIAG_NUMBER(slot_count);
    RA2_DIAG_NUMBER(release_count);
    RA2_DIAG_NUMBER(info_capacity);
    RA2_DIAG_NUMBER(world);
    RA2_DIAG_NUMBER(revision);
    RA2_DIAG_NUMBER(lighting_quality);
    RA2_DIAG_NUMBER(dirty_count);
    RA2_DIAG_NUMBER(order_count);
    RA2_DIAG_NUMBER(draw_count);
    RA2_DIAG_NUMBER(light_count);
    RA2_DIAG_NUMBER(shroud_count);
    RA2_DIAG_NUMBER(alpha_count);
    RA2_DIAG_NUMBER(layer);
    RA2_DIAG_NUMBER(object);
    RA2_DIAG_NUMBER(digest);
    RA2_DIAG_NUMBER(epoch);
    RA2_DIAG_POINTER(target);
    RA2_DIAG_POINTER(palette);
    RA2_DIAG_POINTER(image);
    RA2_DIAG_POINTER(depth_image);
    RA2_DIAG_POINTER(resource);
    RA2_DIAG_POINTER(pixels);
    RA2_DIAG_POINTER(bits);
    RA2_DIAG_POINTER(voxel);
    RA2_DIAG_POINTER(entries);
    RA2_DIAG_POINTER(dirty);
    RA2_DIAG_POINTER(orders);
    RA2_DIAG_POINTER(places);
    RA2_DIAG_POINTER(lights);
    RA2_DIAG_POINTER(shroud);
    RA2_DIAG_POINTER(alphas);
    RA2_DIAG_POINTER(visible);
    RA2_DIAG_POINTER(window);
    RA2_DIAG_POINTER(converter);
#undef RA2_DIAG_NUMBER
#undef RA2_DIAG_POINTER
    if constexpr (requires { r.flags; }) add("flags=0x%llX ", static_cast<unsigned long long>(r.flags));
    if constexpr (requires {
                      r.position.X;
                      r.position.Y;
                  })
        add("position=(%d,%d) ", r.position.X, r.position.Y);
    if constexpr (requires { r.clip.Width; })
        add("clip=(%d,%d,%d,%d) ", r.clip.X, r.clip.Y, r.clip.Width, r.clip.Height);
    if constexpr (requires { r.cell.X; }) add("cell=(%d,%d) ", int(r.cell.X), int(r.cell.Y));
    if constexpr (requires { r.step.X; }) add("step=(%d,%d) ", r.step.X, r.step.Y);
    if constexpr (requires { r.view_origin.X; }) add("view_origin=(%d,%d) ", r.view_origin.X, r.view_origin.Y);
    if constexpr (requires { r.shape; }) {
        add("%s", "shape: ");
        if (used < capacity) describe_drawing_request(r.shape, output + used, capacity - used);
    }
}
template <class Request>
DrawingStatus diagnose_drawing_result(DrawingStatus status, const char* stage, const char* condition,
                                      const Request& request, const char* source, std::uint32_t line,
                                      std::uint64_t ordinal = 0) noexcept {
    // Unsupported/unavailable are also feature-negotiation results. The final
    // frame boundary records them only if fallback did not handle them.
    if (status == DrawingStatus::invalid_argument || status == DrawingStatus::backend_failure) {
        char detail[2048];
        describe_drawing_request(request, detail, sizeof(detail));
        record_drawing_diagnostic(status, stage, condition, detail, source, line, ordinal);
    }
    return status;
}
}
