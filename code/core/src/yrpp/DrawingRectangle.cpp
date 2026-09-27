// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/rect.h::Intersect; YR 0x00421B60.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/Drawing.h"
#include <bit>

RectangleStruct* YRPP_FASTCALL Drawing::Intersect(RectangleStruct* out,
        const RectangleStruct& first,const RectangleStruct& second,int* dx,int* dy) {
    const auto add=[](int a,int b){return std::bit_cast<int>(unsigned(a)+unsigned(b));};
    const auto sub=[](int a,int b){return std::bit_cast<int>(unsigned(a)-unsigned(b));};
    // Copy both operands before writing out: the original permits aliasing.
    const auto a=first,b=second;
    auto r=b;
    if(a.Width>0 && a.Height>0 && r.Width>0 && r.Height>0) {
        if(r.X<a.X){r.Width=add(sub(r.X,a.X),r.Width);r.X=a.X;}
        if(r.Width>0) {
            if(r.Y<a.Y){r.Height=add(sub(r.Y,a.Y),r.Height);r.Y=a.Y;}
            if(r.Height>0) {
                if(add(r.X,r.Width)>add(a.X,a.Width))r.Width=sub(add(a.Width,a.X),r.X);
                if(r.Width>0) {
                    if(add(r.Y,r.Height)>add(a.Y,a.Height))r.Height=sub(add(a.Height,a.Y),r.Y);
                    if(r.Height>0) {
                        if(dx)*dx=add(*dx,sub(b.X,r.X));
                        if(dy)*dy=add(*dy,sub(b.Y,r.Y));
                        *out=r;return out;
                    }
                }
            }
        }
    }
    *out={};return out; // Empty intersection leaves the optional deltas alone.
}
