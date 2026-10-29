// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/rect.h::Intersect; YR 0x00421B60.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/Drawing.h"
#include <bit>

RectangleStruct* YRPP_FASTCALL Drawing::Intersect(RectangleStruct* out, const RectangleStruct& first,
                                                  const RectangleStruct& second, int* dx, int* dy) {
    const auto add = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
    const auto sub = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
    // Copy both operands before writing out: the original permits aliasing.
    const auto a = first, b = second;
    auto r = b;
    if (a.Width > 0 && a.Height > 0 && r.Width > 0 && r.Height > 0) {
        if (r.X < a.X) {
            r.Width = add(sub(r.X, a.X), r.Width);
            r.X = a.X;
        }
        if (r.Width > 0) {
            if (r.Y < a.Y) {
                r.Height = add(sub(r.Y, a.Y), r.Height);
                r.Y = a.Y;
            }
            if (r.Height > 0) {
                if (add(r.X, r.Width) > add(a.X, a.Width)) r.Width = sub(add(a.Width, a.X), r.X);
                if (r.Width > 0) {
                    if (add(r.Y, r.Height) > add(a.Y, a.Height)) r.Height = sub(add(a.Height, a.Y), r.Y);
                    if (r.Height > 0) {
                        if (dx) *dx = add(*dx, sub(b.X, r.X));
                        if (dy) *dy = add(*dy, sub(b.Y, r.Y));
                        *out = r;
                        return out;
                    }
                }
            }
        }
    }
    *out = {};
    return out;  // Empty intersection leaves the optional deltas alone.
}

#if !defined(RA2_IMAGE_GAME) && !defined(RA2_FILES_GAME) && !defined(RA2_YRPP_GAME)
bool YRPP_FASTCALL Drawing::BlitClip(RectangleStruct& d, const RectangleStruct& dw, RectangleStruct& s,
                                     const RectangleStruct& sw) {
    const auto add = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
    const auto sub = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
    if (d.Width == s.Width && d.Height == s.Height) {
        // 0x7BBE20 uses window-relative coordinates for an unscaled blit.
        if (d.X < 0) {
            s.X = sub(s.X, d.X);
            s.Width = add(s.Width, d.X);
            d.Width = add(d.Width, d.X);
            d.X = 0;
        }
        if (d.Y < 0) {
            s.Y = sub(s.Y, d.Y);
            s.Height = add(s.Height, d.Y);
            d.Height = add(d.Height, d.Y);
            d.Y = 0;
        }
        auto excess = sub(add(d.X, d.Width), dw.Width);
        if (excess > 0) {
            s.Width = sub(s.Width, excess);
            d.Width = sub(d.Width, excess);
        }
        excess = sub(add(d.Y, d.Height), dw.Height);
        if (excess > 0) {
            s.Height = sub(s.Height, excess);
            d.Height = sub(d.Height, excess);
        }
        if (s.X < 0) {
            d.X = sub(d.X, s.X);
            s.Width = add(s.Width, s.X);
            d.Width = add(d.Width, s.X);
            s.X = 0;
        }
        if (s.Y < 0) {
            d.Y = sub(d.Y, s.Y);
            s.Height = add(s.Height, s.Y);
            d.Height = add(d.Height, s.Y);
            s.Y = 0;
        }
        excess = sub(add(s.X, s.Width), sw.Width);
        if (excess > 0) {
            s.Width = sub(sw.Width, s.X);
            d.Width = sub(d.Width, excess);
        }
        excess = sub(add(s.Y, s.Height), sw.Height);
        if (excess > 0) {
            s.Height = sub(sw.Height, s.Y);
            d.Height = sub(d.Height, excess);
        }
    } else {
        Intersect(&d, dw, d, nullptr, nullptr);
        Intersect(&s, sw, s, nullptr, nullptr);
    }
    return d.Width > 0 && d.Height > 0 && s.Width > 0 && s.Height > 0;
}

// YR 0x00487F40. An empty operand yields the other one. Growing to the second
// rectangle's far edge adds one more than the covering size.
RectangleStruct* YRPP_FASTCALL Drawing::Union(RectangleStruct* out, const RectangleStruct& first,
                                              const RectangleStruct& second) {
    const auto add = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
    const auto sub = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
    const auto a = first, b = second;
    if (a.Width <= 0 || a.Height <= 0) {
        *out = b;
        return out;
    }
    if (b.Width <= 0 || b.Height <= 0) {
        *out = a;
        return out;
    }
    RectangleStruct r = a;
    if (r.X > b.X) {
        r.Width = add(r.Width, sub(r.X, b.X));
        r.X = b.X;
    }
    if (r.Y > b.Y) {
        r.Height = add(r.Height, sub(r.Y, b.Y));
        r.Y = b.Y;
    }
    if (add(r.X, r.Width) < add(b.X, b.Width)) r.Width = add(add(sub(b.X, r.X), b.Width), 1);
    if (add(r.Height, r.Y) < add(b.Y, b.Height)) r.Height = add(sub(add(b.Y, b.Height), r.Y), 1);
    *out = r;
    return out;
}
#endif
