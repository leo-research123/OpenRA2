#include "api/drawing_diagnostics.hpp"
#include "lighting_batch.hpp"
#include "yrpp/FileFormats/SHP.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace game {
DrawingStatus decode_lighting_image(SHPStruct* image, int frame, LightingImageData& output) noexcept {
    output = {};
    try {
        if (!image) return DrawingStatus::unavailable;
        if (auto* ref = image->AsReference()) {
            ref->Load();
            image = ref->Data;
        }
        if (!image) return DrawingStatus::unavailable;
        if (frame < 0 || frame >= image->Frames) {
            char detail[256];
            std::snprintf(detail, sizeof(detail), "frame=%d expected_frame=[0,%d)", frame, int(image->Frames));
            return record_drawing_diagnostic(DrawingStatus::invalid_argument, "decode_lighting_image",
                                              "lighting_frame_out_of_range", detail, __FILE__, __LINE__);
        }
        const auto rect = image->GetFrameBounds(frame);
        if (rect.Width <= 0 || rect.Height <= 0) return DrawingStatus::skipped;
        if (rect.Width > 8192 || rect.Height > 8192) return DrawingStatus::invalid_argument;
        const auto* pixels = image->GetPixels(frame);
        if (!pixels) return DrawingStatus::unavailable;
        output.bounds = rect;
        output.texels.resize(std::size_t(rect.Width) * rect.Height);
        // Original lighting frames are byte samples; zero is covered/dark,
        // unlike the transparency convention of ordinary palette SHP drawing.
        for (std::size_t i = 0; i < output.texels.size(); ++i)
            output.texels[i] = 0x10000u | pixels[i];
        return DrawingStatus::drawn;
    } catch (...) {
        output = {};
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus prepare_lighting_parameters(const LightingShapeDrawingRequest& r, const RectangleStruct& image, int width,
                                          int height, LightingParameters& p) noexcept {
    const auto ordinal = next_drawing_request();
    const auto diagnose = [&](DrawingStatus status, const char* condition, unsigned line) noexcept {
        return diagnose_drawing_result(status, "prepare_lighting_parameters", condition, r, __FILE__, line, ordinal);
    };
    p = {};
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when width <= 0 || height <= 0 || width > 8192 || height > 8192", __LINE__);
    const auto x = std::int64_t(r.position.X) + image.X, y = std::int64_t(r.position.Y) + image.Y;
    if (x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX)
        return diagnose(DrawingStatus::invalid_argument,
                        "rejected when x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX", __LINE__);
    const auto left = std::max({std::int64_t(0), x, std::int64_t(r.clip.X)});
    const auto top = std::max({std::int64_t(0), y, std::int64_t(r.clip.Y)});
    const auto right = std::min({std::int64_t(width), x + image.Width, std::int64_t(r.clip.X) + r.clip.Width});
    const auto bottom = std::min({std::int64_t(height), y + image.Height, std::int64_t(r.clip.Y) + r.clip.Height});
    if (right <= left || bottom <= top) return DrawingStatus::skipped;
    int operation = 0;
    if (r.operation == RasterBlendMode::shroud)
        operation = 1;
    else if (r.operation == RasterBlendMode::fog)
        operation = 2;
    else if (r.operation == RasterBlendMode::alpha_shape)
        operation = 3;
    else
        return diagnose(DrawingStatus::invalid_argument, "rejected when r.operation == RasterBlendMode::alpha_shape",
                        __LINE__);
    p = {width,  height,    image.Width, image.Height,      int(x),
         int(y), int(left), int(top),    int(right - left), int(bottom - top)};
    p[19] = operation;
    return DrawingStatus::drawn;
}
bool plan_light_field(const LightingShapeBatchRequest& r, LightFieldPlan& plan) noexcept {
    plan = LightFieldPlan{};
    try {
        std::uint32_t n = 0;
        while (n < r.count && r.entries[n].cell.X >= 0)
            ++n;
        for (std::uint32_t i = n; i < r.count; ++i)
            if (r.entries[i].cell.X >= 0 || r.entries[i].request.operation != RasterBlendMode::alpha_shape)
                return false;
        plan.first_other = n;
        if (!n) return true;
        int a0 = INT32_MAX, a1 = INT32_MIN, s0 = INT32_MAX, s1 = INT32_MIN;
        for (std::uint32_t i = 0; i < n; ++i) {
            const auto& e = r.entries[i];
            const auto& q = e.request;
            if (q.operation != RasterBlendMode::shroud || q.image != r.entries[0].request.image || q.frame < 0 ||
                q.frame >= 4096)
                return false;
            const int a = e.cell.X - e.cell.Y, s = e.cell.X + e.cell.Y;
            // DrawShroud: position = footprint origin + view origin; the clip
            // lies inside the cell's footprint (view.X does not shift it).
            const std::int64_t fx = std::int64_t(q.position.X) - r.view.X, fy = q.position.Y;
            const std::int64_t kx = fx - 30ll * a, ky = fy - 15ll * s;
            if (kx < INT32_MIN || kx > INT32_MAX || ky < INT32_MIN || ky > INT32_MAX) return false;
            if (i == 0) {
                plan.kx = int(kx);
                plan.ky = int(ky);
            } else if (kx != plan.kx || ky != plan.ky)
                return false;
            if (q.clip.X < fx || q.clip.Y < fy || std::int64_t(q.clip.X) + q.clip.Width > fx + 60 ||
                std::int64_t(q.clip.Y) + q.clip.Height > fy + 30)
                return false;
            a0 = std::min(a0, a);
            a1 = std::max(a1, a);
            s0 = std::min(s0, s);
            s1 = std::max(s1, s);
            plan.frames = std::max(plan.frames, q.frame + 1);
        }
        if (std::int64_t(a1 - a0 + 1) * (s1 - s0 + 1) > (1 << 22)) return false;
        plan.a0 = a0;
        plan.s0 = s0;
        plan.columns = a1 - a0 + 1;
        plan.rows = s1 - s0 + 1;
        plan.grid.assign(std::size_t(plan.columns) * plan.rows, -1);
        plan.cells.reserve(std::size_t(n) * LightFieldCellWords);
        plan.cell_image = r.entries[0].request.image;
        for (std::uint32_t i = 0; i < n; ++i) {
            const auto& e = r.entries[i];
            const auto& q = e.request;
            auto& slot =
                plan.grid[std::size_t(e.cell.X + e.cell.Y - plan.s0) * plan.columns + (e.cell.X - e.cell.Y - plan.a0)];
            if (slot >= 0) return false;  // one request per cell
            slot = std::int32_t(i);
            plan.cells.insert(plan.cells.end(),
                              {q.clip.X, q.clip.Y, q.clip.Width, q.clip.Height, q.position.X, q.position.Y, q.frame});
        }
        return true;
    } catch (...) {
        plan = LightFieldPlan{};
        return false;
    }
}
}
