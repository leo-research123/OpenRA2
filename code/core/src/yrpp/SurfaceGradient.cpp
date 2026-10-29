// Line_In_Bounds 0x007BC2B0: the original endpoint clip with its x87 chopping
// arithmetic. The gradient line that uses it is game::submit_gradient_line
// (surface_drawing.cpp).
#include "yrpp/Surface.h"
#include "x87_double.hpp"

namespace {
// The original repeats the border clip until both ends are inside and has no limit: when a truncated
// intersection lands just outside the other border (a line ending on a rectangle corner) it creeps
// toward the border by one ulp per pass, which takes thousands of passes. The host stops after this many
// passes and rejects the line, so an input the original needs about 7 million passes for does not stall a frame.
constexpr int max_clip_passes = 1 << 16;
bool clip_line(Point2D& first, Point2D& last, int width, int height, int left = 0, int top = 0) noexcept {
    double x = first.X, y = first.Y, ex = last.X, ey = last.Y;
    // Original FPCW 0xE7F rounds each double-precision x87 operation toward zero, including the
    // subtraction of the current start from the border. This affects an integer endpoint (e.g.
    // 30,10 -> 36,40 at y=35, or 29,16 reached from a start that was moved to a fractional y).
    const double dxdy = game::x87_divide(ex - x, ey - y), dydx = game::x87_divide(ey - y, ex - x);
    const auto intersection = [](double border, double from, double slope, double origin) {
        return game::x87_add(game::x87_multiply(game::x87_add(border, -from), slope), origin);
    };
    const auto code = [&](double a, double b) {
        return (a >= left + width ? 2 : a < left ? 1 : 0) | (b >= top + height ? 4 : b < top ? 8 : 0);
    };
    int a = code(x, y), b = code(ex, ey);
    for (int pass = 0; pass < max_clip_passes; ++pass) {
        if (!(a | b)) {
            first = {int(x), int(y)};
            last = {int(ex), int(ey)};
            return true;
        }
        if (a & b) return false;
        const int c = a ? a : b;
        double nx = x, ny = y;
        if (c & 8) {
            nx = intersection(top, y, dxdy, x);
            ny = top;
        } else if (c & 4) {
            ny = top + height - 1;
            nx = intersection(ny, y, dxdy, x);
        } else if (c & 2) {
            nx = left + width - 1;
            ny = intersection(nx, x, dydx, y);
        } else {
            nx = left;
            ny = intersection(left, x, dydx, y);
        }
        if (c == a) {
            x = nx;
            y = ny;
            a = code(x, y);
        } else {
            ex = nx;
            ey = ny;
            b = code(ex, ey);
        }
    }
    return false;
}
}
bool YRPP_FASTCALL Line_In_Bounds(Point2D* first, Point2D* last, RectangleStruct* bounds) {
    return first && last && bounds && bounds->Width > 0 && bounds->Height > 0 &&
           clip_line(*first, *last, bounds->Width, bounds->Height, bounds->X, bounds->Y);
}
