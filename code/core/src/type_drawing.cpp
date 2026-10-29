#include "type_drawing.hpp"
#include "verify/draw_coverage.hpp"
#include "presentation/recorder.hpp"
#include "api/drawing_diagnostics.hpp"
#include "yrpp/Unsorted.h"
#include <atomic>
#include <cstring>
#include "yrpp/ObjectClass.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <cstdio>
#include <vector>
#include <algorithm>
#include <climits>

namespace game {
namespace {
// Per-thread failure diagnostics of the frame being drawn (the first failure survives later passes).
thread_local DrawingFailureInfo diagnostic;
thread_local std::uint64_t request_ordinal = 0;
thread_local std::int32_t copied_frame = -1;
thread_local char failure[4096]{};
std::atomic<std::uint64_t> next_failure_id{1};
bool valid(const TypeDrawingContext& context) noexcept {
    return valid_drawing_context(context);
}
}
bool drawing_completed(DrawingStatus status) noexcept {
    return status == DrawingStatus::drawn || status == DrawingStatus::skipped;
}
DrawingStatus with_drawing_resources(const DrawingResources& value, const TypeDrawingContext& context,
                                     void (*operation)(void*, PresentationContext&), void* argument,
                                     PresentationContext ctx) noexcept {
    ctx.resources = &value;
    return with_type_drawing(context, operation, argument, ctx);
}
bool resolve_drawing_palette(PresentationContext& ctx, DrawingPaletteKind kind, int index,
                             const DrawingPaletteHandle*& result) noexcept {
    result = nullptr;
    const auto* resources = ctx.resources;
    auto status = resources && resources->palette ? resources->palette(resources->context, kind, index, result)
                                                  : DrawingStatus::unavailable;
    if (status == DrawingStatus::drawn && !result) status = DrawingStatus::unavailable;
    if (!drawing_completed(status)) record_type_drawing_result(ctx, status);
    return status == DrawingStatus::drawn;
}
void begin_drawing_diagnostics(std::int32_t frame) noexcept {
    copied_frame = frame;
    diagnostic = {};
    request_ordinal = 0;
    failure[0] = 0;
}
std::uint64_t next_drawing_request() noexcept {
    draw_coverage_request();
    return ++request_ordinal;
}
void set_drawing_diagnostic_frame(std::int32_t frame) noexcept { copied_frame = frame; }
std::int32_t drawing_diagnostic_frame() noexcept { return copied_frame >= 0 ? copied_frame : Unsorted::CurrentFrame; }
bool get_drawing_failure(DrawingFailureInfo& output) noexcept {
    if (!diagnostic.id) return false;
    output = diagnostic;
    return true;
}
DrawingStatus record_drawing_diagnostic(DrawingStatus status, const char* stage, const char* condition,
                                        const char* detail, const char* source, std::uint32_t line,
                                        std::uint64_t request) noexcept {
    if (drawing_completed(status) || diagnostic.id) return status;
    diagnostic.id = next_failure_id.fetch_add(1, std::memory_order_relaxed);
    diagnostic.request = request ? request : request_ordinal;
    diagnostic.frame = drawing_diagnostic_frame();
    diagnostic.status = status;
    diagnostic.line = line;
    std::snprintf(diagnostic.stage, sizeof(diagnostic.stage), "%s", stage ? stage : "unknown");
    std::snprintf(diagnostic.condition, sizeof(diagnostic.condition), "%s", condition ? condition : "unknown");
    std::snprintf(diagnostic.detail, sizeof(diagnostic.detail), "%s", detail ? detail : "");
    std::snprintf(diagnostic.source, sizeof(diagnostic.source), "%s", source ? source : "");
    return status;
}
void clear_drawing_failure() noexcept { begin_drawing_diagnostics(); }
const char* drawing_failure() noexcept {
    if (!diagnostic.id) return "";
    std::snprintf(failure, sizeof(failure),
                  "failure=%llu request=%llu frame=%d %s: %s; rejected=%s; %s; %s; source=%s:%u",
                  static_cast<unsigned long long>(diagnostic.id), static_cast<unsigned long long>(diagnostic.request),
                  diagnostic.frame, diagnostic.stage, drawing_status_name(diagnostic.status), diagnostic.condition,
                  diagnostic.detail, diagnostic.object, diagnostic.source, diagnostic.line);
    return failure;
}
DrawingStatus record_drawing_failure(DrawingStatus status, const char* stage, const ObjectClass* object,
                                     const ShapeDrawingRequest* shape) noexcept {
    if (drawing_completed(status)) return status;
    record_drawing_diagnostic(status, stage, "object_draw_failed", "", __FILE__, __LINE__);
    if (diagnostic.object[0]) return status;
    const auto* type = object ? object->GetType() : nullptr;
    const auto location = object ? object->Location : CoordStruct{};
    const auto* image = shape && shape->image ? shape->image->AsReference() : nullptr;
    const auto* depth = shape && shape->depth_image ? shape->depth_image->AsReference() : nullptr;
    std::snprintf(diagnostic.object, sizeof(diagnostic.object),
                  "%s: %s; object=%s id=%u kind=%d world=(%d,%d,%d) image=%s frame=%d flags=0x%X depth_image=%s", stage,
                  drawing_status_name(status), type ? type->ID : "<none>", object ? unsigned(object->UniqueID) : 0,
                  object ? int(object->WhatAmI()) : -1, location.X, location.Y, location.Z,
                  image ? image->Filename : "<none/raw>", shape ? shape->frame : -1, shape ? shape->flags : 0,
                  depth ? depth->Filename : "<none/raw>");
    return status;
}
DrawingStatus type_drawing_status(const PresentationContext& ctx) noexcept {
    return ctx.type_status ? *ctx.type_status : DrawingStatus::skipped;
}
void record_type_drawing_result(PresentationContext& ctx, DrawingStatus status) noexcept {
    if (!ctx.type_status || !drawing_completed(*ctx.type_status)) return;
    if (status != DrawingStatus::skipped) *ctx.type_status = status;
}
DrawingStatus submit_type_shape(const TypeDrawingContext& context, const ShapeDrawingRequest& request) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_shape", condition, request, __FILE__, line, ordinal);
    };
    if (!valid(context)) return DrawingStatus::unsupported;
    if (!supports_drawing(context, DrawingKind::shape)) return DrawingStatus::unavailable;
    if (!request.target || !request.palette || request.frame < 0)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !request.target || !request.palette || request.frame < 0", __LINE__);
    if (!request.image || request.clip.Width <= 0 || request.clip.Height <= 0) return DrawingStatus::skipped;
    try {
        return diagnose(record_shape(context, request), "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus submit_type_tile(const TypeDrawingContext& context, const TileDrawingRequest& request) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_tile", condition, request, __FILE__, line, ordinal);
    };
    if (!valid(context)) return DrawingStatus::unsupported;
    if (!supports_drawing(context, DrawingKind::tile)) return DrawingStatus::unavailable;
    if (!request.target || !request.palette || request.sub_tile < 0)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !request.target || !request.palette || request.sub_tile < 0", __LINE__);
    if (!request.resource || !request.image || request.clip.Width <= 0 || request.clip.Height <= 0)
        return DrawingStatus::skipped;
    try {
        return diagnose(record_tile(context, request), "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus with_type_drawing(const TypeDrawingContext& context, void (*operation)(void*, PresentationContext&),
                                void* argument, PresentationContext ctx) noexcept {
    if (!operation) return DrawingStatus::invalid_argument;
    if (!valid(context)) return DrawingStatus::unsupported;
    DrawingStatus status = DrawingStatus::skipped;
    ctx.type = &context;
    ctx.type_status = &status;
    try {
        operation(argument, ctx);
    } catch (...) {
        status = DrawingStatus::backend_failure;
    }
    return status;
}
DrawingStatus submit_type_mask(const TypeDrawingContext& c, const MaskDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_mask", condition, r, __FILE__, line, ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    if (!r.target || r.width < 0 || r.height < 0 || r.width > 4096)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !r.target || r.width < 0 || r.height < 0 || r.width > 4096", __LINE__);
    if (!r.width || !r.height || r.clip.Width <= 0 || r.clip.Height <= 0) return DrawingStatus::skipped;
    if (!r.bits || r.pitch < (r.width + 7) / 8 ||
        std::uint64_t(r.height - 1) * r.pitch + (r.width + 7) / 8 > r.byte_count ||
        std::int64_t(r.position.X) + r.width > INT_MAX || std::int64_t(r.position.Y) + r.height > INT_MAX)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !r.bits || r.pitch < (r.width + 7) / 8 || std::uint64_t(r.height - 1) * r.pitch "
                        "+ (r.width + 7) / 8 > r.byte_count || std::int64_t(r.position.X) + r.width > INT_MAX || "
                        "std::int64_t(r.position.Y) + r.height > INT_MAX",
                        __LINE__);
    const auto left = std::max(std::int64_t(r.position.X), std::int64_t(r.clip.X));
    const auto top = std::max(std::int64_t(r.position.Y), std::int64_t(r.clip.Y));
    const auto right = std::min(std::int64_t(r.position.X) + r.width, std::int64_t(r.clip.X) + r.clip.Width);
    const auto bottom = std::min(std::int64_t(r.position.Y) + r.height, std::int64_t(r.clip.Y) + r.clip.Height);
    if (left >= right || top >= bottom) return DrawingStatus::skipped;
    if (supports_drawing(c, DrawingKind::mask)) {
        try {
            const auto status = record_mask(c, r);
            if (status != DrawingStatus::unsupported) return diagnose(status, "backend_result", __LINE__);
        } catch (...) {
            return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
        }
    }
    bool drawn = false;
    for (auto y = top; y < bottom; ++y) {
        const auto* bits = r.bits + std::size_t(y - r.position.Y) * r.pitch;
        const auto bit = [&](std::int64_t x) {
            return (bits[(x - r.position.X) / 8] & (0x80u >> ((x - r.position.X) % 8))) != 0;
        };
        for (auto x = left; x < right;) {
            if (!bit(x)) {
                ++x;
                continue;
            }
            const auto start = x++;
            while (x < right && bit(x))
                ++x;
            RasterDrawingRequest run;
            run.target = r.target;
            run.position = {int(start), int(y)};
            run.clip = r.clip;
            run.width = int(x - start);
            run.height = 1;
            run.color = r.color;
            const auto status = submit_type_raster(c, run);
            if (!drawing_completed(status)) return diagnose(status, "backend_result", __LINE__);
            drawn |= status == DrawingStatus::drawn;
        }
    }
    return diagnose(drawn ? DrawingStatus::drawn : DrawingStatus::skipped, "backend_result", __LINE__);
}
DrawingStatus submit_type_shape_strip(const TypeDrawingContext& c, const ShapeStripDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_shape_strip", condition, r, __FILE__, line, ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    const auto& shape = r.shape;
    if (r.count < 0 || r.count > 8192 || r.step.X < 0 || r.step.Y < 0 || !shape.target || !shape.palette ||
        shape.frame < 0)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when r.count < 0 || r.count > 8192 || r.step.X < 0 || r.step.Y < 0 || !shape.target "
                        "|| !shape.palette || shape.frame < 0",
                        __LINE__);
    if (shape.flags || shape.depth_image || shape.blend_mode != ShapeBlendMode::palette ||
        (shape.depth_mode != ShapeDepthMode::none && shape.depth_mode != ShapeDepthMode::legacy))
        return DrawingStatus::unsupported;
    if (!r.count || !shape.image || shape.clip.Width <= 0 || shape.clip.Height <= 0) return DrawingStatus::skipped;
    if (std::int64_t(shape.position.X) + std::int64_t(r.count - 1) * r.step.X > INT_MAX ||
        std::int64_t(shape.position.Y) + std::int64_t(r.count - 1) * r.step.Y > INT_MAX)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when std::int64_t(shape.position.X) + std::int64_t(r.count - 1) * r.step.X > INT_MAX "
                        "|| std::int64_t(shape.position.Y) + std::int64_t(r.count - 1) * r.step.Y > INT_MAX",
                        __LINE__);
    if (supports_drawing(c, DrawingKind::shape_strip)) {
        try {
            const auto status = record_shape_strip(c, r);
            if (status != DrawingStatus::unsupported) return diagnose(status, "backend_result", __LINE__);
        } catch (...) {
            return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
        }
    }
    bool drawn = false;
    for (int i = 0; i < r.count; ++i) {
        auto next = shape;
        next.position.X = int(std::int64_t(shape.position.X) + std::int64_t(i) * r.step.X);
        next.position.Y = int(std::int64_t(shape.position.Y) + std::int64_t(i) * r.step.Y);
        const auto status = submit_type_shape(c, next);
        if (!drawing_completed(status)) return diagnose(status, "backend_result", __LINE__);
        drawn |= status == DrawingStatus::drawn;
    }
    return diagnose(drawn ? DrawingStatus::drawn : DrawingStatus::skipped, "backend_result", __LINE__);
}
DrawingStatus submit_type_raster(const TypeDrawingContext& context, const RasterDrawingRequest& request) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_raster", condition, request, __FILE__, line, ordinal);
    };
    if (!valid(context)) return DrawingStatus::unsupported;
    if ((request.content_identity == 0) != (request.content_version == 0) ||
        (request.content_identity && (!request.pixels || (request.blend_mode != RasterBlendMode::copy &&
                                                          request.blend_mode != RasterBlendMode::spotlight))))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when (request.content_identity == 0) != (request.content_version == 0) || "
                        "(request.content_identity && (!request.pixels || (request.blend_mode != RasterBlendMode::copy "
                        "&& request.blend_mode != RasterBlendMode::spotlight)))",
                        __LINE__);
    if (!supports_drawing(context, DrawingKind::raster)) return DrawingStatus::unavailable;
    if (request.blend_mode == RasterBlendMode::wave) {
        if (!request.target || request.color > 3 || (request.wave_pixel_count && !request.wave_pixels))
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when !request.target || request.color > 3 || (request.wave_pixel_count && "
                            "!request.wave_pixels)",
                            __LINE__);
        if (!request.wave_pixel_count) return DrawingStatus::skipped;
        try {
            return diagnose(record_raster(context, request), "backend_result",
                            __LINE__);
        } catch (...) {
            return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
        }
    }
    if (request.blend_mode != RasterBlendMode::copy && request.blend_mode != RasterBlendMode::spotlight &&
        request.blend_mode != RasterBlendMode::depth_glow && request.blend_mode != RasterBlendMode::depth_alpha &&
        request.blend_mode != RasterBlendMode::particle && request.blend_mode != RasterBlendMode::shroud &&
        request.blend_mode != RasterBlendMode::fog && request.blend_mode != RasterBlendMode::alpha_shape &&
        request.blend_mode != RasterBlendMode::depth_additive &&
        request.blend_mode != RasterBlendMode::depth_laser_pair &&
        request.blend_mode != RasterBlendMode::light_nonzero && request.blend_mode != RasterBlendMode::light_zero)
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when request.blend_mode != RasterBlendMode::copy && request.blend_mode != "
            "RasterBlendMode::spotlight && request.blend_mode != RasterBlendMode::depth_glow && request.blend_mode != "
            "RasterBlendMode::depth_alpha && request.blend_mode != RasterBlendMode::particle && request.blend_mode != "
            "RasterBlendMode::shroud && request.blend_mode != RasterBlendMode::fog && request.blend_mode != "
            "RasterBlendMode::alpha_shape && request.blend_mode != RasterBlendMode::depth_additive && "
            "request.blend_mode != RasterBlendMode::depth_laser_pair && request.blend_mode != "
            "RasterBlendMode::light_nonzero && request.blend_mode != RasterBlendMode::light_zero",
            __LINE__);
    if ((request.blend_mode == RasterBlendMode::light_nonzero || request.blend_mode == RasterBlendMode::light_zero) &&
        (request.width != 1 || request.height != 1 || request.pixels))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when (request.blend_mode == RasterBlendMode::light_nonzero || request.blend_mode == "
                        "RasterBlendMode::light_zero) && (request.width != 1 || request.height != 1 || request.pixels)",
                        __LINE__);
    if (request.original_line && request.blend_mode != RasterBlendMode::copy)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when request.original_line && request.blend_mode != RasterBlendMode::copy", __LINE__);
    if (request.blend_mode != RasterBlendMode::copy && request.spotlight_flags > 15)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when request.blend_mode != RasterBlendMode::copy && request.spotlight_flags > 15",
                        __LINE__);
    if (request.blend_mode == RasterBlendMode::spotlight && !request.pixels && request.color > 255)
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when request.blend_mode == RasterBlendMode::spotlight && !request.pixels && request.color > 255",
            __LINE__);
    if (request.blend_mode == RasterBlendMode::depth_alpha &&
        (request.line_rgb > 0xFFFFFFu || request.line_opacity < 0 || request.line_opacity > 255))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when request.blend_mode == RasterBlendMode::depth_alpha && (request.line_rgb > "
                        "0xFFFFFFu || request.line_opacity < 0 || request.line_opacity > 255)",
                        __LINE__);
    if ((request.blend_mode == RasterBlendMode::depth_additive ||
         request.blend_mode == RasterBlendMode::depth_laser_pair) &&
        (request.line_rgb > 0xFFFFFFu || request.line_channels > 7 || request.line_weight < 0 ||
         request.line_weight > 256 || request.line_neighbor_weight < 0 || request.line_neighbor_weight > 256))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when (request.blend_mode == RasterBlendMode::depth_additive || request.blend_mode == "
                        "RasterBlendMode::depth_laser_pair) && (request.line_rgb > 0xFFFFFFu || request.line_channels "
                        "> 7 || request.line_weight < 0 || request.line_weight > 256 || request.line_neighbor_weight < "
                        "0 || request.line_neighbor_weight > 256)",
                        __LINE__);
    if (request.blend_mode == RasterBlendMode::particle && request.line_rgb > 0xFFFFFFu)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when request.blend_mode == RasterBlendMode::particle && request.line_rgb > 0xFFFFFFu",
                        __LINE__);

    if (!request.target || request.width < 0 || request.height < 0 || request.width > 8192 || request.height > 8192)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !request.target || request.width < 0 || request.height < 0 || request.width > "
                        "8192 || request.height > 8192",
                        __LINE__);
    if (!request.width || !request.height || request.clip.Width <= 0 || request.clip.Height <= 0)
        return DrawingStatus::skipped;
    if ((request.original_line || request.blend_mode == RasterBlendMode::depth_glow ||
         request.blend_mode == RasterBlendMode::depth_alpha || request.blend_mode == RasterBlendMode::particle ||
         request.blend_mode == RasterBlendMode::depth_additive ||
         request.blend_mode == RasterBlendMode::depth_laser_pair) &&
        (request.width != 1 || request.height != 1 || request.pixels))
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when (request.original_line || request.blend_mode == RasterBlendMode::depth_glow || "
            "request.blend_mode == RasterBlendMode::depth_alpha || request.blend_mode == RasterBlendMode::particle || "
            "request.blend_mode == RasterBlendMode::depth_additive || request.blend_mode == "
            "RasterBlendMode::depth_laser_pair) && (request.width != 1 || request.height != 1 || request.pixels)",
            __LINE__);
    if (request.pixels && (request.pitch < request.width ||
                           std::uint64_t(request.height - 1) * request.pitch + request.width > request.pixel_count))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when request.pixels && (request.pitch < request.width || "
                        "std::uint64_t(request.height - 1) * request.pitch + request.width > request.pixel_count)",
                        __LINE__);
    try {
        return diagnose(record_raster(context, request), "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus submit_type_lighting_shape(const TypeDrawingContext& c, const LightingShapeDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_lighting_shape", condition, r, __FILE__, line, ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    if (!r.target || r.frame < 0 ||
        (r.operation != RasterBlendMode::shroud && r.operation != RasterBlendMode::fog &&
         r.operation != RasterBlendMode::alpha_shape))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !r.target || r.frame < 0 || (r.operation != RasterBlendMode::shroud && "
                        "r.operation != RasterBlendMode::fog && r.operation != RasterBlendMode::alpha_shape)",
                        __LINE__);
    if (!r.image) return DrawingStatus::unavailable;
    if (r.clip.Width <= 0 || r.clip.Height <= 0) return DrawingStatus::skipped;
    try {
        if (supports_drawing(c, DrawingKind::lighting_shape))
            return diagnose(record_lighting_shape(c, r), "backend_result", __LINE__);
        if (!supports_drawing(c, DrawingKind::raster)) return DrawingStatus::unavailable;
        auto* image = r.image;
        if (auto* ref = image->AsReference()) {
            ref->Load();
            image = ref->Data;
        }
        if (!image) return DrawingStatus::unavailable;
        if (r.frame >= image->Frames)
            return diagnose(DrawingStatus::invalid_argument, "rejected when r.frame >= image->Frames", __LINE__);
        const auto rect = image->GetFrameBounds(r.frame);
        if (rect.Width <= 0 || rect.Height <= 0) return DrawingStatus::skipped;
        if (rect.Width > 8192 || rect.Height > 8192)
            return diagnose(DrawingStatus::invalid_argument, "rejected when rect.Width > 8192 || rect.Height > 8192",
                            __LINE__);
        const auto* pixels = image->GetPixels(r.frame);
        if (!pixels) return DrawingStatus::unavailable;
        // Same raw lighting-frame contract as the original shroud/alpha loop.
        std::vector<std::uint16_t> samples(std::size_t(rect.Width) * rect.Height);
        for (std::size_t i = 0; i < samples.size(); ++i)
            samples[i] = pixels[i];
        const auto x = std::int64_t(r.position.X) + rect.X, y = std::int64_t(r.position.Y) + rect.Y;
        if (x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX)
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX", __LINE__);
        RasterDrawingRequest raster;
        raster.target = r.target;
        raster.position = {int(x), int(y)};
        raster.clip = r.clip;
        raster.width = rect.Width;
        raster.height = rect.Height;
        raster.pitch = rect.Width;
        raster.pixels = samples.data();
        raster.pixel_count = unsigned(samples.size());
        raster.blend_mode = r.operation;
        return submit_type_raster(c, raster);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus submit_type_lighting_shape_batch(const TypeDrawingContext& c,
                                               const LightingShapeBatchRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_lighting_shape_batch", condition, r, __FILE__, line,
                                       ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    if (!r.target || (r.count && !r.entries))
        return diagnose(DrawingStatus::invalid_argument, "rejected when !r.target || (r.count && !r.entries)",
                        __LINE__);
    for (std::uint32_t i = 0; i < r.count; ++i) {
        const auto& q = r.entries[i].request;
        if (q.target != r.target || q.frame < 0 || !q.image || q.clip.Width <= 0 || q.clip.Height <= 0 ||
            (q.operation != RasterBlendMode::shroud && q.operation != RasterBlendMode::fog &&
             q.operation != RasterBlendMode::alpha_shape))
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when q.target != r.target || q.frame < 0 || !q.image || q.clip.Width <= 0 || "
                            "q.clip.Height <= 0 || (q.operation != RasterBlendMode::shroud && q.operation != "
                            "RasterBlendMode::fog && q.operation != RasterBlendMode::alpha_shape)",
                            __LINE__);
    }
    // A world shroud batch draws the scene's cells even without entries;
    // only a batching host holds those cells.
    if (!r.count && !r.shroud) return DrawingStatus::skipped;
    if (r.shroud && !supports_drawing(c, DrawingKind::lighting_shape_batch)) return DrawingStatus::unsupported;
    try {
        if (supports_drawing(c, DrawingKind::lighting_shape_batch))
            return diagnose(record_lighting_shape_batch(c, r), "backend_result", __LINE__);
        auto result = DrawingStatus::skipped;
        for (std::uint32_t i = 0; i < r.count; ++i) {
            const auto status = submit_type_lighting_shape(c, r.entries[i].request);
            if (!drawing_completed(status)) return diagnose(status, "backend_result", __LINE__);
            if (status == DrawingStatus::drawn) result = status;
        }
        return diagnose(result, "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus submit_type_indexed(const TypeDrawingContext& c, const IndexedDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_type_indexed", condition, r, __FILE__, line, ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    if (!(r.voxel ? supports_drawing(c, DrawingKind::voxel_indexed) : supports_drawing(c, DrawingKind::indexed))) return DrawingStatus::unavailable;
    if (!r.target || !r.palette || r.width < 0 || r.height < 0 || r.width > 2048 || r.height > 2048)
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when !r.target || !r.palette || r.width < 0 || r.height < 0 || r.width > 2048 || r.height > 2048",
            __LINE__);
    if (!r.width || !r.height || r.clip.Width <= 0 || r.clip.Height <= 0) return DrawingStatus::skipped;
    if ((!r.voxel && (!r.pixels || std::uint64_t(r.width) * r.height > r.pixel_count)) ||
        static_cast<unsigned>(r.depth_mode) > static_cast<unsigned>(ShapeDepthMode::read_write))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when (!r.voxel && (!r.pixels || std::uint64_t(r.width) * r.height > r.pixel_count)) "
                        "|| static_cast<unsigned>(r.depth_mode) > static_cast<unsigned>(ShapeDepthMode::read_write)",
                        __LINE__);
    try {
        return diagnose(r.voxel ? record_voxel_indexed(c, r)
                                : record_indexed(c, r),
                        "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
const char* drawing_status_name(DrawingStatus status) noexcept {
    switch (status) {
        case DrawingStatus::drawn:
            return "drawn";
        case DrawingStatus::skipped:
            return "skipped";
        case DrawingStatus::unavailable:
            return "unavailable";
        case DrawingStatus::unsupported:
            return "unsupported";
        case DrawingStatus::invalid_argument:
            return "invalid_argument";
        case DrawingStatus::backend_failure:
            return "backend_failure";
    }
    return "invalid_status";
}
}

namespace game {
DrawingStatus submit_screen_effect(const TypeDrawingContext& c, const ScreenEffectDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_screen_effect", condition, r, __FILE__, line, ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    if (!supports_drawing(c, DrawingKind::screen_effect)) return DrawingStatus::unavailable;
    if (!r.target || r.width < 1 || r.height < 1 || r.width > 8192 || r.height > 8192 || r.brightness < 0 ||
        r.brightness > 255 ||
        static_cast<std::uint32_t>(r.operation) > static_cast<std::uint32_t>(ScreenEffectOperation::ion_blast))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !r.target || r.width < 1 || r.height < 1 || r.width > 8192 || r.height > 8192 "
                        "|| r.brightness < 0 || r.brightness > 255 || static_cast<std::uint32_t>(r.operation) > "
                        "static_cast<std::uint32_t>(ScreenEffectOperation::ion_blast)",
                        __LINE__);
    if (r.initial_duration <= 0 || r.warp_step < 0 || r.warp_step > r.initial_duration)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when r.initial_duration <= 0 || r.warp_step < 0 || r.warp_step > r.initial_duration",
                        __LINE__);
    try {
        return diagnose(record_screen_effect(c, r), "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus submit_background_scene(const TypeDrawingContext& c, const BackgroundSceneRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_background_scene", condition, r, __FILE__, line, ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    if (!supports_drawing(c, DrawingKind::background_scene)) return DrawingStatus::unavailable;
    if (!r.target || static_cast<unsigned>(r.operation) > static_cast<unsigned>(BackgroundSceneOperation::alpha))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !r.target || static_cast<unsigned>(r.operation) > "
                        "static_cast<unsigned>(BackgroundSceneOperation::alpha)",
                        __LINE__);
    try {
        return diagnose(record_background_scene(c, r), "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus submit_background_complete(const TypeDrawingContext& c, const BackgroundDrawingRequest& r) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "submit_background_complete", condition, r, __FILE__, line, ordinal);
    };
    if (!valid(c)) return DrawingStatus::unsupported;
    if (!supports_drawing(c, DrawingKind::background_complete)) return DrawingStatus::unavailable;
    if (!r.target) return diagnose(DrawingStatus::invalid_argument, "rejected when !r.target", __LINE__);
    try {
        return diagnose(record_background_complete(c, r), "backend_result", __LINE__);
    } catch (...) {
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
}
