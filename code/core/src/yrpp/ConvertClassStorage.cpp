// Shared real ConvertClass storage and palette lifetime; no rasterizer initialization.
// Extracted from ConvertClassLifecycle.cpp (YR 0x48E740).
// Copyright 2025 Electronic Arts Inc.; SPDX-License-Identifier: GPL-3.0-or-later
#include "ConvertClassHelpers.hpp"
#include "yrpp/Memory.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/Blitters/Blitter.h"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <new>
#ifndef RA2_IMAGE_GAME
#include "convert_revision.hpp"
#endif

#ifndef RA2_IMAGE_GAME
namespace {
DynamicVectorClass<ConvertClass*> converts;
}
DynamicVectorClass<ConvertClass*>& ConvertClass::Array = converts;
#endif

namespace game {
namespace {
struct HSV {
    BYTE h, s, v;
};
HSV to_hsv(const ColorStruct& c) {
    const int r = c.R, g = c.G, b = c.B;
    const int value = std::max({r, g, b}), white = std::min({r, g, b});
    const int saturation = value ? (value - white) * 255 / value : 0;
    unsigned hue = 0;
    if (saturation) {
        const unsigned delta = unsigned(value - white);
        const unsigned r1 = (value - r) * 255 / delta;
        const unsigned g1 = (value - g) * 255 / delta;
        const unsigned b1 = (value - b) * 255 / delta;
        if (value == r)
            hue = white == g ? 1280 + b1 : 256 - g1;
        else if (value == g)
            hue = white == b ? 256 + r1 : 768 - b1;
        else
            hue = white == r ? 768 + g1 : 1280 - r1;
        hue /= 6;
    }
    return {BYTE(hue), BYTE(saturation), BYTE(value)};
}
ColorStruct to_rgb(HSV c) {
    const unsigned hue = c.h * 6u, s = c.s, v = c.v, f = hue % 255u;
    const unsigned values[7] = {0,
                                v,
                                v,
                                v * (255 - s * f / 255) / 255,
                                v * (255 - s) / 255,
                                v * (255 - s) / 255,
                                v * (255 - s * (255 - f) / 255) / 255};
    unsigned i = hue / 255;
    i = i > 4 ? i - 4 : i + 2;
    const auto r = values[i];
    i = i > 4 ? i - 4 : i + 2;
    const auto b = values[i];
    i = i > 4 ? i - 4 : i + 2;
    return {BYTE(r), BYTE(values[i]), BYTE(b)};
}
BYTE closest(const BytePalette& palette, const ColorStruct& c) {
    int best = -1;
    BYTE index = 0;
    for (int i = 0; i < 256; ++i) {
        const auto& p = palette.Entries[i];
        const int r = int(p.R) - c.R, g = int(p.G) - c.G, b = int(p.B) - c.B;
        // YR 0x661350 differs from WWLib: weighted absolute RGB distance.
        const int distance = 2 * std::abs(r) + 4 * std::abs(g) + 3 * std::abs(b);
        if (best == -1 || distance < best) {
            best = distance;
            index = BYTE(i);
        }
    }
    return index;
}
void* allocate_table(std::size_t size) {
    void* result = YRMemory::Allocate(size);
    if (!result) throw std::bad_alloc();
    return result;
}
}
void clear_blitters(ConvertClass* self) {
    // The target clears its darken/lighten before its special color effects.
    constexpr unsigned order[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                                  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 33, 34, 30, 31,
                                  32, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49};
    for (const auto index : order) {
        auto*& blitter = self->Blitters[index];
        if (blitter) {
            blitter->~Blitter();  // virtual destruction, without freeing storage twice
            YRMemory::Deallocate(blitter);
        }
        blitter = nullptr;
    }
    for (auto*& blitter : self->RLEBlitters) {
        if (blitter) {
            blitter->~RLEBlitter();
            YRMemory::Deallocate(blitter);
        }
        blitter = nullptr;
    }
}
ConvertClass* initialize_convert_storage(ConvertClass* self, const BytePalette& art, const BytePalette& screen,
                                         int bytes_per_pixel, int shades, bool skip) {
    self->BytesPerPixel = bytes_per_pixel;
    std::fill(std::begin(self->Blitters), std::end(self->Blitters), nullptr);
    std::fill(std::begin(self->RLEBlitters), std::end(self->RLEBlitters), nullptr);
    self->ShadeCount = std::max(shades, 1);
    self->FullColorData = self->PaletteData = self->ByteColorData = nullptr;
    self->CurrentZRemap = nullptr;
    if (self->BytesPerPixel == 1) {
        auto* shadow = static_cast<BYTE*>(allocate_table(256));
        self->ByteColorData = shadow;
        shadow[0] = 0;
        for (int i = 1; i < 256; ++i) {
            auto hsv = to_hsv(art.Entries[i]);
            hsv.v >>= 1;
            shadow[i] = closest(art, to_rgb(hsv));
        }
        auto* data = static_cast<BYTE*>(allocate_table(std::uint32_t(self->ShadeCount) << 8));
        self->FullColorData = data;
        self->PaletteData = data + 256u * ((self->ShadeCount - 1) >> 1);
        for (int level = 0; level < self->ShadeCount; ++level) {
            *data++ = 0;
            for (int i = 1; i < 256; ++i) {
                if (self->ShadeCount == 1)
                    *data++ = closest(screen, art.Entries[i]);
                else {
                    auto hsv = to_hsv(art.Entries[i]);
                    hsv.v = BYTE(2 * level * int(hsv.v) / (self->ShadeCount - 1));
                    *data++ = closest(screen, to_rgb(hsv));
                }
            }
        }
    } else {
        self->FullColorData = allocate_table(std::uint32_t(self->ShadeCount) << 9);
        self->PaletteData = static_cast<BYTE*>(self->FullColorData) + 512u * ((self->ShadeCount - 1) >> 1);
        if (!skip) build_palette_table(static_cast<WORD*>(self->FullColorData), self->ShadeCount, art);
        self->HalfTranslucencyMask = static_cast<WORD>(Drawing::HalfbrightMask);
        self->QuatTranslucencyMask = static_cast<WORD>(Drawing::QuarterbrightMask);
    }
#ifndef RA2_IMAGE_GAME
    note_converter_tables(*self);
#endif
    return self;
}
void destroy_convert(ConvertClass* self) {
#ifndef RA2_IMAGE_GAME
    forget_converter(*self);
#endif
    clear_blitters(self);
    YRMemory::Deallocate(self->FullColorData);
    self->FullColorData = nullptr;
    YRMemory::Deallocate(self->ByteColorData);
    self->ByteColorData = nullptr;
    ConvertClass::Array.Remove(self);
}
}

ConvertClass::ConvertClass(const BytePalette& palette) {
    try {
        game::initialize_convert_storage(this, palette, palette, 2, 1, false);
        if (!Array.AddItem(this)) throw std::bad_alloc();
    } catch (...) {
        game::destroy_convert(this);
        throw;
    }
}
ConvertClass::~ConvertClass() { game::destroy_convert(this); }
