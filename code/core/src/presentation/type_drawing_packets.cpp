#include "api/drawing_diagnostics.hpp"
// Resource decoding follows the existing SHP decoder and XCC TMP traversal in
// IsometricTileTypeClassDraw.cpp. YR lighting/depth expressions: 547CF0,
// AlphaLightingRemapClass 420140. No Surface, ConvertClass or rasterizer linked.
#include "type_drawing_packets.hpp"
#include "api/images.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <string>
namespace game {
DrawingStatus prepare_type_laser_parameters(const RasterDrawingRequest& r, const TypeGpuTarget& target,
                                            TypeGpuPacket& packet) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_laser_parameters", condition, r, __FILE__, line, ordinal);
    };
    if ((r.blend_mode != RasterBlendMode::depth_additive && r.blend_mode != RasterBlendMode::depth_laser_pair) ||
        r.width != 1 || r.height != 1 || r.pixels || r.line_rgb > 0xFFFFFFu || r.line_channels > 7 ||
        r.line_weight < 0 || r.line_weight > 256 || r.line_neighbor_weight < 0 || r.line_neighbor_weight > 256)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when (r.blend_mode != RasterBlendMode::depth_additive && r.blend_mode != "
                        "RasterBlendMode::depth_laser_pair) || r.width != 1 || r.height != 1 || r.pixels || r.line_rgb "
                        "> 0xFFFFFFu || r.line_channels > 7 || r.line_weight < 0 || r.line_weight > 256 || "
                        "r.line_neighbor_weight < 0 || r.line_neighbor_weight > 256",
                        __LINE__);
    const int x = r.position.X, y = r.position.Y;
    if (x < 0 || y < 0 || x >= target.width || y >= target.height || x < r.clip.X || y < r.clip.Y ||
        std::int64_t(x) >= std::int64_t(r.clip.X) + r.clip.Width ||
        std::int64_t(y) >= std::int64_t(r.clip.Y) + r.clip.Height)
        return DrawingStatus::skipped;
    packet.parameters = {target.width, target.height, 1, 1, x, y, x, y, 1, 1};
    auto& p = packet.parameters;
    p[19] = r.blend_mode == RasterBlendMode::depth_additive ? 9 : 10;
    p[10] = int(r.line_channels);
    p[11] = int(r.line_rgb);
    p[12] = r.line_weight | (r.line_neighbor_weight << 16);
    p[13] = int(r.spotlight_flags & 1);
    p[14] = std::uint16_t(unsigned(target.depth_max) + unsigned(target.depth_bounds_y) - unsigned(y) -
                          unsigned(r.clip.Y) + unsigned(r.line_z));
    p[15] = r.blend_mode == RasterBlendMode::depth_additive ? r.line_light_x : r.line_sample_x;
    if (r.blend_mode == RasterBlendMode::depth_additive) {
        p[10] = r.line_blue;
        p[11] = r.line_red;
        p[12] = r.line_green;
    }
    p[16] = target.depth_bounds_y;
    p[17] = r.line_neighbor.X;
    p[18] = r.line_neighbor.Y;
    return DrawingStatus::drawn;
}

DrawingStatus prepare_type_light_test_parameters(const RasterDrawingRequest& r, const TypeGpuTarget& target,
                                                 TypeGpuPacket& packet) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_light_test_parameters", condition, r, __FILE__, line,
                                       ordinal);
    };
    if ((r.blend_mode != RasterBlendMode::light_nonzero && r.blend_mode != RasterBlendMode::light_zero) ||
        r.width != 1 || r.height != 1 || r.pixels)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when (r.blend_mode != RasterBlendMode::light_nonzero && r.blend_mode != "
                        "RasterBlendMode::light_zero) || r.width != 1 || r.height != 1 || r.pixels",
                        __LINE__);
    const int x = r.position.X, y = r.position.Y;
    if (x < 0 || y < 0 || x >= target.width || y >= target.height || x < r.clip.X || y < r.clip.Y ||
        std::int64_t(x) >= std::int64_t(r.clip.X) + r.clip.Width ||
        std::int64_t(y) >= std::int64_t(r.clip.Y) + r.clip.Height)
        return DrawingStatus::skipped;
    packet.parameters = {target.width, target.height, 1, 1, x, y, x, y, 1, 1};
    packet.parameters[10] = r.blend_mode == RasterBlendMode::light_zero ? 1 : 0;
    packet.parameters[16] = target.depth_bounds_y;
    packet.parameters[19] = 14;
    return DrawingStatus::drawn;
}

DrawingStatus prepare_type_particle_parameters(const RasterDrawingRequest& r, const TypeGpuTarget& target,
                                               TypeGpuPacket& packet) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_particle_parameters", condition, r, __FILE__, line,
                                       ordinal);
    };
    if (r.blend_mode != RasterBlendMode::particle || r.width != 1 || r.height != 1 || r.pixels ||
        r.line_rgb > 0xFFFFFFu)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when r.blend_mode != RasterBlendMode::particle || r.width != 1 || r.height != 1 || "
                        "r.pixels || r.line_rgb > 0xFFFFFFu",
                        __LINE__);
    const int x = r.position.X, y = r.position.Y;
    if (x < 0 || y < 0 || x >= target.width || y >= target.height || x < r.clip.X || y < r.clip.Y ||
        std::int64_t(x) >= std::int64_t(r.clip.X) + r.clip.Width ||
        std::int64_t(y) >= std::int64_t(r.clip.Y) + r.clip.Height)
        return DrawingStatus::skipped;
    packet.parameters = {target.width, target.height, 1, 1, x, y, x, y, 1, 1};
    packet.parameters[11] = int(r.line_rgb);
    packet.parameters[19] = 8;
    packet.parameters[14] =
        int(std::uint16_t(unsigned(target.depth_max) + unsigned(target.depth_bounds_y) - unsigned(y))) + r.line_z;
    packet.parameters[16] = target.depth_bounds_y;
    return DrawingStatus::drawn;
}
namespace {
SHPFile* shape_data(SHPStruct* image) {
    if (!image) return nullptr;
    // Keep body and auxiliary data alive together. GetData's shared scratch
    // buffer is overwritten by the next reference; GetPixels on the returned
    // raw SHP cannot promote the original reference to persistent storage.
    if (auto* reference = image->AsReference()) {
        reference->Load();
        return static_cast<SHPFile*>(reference->Data);
    }
    return static_cast<SHPFile*>(image);
}
bool integer(std::int64_t n) { return n >= INT32_MIN && n <= INT32_MAX; }
bool setup(TypeGpuPacket& p, const TypeGpuTarget& t, const RectangleStruct& r, std::int64_t x, std::int64_t y, int w,
           int h, int intensity, bool lit) {
    p = {};
    if (t.width < 1 || t.height < 1 || t.width > 8192 || t.height > 8192 || t.shade_count < 1 || t.shade_count > 256 ||
        w < 0 || h < 0 || w > 4096 || h > 4096 || !integer(x) || !integer(y) || !integer(x + w) || !integer(y + h))
        return false;
    const auto right = std::clamp<std::int64_t>(std::int64_t(r.X) + r.Width, 0, t.width);
    const auto bottom = std::clamp<std::int64_t>(std::int64_t(r.Y) + r.Height, 0, t.height);
    const int left = std::clamp(r.X, 0, t.width), top = std::clamp(r.Y, 0, t.height);
    auto& a = p.parameters;
    a[0] = t.width;
    a[1] = t.height;
    a[2] = w;
    a[3] = h;
    a[4] = int(x);
    a[5] = int(y);
    a[6] = left;
    a[7] = top;
    a[8] = int(std::max<std::int64_t>(0, right - left));
    a[9] = int(std::max<std::int64_t>(0, bottom - top));
    a[10] = lit ? 1 : 0;
    a[11] = int(std::min<std::int64_t>(254, 261ll * std::max(intensity, 0) / 2048));
    a[12] = t.shade_count;
    return true;
}
bool visible(const TypeGpuPacket& p) {
    const auto& a = p.parameters;
    return a[8] > 0 && a[9] > 0 && std::int64_t(a[4]) + a[2] > a[6] && std::int64_t(a[5]) + a[3] > a[7] &&
           a[4] < a[6] + a[8] && a[5] < a[7] + a[9];
}
DrawingStatus auxiliary_bounds(const ShapeDrawingRequest& r, SHPFile& shape, RectangleStruct& body, Point2D& offset) {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "auxiliary_bounds", condition, r, __FILE__, line, ordinal);
    };
    if (!r.depth_image) return DrawingStatus::drawn;
    auto* z = shape_data(r.depth_image);
    if (!z) return DrawingStatus::unavailable;
    if (r.depth_mode != ShapeDepthMode::legacy || r.depth_frame < 0 || r.depth_frame >= z->Frames ||
        z->HasCompression(r.depth_frame))
        return DrawingStatus::unsupported;
    // Unlit RLE palette read/write has separate check/write delta streams.
    // Current world auxiliary requests select the calibrated alpha variants.
    if (shape.HasCompression(r.frame) && (r.flags & 0x4000) && !(r.flags & 0x807)) return DrawingStatus::unsupported;
    const auto b = z->GetFrameBounds(r.depth_frame);
    const auto x = std::int64_t(r.depth_offset.X) - shape.Width / 2 + body.X + b.X;
    const auto y = std::int64_t(r.depth_offset.Y) - shape.Height / 2 + body.Y + b.Y;
    // The original row dispatcher requires its auxiliary lock to succeed.
    if (x < 0 || y < 0) return DrawingStatus::unsupported;
    if (x >= b.Width || y >= b.Height) return DrawingStatus::skipped;
    offset = {int(x), int(y)};
    body.Width = std::min(body.Width, b.Width - offset.X);
    body.Height = std::min(body.Height, b.Height - offset.Y);
    return diagnose(body.Width > 0 && body.Height > 0 ? DrawingStatus::drawn : DrawingStatus::skipped, "backend_result",
                    __LINE__);
}
bool original_shape_mode(const ShapeDrawingRequest& r, const TypeGpuTarget& t, bool rle, TypeGpuPacket& packet) {
    // YR 0x490E50 selects plain warp translucency only on the Z-read route.
    // The shader preserves PlainBlit's forward row/pixel order for these reads.
    const bool warp = !rle && (r.flags & 8) && (r.flags & 6) && !(r.flags & 0x4000) && (r.flags & 0x3000);
    if ((r.flags & ~0x37F0Fu) || ((r.flags & 8) && !warp)) return false;
    if (!rle && (r.flags & 6) == 4 && !(r.flags & 0x7800) && (r.flags & 0x30000)) return false;
    if ((r.flags & 0x100) && !(r.flags & 0x7000)) return false;
    const unsigned mode = (r.flags & 0x4000) ? 1 : (r.flags & 0x3000) ? 2 : 0;
    const unsigned blend = (r.flags & 6)   ? ((r.flags & 6) == 6   ? 4
                                              : (r.flags & 6) == 4 ? 3
                                                                   : 2)
                           : (r.flags & 1) ? 1
                                           : 0;
    auto& a = packet.parameters;
    const bool auxiliary = rle && r.depth_image;
    a[13] = int((mode ? 0x10000u : 0u) | (auxiliary ? 0x20000u : 0u) | mode | (blend << 8));
    // Original tinted warp wrappers pass offset zero and ignore tint.
    if (warp && std::uint16_t(r.tint) == 0) {
        a[13] |= 0x40000;
        a[16] = t.depth_bounds_y;
    }

    // Only alpha palette Z-read, and RLE alpha palette Z-read/write, tint.
    // Translucency, shadows, unlit and plain Z-write ignore Draw_Shape's tint.
    if ((r.flags & 0x800) && !blend && (mode == 2 || (rle && mode == 1)))
        a[10] = std::bit_cast<int>(unsigned(a[10]) | (unsigned(r.tint) << 16));
    if (!mode) return true;
    if (!t.shape_z_state || r.gradient < 0 || r.gradient > 3) return false;
    // Exact descriptors at 0x008176F8, also bound by the software reference.
    constexpr int gradients[4][6] = {
        {1, 1, 1, 1, -1, 1}, {2, 3, 2, 3, -1, 1}, {1, 3, 1, 3, 1, 0}, {1, 1, -1, -1, -1, 1}};
    const auto& g = gradients[r.gradient];
    const int top = std::max(a[5], a[7]);
    const int bottom = std::min(a[5] + a[3], a[7] + a[9]);
    const int ratio = g[3] / g[2];
    std::int64_t base = 0;
    int phase = 0;
    if (g[5]) {
        base = std::int64_t(r.depth_adjustment) + std::uint16_t(std::int64_t(t.depth_bounds_y) + t.depth_max - top);
        base = g[1] * (base / g[1]);
    } else {
        const int last = rle ? a[5] + a[3] : bottom;
        const int rows = last - top;
        base =
            std::int64_t(r.depth_adjustment) + std::uint16_t(std::int64_t(t.depth_bounds_y) + t.depth_max - last + 1);
        if (!auxiliary) {
            base = ratio * (base / ratio) - rows / ratio;
            phase = g[3] - rows % ratio;
            if (phase == g[3]) {
                phase = 0;
                base += g[4];
            }
        }
    }
    if (!integer(base)) return false;
    a[14] = int(base);
    // Normalize the original negative unit slope to a positive step/limit.
    const unsigned step = unsigned(std::abs(g[2])), limit = unsigned(std::abs(g[3]));
    a[15] = std::int32_t(unsigned(phase) | (step << 8) | (limit << 16) | (unsigned(std::uint8_t(g[4])) << 24));
    a[18] = top;
    return true;
}
}
DrawingStatus describe_type_shape(const ShapeDrawingRequest& r, TypeShapeMeta& meta) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "describe_type_shape", condition, r, __FILE__, line, ordinal);
    };
    meta = {};
    try {
        if (r.depth_mode != ShapeDepthMode::legacy && r.flags != 0 && r.flags != 0x400 && r.flags != 0x600 &&
            r.flags != 0xE00)
            return DrawingStatus::unsupported;
        if (!r.image) return DrawingStatus::skipped;
        auto* shape = shape_data(r.image);
        if (!shape) return DrawingStatus::unavailable;
        if (r.frame < 0 || r.frame >= shape->Frames) {
            char detail[512];
            const auto* reference = r.image->AsReference();
            std::snprintf(detail, sizeof(detail), "image=%s frame=%d expected_frame=[0,%d)",
                          reference ? reference->Filename : "<raw>", r.frame, int(shape->Frames));
            return record_drawing_diagnostic(DrawingStatus::invalid_argument, "describe_type_shape",
                                              "shape_frame_out_of_range", detail, __FILE__, __LINE__, ordinal);
        }
        meta.body = shape->GetFrameBounds(r.frame);
        const auto aux = auxiliary_bounds(r, *shape, meta.body, meta.auxiliary_offset);
        if (aux != DrawingStatus::drawn) return diagnose(aux, "backend_result", __LINE__);
        meta.width = shape->Width;
        meta.height = shape->Height;
        meta.rle = shape->HasCompression(r.frame);
        return DrawingStatus::drawn;
    } catch (...) {
        meta = {};
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus prepare_type_shape_parameters(const ShapeDrawingRequest& r, const TypeGpuTarget& t,
                                            TypeGpuPacket& output) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_shape_parameters", condition, r, __FILE__, line, ordinal);
    };
    output = {};
    TypeShapeMeta meta;
    const auto described = describe_type_shape(r, meta);
    if (described != DrawingStatus::drawn) return diagnose(described, "backend_result", __LINE__);
    return prepare_type_shape_parameters(r, meta, t, output);
}
DrawingStatus prepare_type_shape_parameters(const ShapeDrawingRequest& r, const TypeShapeMeta& meta,
                                            const TypeGpuTarget& t, TypeGpuPacket& output) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_shape_parameters", condition, r, __FILE__, line, ordinal);
    };
    output = {};
    try {
        const auto& b = meta.body;
        if (b.Width <= 0 || b.Height <= 0 || r.clip.Width <= 0 || r.clip.Height <= 0) return DrawingStatus::skipped;
        // YR's SHP equal-size blit treats the desired position relative to the
        // intersected clipping rectangle. TMP positions are instead absolute.
        const auto x =
            std::int64_t(std::max(r.clip.X, 0)) + r.position.X - ((r.flags & 0x200) ? meta.width / 2 : 0) + b.X;
        const auto y =
            std::int64_t(std::max(r.clip.Y, 0)) + r.position.Y - ((r.flags & 0x200) ? meta.height / 2 : 0) + b.Y;
        if (!setup(output, t, r.clip, x, y, b.Width, b.Height, r.intensity, (r.flags & 0x800) != 0))
            return diagnose(
                DrawingStatus::invalid_argument,
                "rejected when !setup(output, t, r.clip, x, y, b.Width, b.Height, r.intensity, (r.flags & 0x800) != 0)",
                __LINE__);
        if (!visible(output)) {
            output = {};
            return DrawingStatus::skipped;
        }
        const auto depth = static_cast<unsigned>(r.depth_mode);
        const auto blend = static_cast<unsigned>(r.blend_mode);
        if (depth > static_cast<unsigned>(ShapeDepthMode::read_write) ||
            blend > static_cast<unsigned>(ShapeBlendMode::translucent75))
            return DrawingStatus::unsupported;
        if (r.depth_mode != ShapeDepthMode::legacy && r.depth_mode != ShapeDepthMode::none) {
            if (r.absolute_depth < 0 || r.absolute_depth > 65535)
                return diagnose(DrawingStatus::invalid_argument,
                                "rejected when r.absolute_depth < 0 || r.absolute_depth > 65535", __LINE__);
            output.parameters[13] = r.depth_mode == ShapeDepthMode::read_write ? 1 : 2;
            output.parameters[14] = r.absolute_depth;
        }
        output.parameters[13] |= int(blend << 8);
        const bool rle = meta.rle;
        if (r.depth_mode == ShapeDepthMode::legacy && !original_shape_mode(r, t, rle, output))
            return DrawingStatus::unsupported;
        if (rle && !t.shape_z_state) {
            output.parameters[17] = 1;
            output.parameters[18] = std::max(output.parameters[5], output.parameters[7]);
        }
        return DrawingStatus::drawn;
    } catch (...) {
        output = {};
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus compile_type_shape(const ShapeDrawingRequest& request, const TypeShapeMeta& meta,
                                 TypeShapeProgram& program) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "compile_type_shape", condition, request, __FILE__, line, ordinal);
    };
    // Warp reads depend on destination color and remain on the exact path.
    if (request.flags & 8) return DrawingStatus::unavailable;
    auto sample = request;
    sample.clip = {0, 0, 8192, 8192};
    sample.position = {2048 - meta.body.X + ((sample.flags & 0x200) ? meta.width / 2 : 0),
                       2048 - meta.body.Y + ((sample.flags & 0x200) ? meta.height / 2 : 0)};
    sample.depth_adjustment = 0;
    sample.absolute_depth = 0;
    sample.tint = 0;
    TypeGpuPacket packet;
    const auto status = prepare_type_shape_parameters(sample, meta, {8192, 8192, 1, 0, 0x8000, true}, packet);
    if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
    program.meta = meta;
    program.words = packet.parameters;
    return DrawingStatus::drawn;
}
DrawingStatus emit_type_shape(const ShapeDrawingRequest& r, const TypeShapeProgram& program, const TypeGpuTarget& t,
                              TypeGpuPacket& output) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "emit_type_shape", condition, r, __FILE__, line, ordinal);
    };
    if (!t.shape_z_state) return prepare_type_shape_parameters(r, program.meta, t, output);
    const auto& meta = program.meta;
    const auto& b = meta.body;
    if (r.clip.Width <= 0 || r.clip.Height <= 0) return DrawingStatus::skipped;
    const auto x = std::int64_t(std::max(r.clip.X, 0)) + r.position.X - ((r.flags & 0x200) ? meta.width / 2 : 0) + b.X;
    const auto y = std::int64_t(std::max(r.clip.Y, 0)) + r.position.Y - ((r.flags & 0x200) ? meta.height / 2 : 0) + b.Y;
    if (!setup(output, t, r.clip, x, y, b.Width, b.Height, r.intensity, (r.flags & 0x800) != 0))
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when !setup(output, t, r.clip, x, y, b.Width, b.Height, r.intensity, (r.flags & 0x800) != 0)",
            __LINE__);
    if (!visible(output)) {
        output = {};
        return DrawingStatus::skipped;
    }
    auto& a = output.parameters;
    a[13] = program.words[13];
    a[15] = program.words[15];
    if (r.depth_mode != ShapeDepthMode::legacy) {
        if (r.depth_mode != ShapeDepthMode::none) {
            if (r.absolute_depth < 0 || r.absolute_depth > 0xFFFF)
                return diagnose(DrawingStatus::invalid_argument,
                                "rejected when r.absolute_depth < 0 || r.absolute_depth > 0xFFFF", __LINE__);
            a[14] = r.absolute_depth;
        }
        return DrawingStatus::drawn;
    }
    const int mode = a[13] & 0xFF, blend = (a[13] >> 8) & 0xFF;
    if ((r.flags & 0x800) && !blend && (mode == 2 || (meta.rle && mode == 1)))
        a[10] = std::bit_cast<int>(unsigned(a[10]) | (unsigned(r.tint) << 16));
    if (!mode) return DrawingStatus::drawn;
    const bool auxiliary = (a[13] & 0x20000) != 0;
    const int top = std::max(a[5], a[7]), bottom = std::min(a[5] + a[3], a[7] + a[9]);
    std::int64_t base;
    int phase = 0;
    if (r.gradient == 2) {
        const int last = meta.rle ? a[5] + a[3] : bottom, rows = last - top;
        base =
            std::int64_t(r.depth_adjustment) + std::uint16_t(std::int64_t(t.depth_bounds_y) + t.depth_max - last + 1);
        if (!auxiliary) {
            base = 3 * (base / 3) - rows / 3;
            phase = 3 - rows % 3;
            if (phase == 3) {
                phase = 0;
                ++base;
            }
        }
    } else {
        const int divisor = r.gradient == 1 ? 3 : 1;
        base = std::int64_t(r.depth_adjustment) + std::uint16_t(std::int64_t(t.depth_bounds_y) + t.depth_max - top);
        base = divisor * (base / divisor);
    }
    if (!integer(base)) return DrawingStatus::unsupported;
    a[14] = int(base);
    a[15] = std::int32_t((std::uint32_t(a[15]) & 0xFFFFFF00u) | unsigned(phase));
    a[18] = top;
    return DrawingStatus::drawn;
}
DrawingStatus emit_type_shape_instance(const ShapeDrawingRequest& r, const TypeShapeProgram& program,
                                       const TypeGpuTarget& t, render::Instance& a,
                                       std::uint32_t auxiliary_entry) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "emit_type_shape_instance", condition, r, __FILE__, line, ordinal);
    };
    const auto& meta = program.meta;
    const auto& b = meta.body;
    if (!t.shape_z_state) return DrawingStatus::unsupported;
    if (r.clip.Width <= 0 || r.clip.Height <= 0) return DrawingStatus::skipped;
    const auto x = std::int64_t(std::max(r.clip.X, 0)) + r.position.X - ((r.flags & 0x200) ? meta.width / 2 : 0) + b.X;
    const auto y = std::int64_t(std::max(r.clip.Y, 0)) + r.position.Y - ((r.flags & 0x200) ? meta.height / 2 : 0) + b.Y;
    if (t.width < 1 || t.height < 1 || t.width > 8192 || t.height > 8192 || t.shade_count < 1 || t.shade_count > 256 ||
        b.Width < 0 || b.Height < 0 || b.Width > 4096 || b.Height > 4096 || !integer(x) || !integer(y) ||
        !integer(x + b.Width) || !integer(y + b.Height))
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when t.width < 1 || t.height < 1 || t.width > 8192 || t.height > 8192 || t.shade_count < 1 || "
            "t.shade_count > 256 || b.Width < 0 || b.Height < 0 || b.Width > 4096 || b.Height > 4096 || !integer(x) || "
            "!integer(y) || !integer(x + b.Width) || !integer(y + b.Height)",
            __LINE__);
    const auto right = std::clamp<std::int64_t>(std::int64_t(r.clip.X) + r.clip.Width, 0, t.width);
    const auto bottom = std::clamp<std::int64_t>(std::int64_t(r.clip.Y) + r.clip.Height, 0, t.height);
    const int left = std::clamp(r.clip.X, 0, t.width), top = std::clamp(r.clip.Y, 0, t.height);
    if (right <= left || bottom <= top || x + b.Width <= left || y + b.Height <= top || x >= right || y >= bottom)
        return DrawingStatus::skipped;
    const int mode = program.words[13] & 0xFF, blend = (program.words[13] >> 8) & 0xFF;
    const bool lit = (r.flags & 0x800) != 0;
    int light = lit ? 1 : 0, base = 0, gradient = program.words[15], first = 0;
    if (r.depth_mode == ShapeDepthMode::legacy) {
        if (lit && !blend && (mode == 2 || (meta.rle && mode == 1)))
            light = std::bit_cast<int>(unsigned(light) | (unsigned(r.tint) << 16));
        if (mode) {
            first = std::max(int(y), top);
            const auto visible_bottom = std::min(y + b.Height, bottom);
            const bool auxiliary = (program.words[13] & 0x20000) != 0;
            std::int64_t depth;
            int phase = 0;
            if (r.gradient == 2) {
                const auto last = meta.rle ? y + b.Height : visible_bottom, rows = last - first;
                depth = std::int64_t(r.depth_adjustment) +
                        std::uint16_t(std::int64_t(t.depth_bounds_y) + t.depth_max - last + 1);
                if (!auxiliary) {
                    depth = 3 * (depth / 3) - rows / 3;
                    phase = int(3 - rows % 3);
                    if (phase == 3) {
                        phase = 0;
                        ++depth;
                    }
                }
            } else {
                const int divisor = r.gradient == 1 ? 3 : 1;
                depth = std::int64_t(r.depth_adjustment) +
                        std::uint16_t(std::int64_t(t.depth_bounds_y) + t.depth_max - first);
                depth = divisor * (depth / divisor);
            }
            if (!integer(depth)) return DrawingStatus::unsupported;
            base = int(depth);
            gradient = std::int32_t((std::uint32_t(gradient) & 0xFFFFFF00u) | unsigned(phase));
        }
    } else if (r.depth_mode != ShapeDepthMode::none) {
        if (r.absolute_depth < 0 || r.absolute_depth > 0xFFFF)
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when r.absolute_depth < 0 || r.absolute_depth > 0xFFFF", __LINE__);
        base = r.absolute_depth;
    }
    a = {0,
         0,
         std::int32_t(unsigned(b.Width) | (unsigned(b.Height) << 16)),
         int(x),
         int(y),
         left,
         top,
         int(right - left),
         int(bottom - top),
         program.words[13],
         light,
         int(std::min<std::int64_t>(254, 261ll * std::max(r.intensity, 0) / 2048)),
         t.shade_count,
         base,
         gradient,
         first};
    if (program.auxiliary_depth() && r.depth_mode == ShapeDepthMode::legacy) {
        // The SHA frame replaces the gradient, which auxiliary depth does not use.
        a[14] = std::int32_t(auxiliary_entry);
        a[15] = std::int32_t(unsigned(meta.auxiliary_offset.X) | (unsigned(meta.auxiliary_offset.Y) << 16));
    }
    return DrawingStatus::drawn;
}
DrawingStatus decode_type_shape(const ShapeDrawingRequest& r, std::vector<std::uint32_t>& output) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "decode_type_shape", condition, r, __FILE__, line, ordinal);
    };
    output.clear();
    try {
        auto* shape = shape_data(r.image);
        if (!shape) return DrawingStatus::unavailable;
        if (r.frame < 0 || r.frame >= shape->Frames)
            return diagnose(DrawingStatus::invalid_argument, "rejected when r.frame < 0 || r.frame >= shape->Frames",
                            __LINE__);
        const auto original = shape->GetFrameBounds(r.frame);
        auto b = original;
        Point2D offset;
        const auto aux = auxiliary_bounds(r, *shape, b, offset);
        if (aux != DrawingStatus::drawn) return diagnose(aux, "backend_result", __LINE__);
        if (b.Width <= 0 || b.Height <= 0 || b.Width > 4096 || b.Height > 4096)
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when b.Width <= 0 || b.Height <= 0 || b.Width > 4096 || b.Height > 4096",
                            __LINE__);
        std::vector<std::uint8_t> bytes;
        std::string error;
        if (!shape->GetPixels(r.frame) || !decode_shp_pixels(shape->GetPixels(r.frame), original.Width, original.Height,
                                                             shape->HasCompression(r.frame), bytes, error))
            return diagnose(DrawingStatus::invalid_argument,
                            "rejected when !shape->GetPixels(r.frame) || !decode_shp_pixels(shape->GetPixels(r.frame), "
                            "original.Width, original.Height, shape->HasCompression(r.frame), bytes, error)",
                            __LINE__);
        auto* z = shape->HasCompression(r.frame) ? shape_data(r.depth_image) : nullptr;
        const auto* deltas = z ? z->GetPixels(r.depth_frame) : nullptr;
        if (z && !deltas) return DrawingStatus::unavailable;
        const int pitch = z ? z->GetFrameBounds(r.depth_frame).Width : 0;
        output.resize(std::size_t(b.Width) * b.Height);
        for (int y = 0; y < b.Height; ++y)
            for (int x = 0; x < b.Width; ++x) {
                const auto value = bytes[std::size_t(y) * original.Width + x];
                const unsigned delta = deltas ? deltas[std::size_t(y + offset.Y) * pitch + x + offset.X] : 0;
                output[std::size_t(y) * b.Width + x] = (value ? 0x10000u | value : 0) | (delta << 8);
            }
        if (z) {
            // Original RLE prefix clipping advances destination/Z/alpha when
            // a transparent run crosses the clip, but holds the SHA pointer.
            // Preserve each run boundary so the GPU can reproduce that shift
            // without camera-dependent decoding or merging adjacent runs.
            const auto* row = shape->GetPixels(r.frame);
            for (int y = 0; y < b.Height; ++y) {
                const auto* cursor = row + 2;
                int x = 0;
                while (x < original.Width) {
                    if (*cursor++) {
                        ++x;
                        continue;
                    }
                    const int run = *cursor++;
                    for (int j = 1; j < run && x + j < b.Width; ++j)
                        output[std::size_t(y) * b.Width + x + j] |= unsigned(run - j) << 17;
                    x += run;
                }
                row += unsigned(row[0]) | (unsigned(row[1]) << 8);
            }
        }
        return DrawingStatus::drawn;
    } catch (...) {
        output.clear();
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus decode_type_shape_sequence(SHPStruct* image, TypeShapeSequence& output, bool prefix) noexcept {
    output = {};
    try {
        auto* shape = shape_data(image);
        if (!shape) return DrawingStatus::unavailable;
        if (shape->Frames < 0) return DrawingStatus::invalid_argument;
        output.frames.resize(std::size_t(shape->Frames));
        std::vector<std::uint8_t> bytes;
        std::string error;
        for (int index = 0; index < shape->Frames; ++index) {
            auto& frame = output.frames[std::size_t(index)];
            const auto original = shape->GetFrameBounds(index);
            frame.bounds = original;
            frame.rle = shape->HasCompression(index);
            // Same checks and pixels as decode_type_shape for a request without an auxiliary image.
            if (original.Width <= 0 || original.Height <= 0 || original.Width > 4096 || original.Height > 4096) {
                frame.status = DrawingStatus::invalid_argument;
                continue;
            }
            const auto* payload = shape->GetPixels(index);
            if (!payload || !decode_shp_pixels(payload, original.Width, original.Height, frame.rle, bytes, error)) {
                frame.status = DrawingStatus::invalid_argument;
                continue;
            }
            const std::size_t count = std::size_t(original.Width) * original.Height;
            if (bytes.size() < count) {
                frame.status = DrawingStatus::invalid_argument;
                continue;
            }
            if (output.bytes.size() + count * (frame.rle ? 2 : 1) > 0x7FFFFFFFu) return DrawingStatus::unsupported;
            frame.status = DrawingStatus::drawn;
            frame.color = std::uint32_t(output.bytes.size());
            output.bytes.insert(output.bytes.end(), bytes.begin(), bytes.begin() + std::ptrdiff_t(count));
            if (!frame.rle || !prefix) continue;
            frame.prefix = std::uint32_t(output.bytes.size());
            output.bytes.resize(output.bytes.size() + count, 0);
            auto* plane = output.bytes.data() + frame.prefix;
            // Original RLE prefix clipping advances destination/Z/alpha when a
            // transparent run crosses the clip, but holds the SHA pointer.
            // Preserve each run boundary so the GPU can reproduce that shift
            // without camera-dependent decoding or merging adjacent runs.
            const auto* row = payload;
            for (int y = 0; y < original.Height; ++y) {
                const auto* cursor = row + 2;
                int x = 0;
                while (x < original.Width) {
                    if (*cursor++) {
                        ++x;
                        continue;
                    }
                    const int run = *cursor++;
                    for (int j = 1; j < run && x + j < original.Width; ++j)
                        plane[std::size_t(y) * original.Width + x + j] = std::uint8_t(run - j);
                    x += run;
                }
                row += unsigned(row[0]) | (unsigned(row[1]) << 8);
            }
        }
        return DrawingStatus::drawn;
    } catch (...) {
        output = {};
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus decode_type_shape_deltas(SHPStruct* image, TypeShapeSequence& output) noexcept {
    output = {};
    try {
        auto* shape = shape_data(image);
        if (!shape) return DrawingStatus::unavailable;
        if (shape->Frames < 0) return DrawingStatus::invalid_argument;
        output.frames.resize(std::size_t(shape->Frames));
        for (int index = 0; index < shape->Frames; ++index) {
            auto& frame = output.frames[std::size_t(index)];
            frame.bounds = shape->GetFrameBounds(index);
            frame.rle = shape->HasCompression(index);
            if (frame.rle) {
                frame.status = DrawingStatus::unsupported;
                continue;
            }
            const auto* deltas = shape->GetPixels(index);
            if (!deltas) {
                frame.status = DrawingStatus::unavailable;
                continue;
            }
            if (frame.bounds.Width <= 0 || frame.bounds.Height <= 0 || frame.bounds.Width > 4096 ||
                frame.bounds.Height > 4096) {
                frame.status = DrawingStatus::invalid_argument;
                continue;
            }
            const std::size_t count = std::size_t(frame.bounds.Width) * frame.bounds.Height;
            if (output.bytes.size() + count > 0x7FFFFFFFu) return DrawingStatus::unsupported;
            frame.status = DrawingStatus::drawn;
            frame.color = std::uint32_t(output.bytes.size());
            output.bytes.insert(output.bytes.end(), deltas, deltas + count);
        }
        return DrawingStatus::drawn;
    } catch (...) {
        output = {};
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus prepare_type_shape(const ShapeDrawingRequest& r, const TypeGpuTarget& t, TypeGpuPacket& p) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_shape", condition, r, __FILE__, line, ordinal);
    };
    auto result = prepare_type_shape_parameters(r, t, p);
    if (result != DrawingStatus::drawn) return diagnose(result, "backend_result", __LINE__);
    result = decode_type_shape(r, p.texels);
    if (result != DrawingStatus::drawn) p = {};
    return diagnose(result, "backend_result", __LINE__);
}
bool type_gpu_packet_visible(const TypeGpuPacket& packet) noexcept { return visible(packet); }
DrawingStatus describe_type_tile(const TileDrawingRequest& r, TypeTileMeta& meta) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "describe_type_tile", condition, r, __FILE__, line, ordinal);
    };
    meta = {};
    if (r.flat || r.flag16 || r.flag17) return DrawingStatus::unsupported;
    if (!r.resource || !r.image)
        return diagnose(DrawingStatus::invalid_argument, "rejected when !r.resource || !r.image", __LINE__);
    if (r.resource->Width != 60 || r.resource->Height != 30) return DrawingStatus::unsupported;
    const auto* im = r.image;
    meta.flags = im->Flags;
    meta.extra_x = std::int64_t(im->ExtraX) - im->X;
    meta.extra_y = std::int64_t(im->ExtraY) - im->Y;
    meta.extra_width = im->ExtraWidth;
    meta.extra_height = im->ExtraHeight;
    return DrawingStatus::drawn;
}
DrawingStatus prepare_type_tile_parameters(const TileDrawingRequest& r, const TypeGpuTarget& t, TypeGpuPacket& base,
                                           TypeGpuPacket& extra) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_tile_parameters", condition, r, __FILE__, line, ordinal);
    };
    base = {};
    extra = {};
    TypeTileMeta meta;
    const auto described = describe_type_tile(r, meta);
    if (described != DrawingStatus::drawn) return diagnose(described, "backend_result", __LINE__);
    return prepare_type_tile_parameters(r, meta, t, base, extra);
}
DrawingStatus prepare_type_tile_parameters(const TileDrawingRequest& r, const TypeTileMeta& im, const TypeGpuTarget& t,
                                           TypeGpuPacket& base, TypeGpuPacket& extra) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_tile_parameters", condition, r, __FILE__, line, ordinal);
    };
    base = {};
    extra = {};
    if (!setup(base, t, r.clip, r.position.X, r.position.Y, 60, 29, r.intensity, true))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !setup(base, t, r.clip, r.position.X, r.position.Y, 60, 29, r.intensity, true)",
                        __LINE__);
    const bool depth = r.use_depth && (im.flags & 2);
    const auto z = depth
                       ? std::int64_t(std::uint16_t(std::int64_t(t.depth_bounds_y) + t.depth_max - r.position.Y - 30)) -
                             15ll * r.level
                       : 0;
    if (!integer(z) || !integer(z + 255))
        return diagnose(DrawingStatus::invalid_argument, "rejected when !integer(z) || !integer(z + 255)", __LINE__);
    base.parameters[13] = depth;
    base.parameters[14] = int(z);
    base.parameters[16] = r.buffer_offset_y % t.height;
    if (im.flags & 1) {
        if (!setup(extra, t, r.clip, std::int64_t(r.position.X) + im.extra_x, std::int64_t(r.position.Y) + im.extra_y,
                   im.extra_width, im.extra_height, r.intensity, true))
            return diagnose(
                DrawingStatus::invalid_argument,
                "rejected when !setup(extra, t, r.clip, std::int64_t(r.position.X) + im.extra_x, "
                "std::int64_t(r.position.Y) + im.extra_y, im.extra_width, im.extra_height, r.intensity, true)",
                __LINE__);
        extra.parameters[13] = depth;
        extra.parameters[14] = int(z);
        extra.parameters[15] = 1;
        extra.parameters[16] = r.buffer_offset_y % t.height;
    }
    return diagnose(visible(base) || visible(extra) ? DrawingStatus::drawn : DrawingStatus::skipped, "backend_result",
                    __LINE__);
}
DrawingStatus decode_type_tile(const TileDrawingRequest& r, std::vector<std::uint32_t>& base,
                               std::vector<std::uint32_t>& extra) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "decode_type_tile", condition, r, __FILE__, line, ordinal);
    };
    base.clear();
    extra.clear();
    if (!r.resource || !r.image)
        return diagnose(DrawingStatus::invalid_argument, "rejected when !r.resource || !r.image", __LINE__);
    if (r.resource->Width != 60 || r.resource->Height != 30) return DrawingStatus::unsupported;
    try {
        const auto* im = r.image;
        const bool depth = r.use_depth && (im->Flags & 2);
        base.assign(60 * 29, 0);
        int source = 0;
        for (int y = 0; y < 29; ++y) {
            const int width = 4 * (y < 15 ? y + 1 : 29 - y), left = (60 - width) / 2;
            for (int x = 0; x < width; ++x, ++source) {
                const unsigned dz = depth ? im->At(im->ZOffset)[source] : 0;
                base[std::size_t(y) * 60 + left + x] = 0x10000u | im->Pixels()[source] | (dz << 8);
            }
        }
        if (im->Flags & 1) {
            if (im->ExtraWidth < 1 || im->ExtraHeight < 1 || im->ExtraWidth > 4096 || im->ExtraHeight > 4096)
                return diagnose(DrawingStatus::invalid_argument,
                                "rejected when im->ExtraWidth < 1 || im->ExtraHeight < 1 || im->ExtraWidth > 4096 || "
                                "im->ExtraHeight > 4096",
                                __LINE__);
            extra.resize(std::size_t(im->ExtraWidth) * im->ExtraHeight);
            for (std::size_t i = 0; i < extra.size(); ++i) {
                const auto index = im->At(im->ExtraOffset)[i];
                const unsigned dz = depth ? im->At(im->ExtraZOffset)[i] : 0;
                extra[i] = index ? (0x10000u | index | (dz << 8)) : 0;
            }
        }
        return DrawingStatus::drawn;
    } catch (...) {
        base.clear();
        extra.clear();
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
DrawingStatus prepare_type_tile(const TileDrawingRequest& r, const TypeGpuTarget& t, TypeGpuPacket& base,
                                TypeGpuPacket& extra) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_tile", condition, r, __FILE__, line, ordinal);
    };
    auto status = prepare_type_tile_parameters(r, t, base, extra);
    if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
    status = decode_type_tile(r, base.texels, extra.texels);
    if (status != DrawingStatus::drawn) {
        base = {};
        extra = {};
        return diagnose(status, "backend_result", __LINE__);
    }
    if (!visible(base)) base.texels.clear();
    if (!visible(extra)) extra.texels.clear();
    return DrawingStatus::drawn;
}
}

namespace game {
DrawingStatus prepare_type_indexed_parameters(const IndexedDrawingRequest& r, const TypeGpuTarget& t,
                                              TypeGpuPacket& out) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_indexed_parameters", condition, r, __FILE__, line,
                                       ordinal);
    };
    out.parameters = {};
    if (t.width <= 0 || t.height <= 0 || t.shade_count < 1 || t.shade_count > 256 || r.width < 0 || r.height < 0 ||
        r.width > 2048 || r.height > 2048 ||
        static_cast<unsigned>(r.depth_mode) > static_cast<unsigned>(ShapeDepthMode::read_write))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when t.width <= 0 || t.height <= 0 || t.shade_count < 1 || t.shade_count > 256 || "
                        "r.width < 0 || r.height < 0 || r.width > 2048 || r.height > 2048 || "
                        "static_cast<unsigned>(r.depth_mode) > static_cast<unsigned>(ShapeDepthMode::read_write)",
                        __LINE__);
    if (!r.width || !r.height || r.clip.Width <= 0 || r.clip.Height <= 0) return DrawingStatus::skipped;
    if (!r.voxel && (!r.pixels || std::uint64_t(r.width) * r.height > r.pixel_count))
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !r.voxel && (!r.pixels || std::uint64_t(r.width) * r.height > r.pixel_count)",
                        __LINE__);
    out.parameters = {t.width,
                      t.height,
                      r.width,
                      r.height,
                      r.position.X,
                      r.position.Y,
                      r.clip.X,
                      r.clip.Y,
                      r.clip.Width,
                      r.clip.Height,
                      1,
                      int(std::clamp(std::int64_t(261) * r.intensity / 2048, std::int64_t(0), std::int64_t(254))),
                      t.shade_count,
                      (r.depth_mode == ShapeDepthMode::none   ? 0
                       : r.depth_mode == ShapeDepthMode::read ? 2
                                                              : 1) |
                          (r.shadow ? 256 : 0),
                      r.absolute_depth,
                      1,
                      0,
                      0,
                      0,
                      3};
    if (r.depth_mode == ShapeDepthMode::legacy) {
        // 0x00706640 -> 0x004AF2A0 -> 0x004373B0 uses the same plain
        // bitmap row dispatcher as an uncompressed SHP. Internal VXL Z stays local.
        ShapeDrawingRequest bitmap;
        bitmap.flags = r.flags | (r.shadow ? 1u : 0u);
        bitmap.tint = int(r.tint);
        bitmap.gradient = r.gradient;
        bitmap.depth_adjustment = r.absolute_depth;
        if (!original_shape_mode(bitmap, t, false, out)) return DrawingStatus::unsupported;
    }
    return diagnose(type_gpu_packet_visible(out) ? DrawingStatus::drawn : DrawingStatus::skipped, "backend_result",
                    __LINE__);
}
}

namespace game {
DrawingStatus prepare_type_mask(const MaskDrawingRequest& r, const TypeGpuTarget& t, TypeGpuPacket& packet) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_type_mask", condition, r, __FILE__, line, ordinal);
    };
    packet = {};
    if (r.width > 4096 || r.height > 4096) return DrawingStatus::unsupported;
    if (!r.bits || r.width <= 0 || r.height <= 0 || r.pitch < (r.width + 7) / 8 ||
        std::uint64_t(r.height - 1) * r.pitch + (r.width + 7) / 8 > r.byte_count)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when !r.bits || r.width <= 0 || r.height <= 0 || r.pitch < (r.width + 7) / 8 || "
                        "std::uint64_t(r.height - 1) * r.pitch + (r.width + 7) / 8 > r.byte_count",
                        __LINE__);
    if (!setup(packet, t, r.clip, r.position.X, r.position.Y, r.width, r.height, 1000, false))
        return diagnose(
            DrawingStatus::invalid_argument,
            "rejected when !setup(packet, t, r.clip, r.position.X, r.position.Y, r.width, r.height, 1000, false)",
            __LINE__);
    if (!visible(packet)) return DrawingStatus::skipped;
    return DrawingStatus::drawn;
}
DrawingStatus describe_type_shape_strip(const ShapeStripDrawingRequest& r, TypeShapeMeta& meta) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "describe_type_shape_strip", condition, r, __FILE__, line, ordinal);
    };
    if (r.count <= 0 || r.step.X < 0 || r.step.Y < 0)
        return diagnose(DrawingStatus::invalid_argument, "rejected when r.count <= 0 || r.step.X < 0 || r.step.Y < 0",
                        __LINE__);
    auto status = describe_type_shape(r.shape, meta);
    if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
    const auto width = std::int64_t(meta.body.Width) + std::int64_t(r.count - 1) * r.step.X;
    const auto height = std::int64_t(meta.body.Height) + std::int64_t(r.count - 1) * r.step.Y;
    // Oversized strips expand through the core's original shape fallback.
    if (width > 4096 || height > 4096 || width * height > 1048576) return DrawingStatus::unsupported;
    meta.body.Width = int(width);
    meta.body.Height = int(height);
    meta.rle = false;
    return DrawingStatus::drawn;
}
DrawingStatus decode_type_shape_strip(const ShapeStripDrawingRequest& r, std::vector<std::uint32_t>& output) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "decode_type_shape_strip", condition, r, __FILE__, line, ordinal);
    };
    output.clear();
    try {
        TypeShapeMeta combined, original;
        auto status = describe_type_shape_strip(r, combined);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        status = describe_type_shape(r.shape, original);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        std::vector<std::uint32_t> source;
        status = decode_type_shape(r.shape, source);
        if (status != DrawingStatus::drawn) return diagnose(status, "backend_result", __LINE__);
        output.resize(std::size_t(combined.body.Width) * combined.body.Height);
        for (int i = 0; i < r.count; ++i)
            for (int y = 0; y < original.body.Height; ++y)
                for (int x = 0; x < original.body.Width; ++x) {
                    const auto pixel = source[std::size_t(y) * original.body.Width + x];
                    // Last covered copy wins; transparent pixels never erase prior copies.
                    if (pixel & 0x10000u)
                        output[std::size_t(y + i * r.step.Y) * combined.body.Width + x + i * r.step.X] =
                            pixel & 0x100FFu;
                }
        return DrawingStatus::drawn;
    } catch (...) {
        output.clear();
        return diagnose(DrawingStatus::backend_failure, "exception_or_backend_failure", __LINE__);
    }
}
}
