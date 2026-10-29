// Project line, ellipse and gradient submission to the drawing scope. The
// original DSurface primitives (DrawLineColor 0x004BFD30, DrawBlendedLine
// 0x004BEAC0, 0x004BDF00, 0x004BC750, gradient 0x004BF750, XSurface ellipse
// 0x007BB350) write a locked 16-bit surface; here they become clipped raster
// requests. Endpoints are clipped by Line_In_Bounds (0x007BC2B0).
#include "surface_drawing.hpp"
#include "type_drawing.hpp"
#include "yrpp/Surface.h"
#include "yrpp/YRMath.h"
#include "x87_double.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace game {
namespace {
int ellipse_add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
int ellipse_sub(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
int ellipse_mul(int a, int b) { return std::bit_cast<int>(unsigned(a) * unsigned(b)); }
struct LineStyle {
    WORD color = 0;
    bool glow = false;
    int strength = 0;
    const ColorStruct* alpha = nullptr;
    int opacity = 255;
    bool additive = false;
    int addRed = 0, addGreen = 0, addBlue = 0;
};

int Add(int a, int b) noexcept { return std::bit_cast<std::int32_t>(std::uint32_t(a) + std::uint32_t(b)); }

int Sub(int a, int b) noexcept { return std::bit_cast<std::int32_t>(std::uint32_t(a) - std::uint32_t(b)); }

// Length of a segment as the original measures it (0x004C1B50 for the blended line, inline in 0x004BFD30):
// the game square root of the sum of squares, truncated to an integer.
int SegmentLength(Point2D p, Point2D q) noexcept {
    const double dx = Sub(p.X, q.X), dy = Sub(p.Y, q.Y);
    return x87_integer(Math::sqrt(x87_add(x87_multiply(dy, dy), x87_multiply(dx, dx))));
}

// Magnitude of the depth change over the part of the line that remains: |ftol(part / whole * delta)|.
// The ratio is formed first and both operations truncate (x87 control word 0x0E7F); the product of
// delta and part divided by whole differs in the last bit and can differ by one in the result.
int DepthShare(int part, int whole, int delta) noexcept {
    const int share = x87_integer(x87_multiply(x87_divide(part, whole), delta));
    return share < 0 ? Sub(0, share) : share;
}

// Draws one clipped line as per-pixel requests (0x004BFD30). Endpoints are sorted by X before
// clipping; when clipping moves an end its depth is re-derived from the segment lengths.
DrawingStatus PlotDepthLine(const TypeDrawingContext& context, const RectangleStruct& clip, Point2D a, Point2D b,
                            int za, int zb, const LineStyle& style) {
    // The original moves both ends by the origin of the clip rectangle first (wrapping) and then sorts and
    // clips in surface coordinates.
    a = {Add(a.X, clip.X), Add(a.Y, clip.Y)};
    b = {Add(b.X, clip.X), Add(b.Y, clip.Y)};
    if (a.X > b.X) {
        std::swap(a, b);
        std::swap(za, zb);
    }
    const Point2D first = a, last = b;
    const int zfirst = za, zlast = zb;
    RectangleStruct area = clip;
    if (!Line_In_Bounds(&a, &b, &area)) return DrawingStatus::skipped;
    // A moved end takes its depth from the other end plus the share of the whole depth change that lies
    // between them. The division is not guarded: a zero length yields an infinite ratio, which converts to 0.
    if (b != last) {
        const int share = DepthShare(SegmentLength(first, b), SegmentLength(first, last), Sub(zfirst, zlast));
        zb = zfirst >= zlast ? Sub(zfirst, share) : Add(zfirst, share);
    }
    if (a != first) {
        const int share = DepthShare(SegmentLength(a, last), SegmentLength(first, last), Sub(zfirst, zlast));
        za = zlast <= zfirst ? Add(zlast, share) : Sub(zlast, share);
    }
    const int lightX = a.X;
    const int dx = b.X - a.X, dy = std::abs(b.Y - a.Y), dz = std::abs(zb - za);
    const int sy = b.Y >= a.Y ? 1 : -1, sz = zb >= za ? 1 : -1;
    const int major = dz > dx && dz > dy ? 2 : dx > dy ? 0 : 1;
    const int n = major == 0 ? dx : major == 1 ? dy : dz;
    if (n > 65536) return DrawingStatus::invalid_argument;
    // Original >0 error rule; the terminal endpoint is NOT drawn.
    int ex = 2 * dx - n, ey = 2 * dy - n, ez = 2 * dz - n;
    DrawingStatus result = DrawingStatus::skipped;
    for (int i = 0; i < n; ++i) {
        RasterDrawingRequest request;
        request.target = context.target;
        request.clip = clip;
        request.position = a;
        request.width = request.height = 1;
        request.color = style.color;
        request.light_strength = style.strength;
        request.original_line = !style.glow;
        request.blend_mode = style.glow ? RasterBlendMode::depth_glow : RasterBlendMode::copy;
        request.line_z = za;
        if (style.alpha) {
            request.original_line = false;
            request.blend_mode = style.additive ? RasterBlendMode::depth_additive : RasterBlendMode::depth_alpha;
            request.line_rgb =
                unsigned(style.alpha->R) | (unsigned(style.alpha->G) << 8) | (unsigned(style.alpha->B) << 16);
            request.line_opacity = style.opacity;
            if (style.additive) {
                request.line_light_x = lightX;
                request.line_red = style.addRed;
                request.line_green = style.addGreen;
                request.line_blue = style.addBlue;
            }
        }
        const auto status = submit_type_raster(context, request);
        if (status != DrawingStatus::drawn && status != DrawingStatus::skipped) return status;
        if (status == DrawingStatus::drawn) result = status;
        if (major == 0 || ex > 0) {
            ++a.X;
            ex -= 2 * n;
        }
        ex += 2 * dx;
        if (major == 1 || ey > 0) {
            a.Y += sy;
            ey -= 2 * n;
        }
        ey += 2 * dy;
        if (major == 2 || ez > 0) {
            za += sz;
            ez -= 2 * n;
        }
        ez += 2 * dz;
    }
    return result;
}

bool Finish(PresentationContext& ctx, DrawingStatus status) noexcept {
    record_type_drawing_result(ctx, status);
    return status == DrawingStatus::drawn;
}
float store_float(double value) noexcept {
    const float nearest = static_cast<float>(value);
    return std::abs(double(nearest)) > std::abs(value) ? std::bit_cast<float>(std::bit_cast<unsigned>(nearest) - 1u)
                                                       : nearest;
}
}
bool submit_ellipse(PresentationContext& ctx, const RectangleStruct& clip, Point2D center, int rx, int ry, WORD color) noexcept {
    const auto* drawing = ctx.type;
    if (!drawing) return false;
    if (rx < 0 || ry < 0) return true;
    // Prevent unbounded work for malformed native presentation requests.
    if (rx > 32767 || ry > 32767) {
        record_type_drawing_result(ctx, DrawingStatus::invalid_argument);
        return false;
    }
    const auto pixel = [&](int x, int y) {
        if (x < clip.X || y < clip.Y || x >= clip.X + clip.Width || y >= clip.Y + clip.Height) return true;
        RasterDrawingRequest r;
        r.target = drawing->target;
        r.clip = clip;
        r.position = {x, y};
        r.width = r.height = 1;
        r.color = color;
        const auto status = submit_type_raster(*drawing, r);
        record_type_drawing_result(ctx, status);
        return drawing_completed(status);
    };
    const auto four = [&](int x, int y) {
        return pixel(ellipse_add(center.X, x), ellipse_add(center.Y, y)) &&
               pixel(ellipse_sub(center.X, x), ellipse_sub(center.Y, y)) &&
               pixel(ellipse_add(center.X, x), ellipse_sub(center.Y, y)) &&
               pixel(ellipse_sub(center.X, x), ellipse_add(center.Y, y));
    };
    if (!pixel(center.X, ellipse_add(center.Y, ry)) || !pixel(center.X, ellipse_sub(center.Y, ry))) return false;
    const int xsq = ellipse_mul(rx, rx), ysq = ellipse_mul(ry, ry);
    int x = 0, y = ry, dx = 0, dy = ellipse_mul(ellipse_mul(2, ry), xsq),
        error = ellipse_sub(xsq / 4, ellipse_mul(ry, xsq));
    do {
        error = ellipse_add(error, ellipse_add(ysq, dx));
        if (error >= 0) {
            dy = ellipse_sub(dy, ellipse_mul(2, xsq));
            error = ellipse_sub(error, dy);
            --y;
        }
        dx = ellipse_add(dx, ellipse_mul(2, ysq));
        ++x;
        if (dx >= dy) break;
        if (!four(x, y)) return false;
    } while (x <= rx);
    if (!pixel(ellipse_add(center.X, rx), center.Y) || !pixel(ellipse_sub(center.X, rx), center.Y)) return false;
    x = rx;
    y = 0;
    dx = ellipse_mul(ellipse_mul(2, rx), ysq);
    dy = 0;
    error = ellipse_sub(ysq / 4, ellipse_mul(rx, ysq));
    do {
        error = ellipse_add(error, ellipse_add(xsq, dy));
        if (error >= 0) {
            dx = ellipse_sub(dx, ellipse_mul(2, ysq));
            error = ellipse_sub(error, dx);
            --x;
        }
        dy = ellipse_add(dy, ellipse_mul(2, xsq));
        ++y;
        if (dy > dx) break;
        if (!four(x, y)) return false;
    } while (y <= ry);
    return true;
}
bool submit_blended_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D a, Point2D b, ColorStruct color, int opacity, int za,
                         int zb) noexcept {
    const auto* drawing = ctx.type;
    if (!drawing) return false;
    if (opacity < 8) return Finish(ctx, DrawingStatus::skipped);
    if (opacity > 255 || clip.Width < 0 || clip.Height < 0 || clip.Width > 8192 || clip.Height > 8192)
        return Finish(ctx, DrawingStatus::invalid_argument);
    if (!clip.Width || !clip.Height) return Finish(ctx, DrawingStatus::skipped);
    try {
        LineStyle style;
        style.alpha = &color;
        style.opacity = opacity;
        return Finish(ctx, PlotDepthLine(*drawing, clip, a, b, za, zb, style));
    } catch (...) {
        return Finish(ctx, DrawingStatus::backend_failure);
    }
}

bool submit_color_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D a, Point2D b, WORD color, int za, int zb) noexcept {
    const auto* drawing = ctx.type;
    if (!drawing) return false;
    try {
        LineStyle style;
        style.color = color;
        return Finish(ctx, PlotDepthLine(*drawing, clip, a, b, za, zb, style));
    } catch (...) {
        return Finish(ctx, DrawingStatus::backend_failure);
    }
}

bool submit_glow_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D a, Point2D b, int za, int zb, int strength) noexcept {
    const auto* drawing = ctx.type;
    if (!drawing) return false;
    try {
        // BuildingLight clips to TacticalRect before invoking DSurface with its
        // full surface rectangle: clipping here does not interpolate Z.
        RectangleStruct area{0, 0, clip.Width, clip.Height};
        if (!Line_In_Bounds(&a, &b, &area)) return Finish(ctx, DrawingStatus::skipped);
        LineStyle style;
        style.glow = true;
        style.strength = strength;
        return Finish(ctx, PlotDepthLine(*drawing, clip, a, b, za + clip.Y, zb + clip.Y, style));
    } catch (...) {
        return Finish(ctx, DrawingStatus::backend_failure);
    }
}

// 0x004BDF00: quantize the source before geometry and add it to the destination.
bool submit_rgb_multiplying_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D a, Point2D b, ColorStruct color, float intensity,
                                 int za, int zb) noexcept {
    const auto* drawing = ctx.type;
    if (!drawing) return false;
    const int r = x87_integer(double(color.R) * intensity), g = x87_integer(double(color.G) * intensity),
              blue = x87_integer(double(color.B) * intensity);
    if (r < 8 && g < 8 && blue < 8) return Finish(ctx, DrawingStatus::skipped);
    // Keep the full quantized channels; the RGB helper receives integers.
    const ColorStruct source{BYTE(r), BYTE(g), BYTE(blue)};
    try {
        LineStyle style;
        style.alpha = &source;
        style.additive = true;
        style.addRed = r;
        style.addGreen = g;
        style.addBlue = blue;
        return Finish(ctx, PlotDepthLine(*drawing, clip, a, b, za, zb, style));
    } catch (...) {
        return Finish(ctx, DrawingStatus::backend_failure);
    }
}

// 0x004BC750 uses a cubic coverage table and a paired destination read. Its
// terminal endpoint is excluded, and fractional stepping advances on >256.
bool submit_subtractive_line(PresentationContext& ctx, const RectangleStruct& clip, Point2D a, Point2D b, ColorStruct color, int za, int zb,
                             bool red, bool green, bool blue, float intensity) noexcept {
    const auto* drawing = ctx.type;
    if (!drawing) return false;
    try {
        // The original reserves one pixel at the surface border before
        // intersecting the caller's clip. A smaller interior clip is unchanged.
        const auto surface = DSurface::ViewBounds.Width > 0 ? DSurface::ViewBounds : clip;
        const int left = std::max(clip.X, surface.X + 1), top = std::max(clip.Y, surface.Y + 1);
        RectangleStruct area{left, top,
                             std::max(0, std::min(clip.X + clip.Width, surface.X + surface.Width - 1) - left),
                             std::max(0, std::min(clip.Y + clip.Height, surface.Y + surface.Height - 1) - top)};
        if (!area.Width && !area.Height) return Finish(ctx, DrawingStatus::skipped);
        a = {Add(a.X, area.X), Add(a.Y, area.Y)};
        b = {Add(b.X, area.X), Add(b.Y, area.Y)};
        if (a.X > b.X) {
            std::swap(a, b);
            std::swap(za, zb);
        }
        const auto first = a, last = b;
        const int firstZ = za, lastZ = zb;
        if (!Line_In_Bounds(&a, &b, &area)) return Finish(ctx, DrawingStatus::skipped);
        if (b != last) {
            const int share = DepthShare(SegmentLength(first, b), SegmentLength(first, last), Sub(firstZ, lastZ));
            zb = firstZ >= lastZ ? Sub(firstZ, share) : Add(firstZ, share);
        }
        if (a != first) {
            const int share = DepthShare(SegmentLength(a, last), SegmentLength(first, last), Sub(firstZ, lastZ));
            za = lastZ <= firstZ ? Add(lastZ, share) : Sub(lastZ, share);
        }
        const int dx = b.X - a.X, dy = std::abs(b.Y - a.Y), dz = std::abs(zb - za);
        const bool vertical = dx <= dy;
        const int n = vertical ? dy : dx;
        if (!n) return Finish(ctx, DrawingStatus::skipped);
        if (n > 65536) return Finish(ctx, DrawingStatus::invalid_argument);
        const int sy = b.Y >= a.Y ? 1 : -1, sz = zb >= za ? 1 : -1;
        const int step = x87_integer(x87_multiply(x87_divide(vertical ? dx : dy, n), 256));
        int fraction = 0, depthError = 2 * dz - n;
        DrawingStatus result = DrawingStatus::skipped;
        const bool depthMajor = dz > dx && dz > dy;
        const int iterations = depthMajor && !vertical ? (n + 1) / 2 : n;
        const int sampleX = a.X;
        for (int i = 0; i < iterations; ++i) {
            RasterDrawingRequest request;
            request.target = drawing->target;
            request.clip = area;
            request.position = a;
            request.width = request.height = 1;
            request.blend_mode = RasterBlendMode::depth_laser_pair;
            request.line_rgb = unsigned(color.R) | (unsigned(color.G) << 8) | (unsigned(color.B) << 16);
            request.line_z = za;
            if (depthMajor && !vertical) request.line_sample_x = Add(sampleX, i);
            request.line_channels = unsigned(red) | (unsigned(green) << 1) | (unsigned(blue) << 2);
            request.line_neighbor = vertical ? Point2D{1, 0} : Point2D{0, sy};
            request.line_weight = x87_integer(double(256 - fraction) * intensity);
            request.line_neighbor_weight = x87_integer(double(fraction) * intensity);
            // The original horizontal path swaps destination source samples.
            if (!vertical) request.spotlight_flags = 1;
            const auto status = submit_type_raster(*drawing, request);
            if (status != DrawingStatus::drawn && status != DrawingStatus::skipped) return Finish(ctx, status);
            if (status == DrawingStatus::drawn) result = status;
            fraction += step;
            if (fraction > 256) {
                fraction -= 256;
                if (vertical)
                    ++a.X;
                else
                    a.Y += sy;
            }
            if (depthMajor) {
                za += sz;
            } else {
                if (depthError > 0) {
                    za += sz;
                    depthError -= 2 * n;
                }
                depthError += 2 * dz;
            }
            if (vertical)
                a.Y += sy;
            else
                a.X += depthMajor ? 2 : 1;
        }
        return Finish(ctx, result);
    } catch (...) {
        return Finish(ctx, DrawingStatus::backend_failure);
    }
}
DrawingStatus submit_gradient_line(const TypeDrawingContext& drawing, const RectangleStruct& clip, Point2D start,
                                   Point2D end, ColorStruct first, ColorStruct last, float& step,
                                   float& phase) noexcept {
    if (!std::isfinite(step) || !std::isfinite(phase) || clip.Width < 0 || clip.Height < 0 || clip.Width > 8192 ||
        clip.Height > 8192 || std::int64_t(clip.X) + clip.Width > INT32_MAX ||
        std::int64_t(clip.Y) + clip.Height > INT32_MAX)
        return DrawingStatus::invalid_argument;
    RectangleStruct area{0, 0, clip.Width, clip.Height};
    if (!clip.Width || !clip.Height || !Line_In_Bounds(&start, &end, &area)) return DrawingStatus::skipped;
    if (start.X > end.X) std::swap(start, end);
    const int dx = end.X - start.X, dy = std::abs(end.Y - start.Y), direction = end.Y >= start.Y ? 1 : -1;
    const bool horizontal = dx > dy;
    const int length = std::max(dx, dy);
    int error = horizontal ? 2 * dy - dx : 2 * dx - dy;
    for (int i = 0; i <= length; ++i) {
        const auto channel = [&](byte a, byte b) {
            return int(std::clamp((1.0 - phase) * a + double(b) * phase, 0.0, 255.0));
        };
        const int r = channel(first.R, last.R), g = channel(first.G, last.G), b = channel(first.B, last.B);
        RasterDrawingRequest request;
        request.target = drawing.target;
        request.position = {clip.X + start.X, clip.Y + start.Y};
        request.clip = clip;
        request.width = request.height = 1;
        request.color = WORD((r >> 3 << 11) | (g >> 2 << 5) | (b >> 3));
        const auto status = submit_type_raster(drawing, request);
        if (status != DrawingStatus::drawn && status != DrawingStatus::skipped) return status;
        const double value = double(phase) + step;
        phase = store_float(value);
        if (value < 0 && step < 0) {
            phase = 0;
            step = -step;
        } else if (value > 1 && step > 0) {
            phase = 1;
            step = -step;
        }
        if (horizontal) {
            if (error > 0) {
                start.Y += direction;
                error -= 2 * dx;
            }
            error += 2 * dy;
            ++start.X;
        } else {
            if (error > 0) {
                ++start.X;
                error -= 2 * dy;
            }
            error += 2 * dx;
            start.Y += direction;
        }
    }
    return DrawingStatus::drawn;
}
}
