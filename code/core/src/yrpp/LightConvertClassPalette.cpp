// Copyright 2025 Electronic Arts Inc.; SPDX-License-Identifier: GPL-3.0-or-later
// Shared YR 556090/7DE200..7DEBBA palette arithmetic, extracted from
// LightConvertClass.cpp. Surface, Blitter and LightConvert lifetime are separate.
#include "LightConvertClassHelpers.hpp"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <bit>
#include <climits>
namespace {
int wrap_product(int a, int b) { return std::bit_cast<int>(unsigned(a) * unsigned(b)); }
int red_dominant_channel(int channel, int maximum) {
    // 555989 retains max * 65536 * binary64(0.001), rounded to the game's
    // 53-bit, toward-zero x87 precision, across _ftol. Its exact numerator
    // before rounding is max * 1152921504606847 over 2^44 (fits uint64).
    // Only an integral channel*1000/max can cross an integer boundary under
    // this rounding. Determine which side using integers, without changing
    // the host floating-point environment or requiring x87/long double.
    const std::uint64_t product=std::uint64_t(maximum)*1152921504606847ULL;
    const auto shift=std::max(int(std::bit_width(product))-53,0);
    const auto rounded=(product>>shift)<<shift;
    const auto ideal_floor=std::uint64_t(maximum)*1152921504606846ULL+
        std::uint64_t(maximum)*976/1000; // floor(max * 2^60 / 1000)
    const int numerator=channel*1000;
    return numerator/maximum-int(channel && numerator%maximum==0 && rounded>ideal_floor);
}
int light_factor(int value) {
    // The stored double 0.065536 is 4722366482869645 / 2^56. x87 keeps
    // the product extended until _ftol; rounding it to double first changes
    // e.g. color=1000 from 65535 to 65536. Split multiplication also works
    // with MSVC, whose long double has only double precision.
    const int input = wrap_product(1000, value);
    const auto magnitude = input < 0 ? std::uint64_t(-std::int64_t(input)) : std::uint64_t(input);
    constexpr std::uint64_t numerator = 4722366482869645ULL;
    const auto high = magnitude * (numerator >> 32);
    const auto tail = ((high & 0xffffffu) << 32) + magnitude * (numerator & 0xffffffffu);
    const int result = int((high >> 24) + (tail >> 56));
    return input < 0 ? -result : result;
}
int scale_channel(BYTE channel, int factor, bool mmx, bool specialized) {
    if (mmx) {
        const int word = std::clamp(factor >> 4, -32768, 32767);
        return std::clamp((int(channel) * 16 * word) >> 16, 0, 255);
    }
    const int product = wrap_product(channel, factor);
    return specialized ? int(std::min(unsigned(product) >> 16, 255u)) : std::min(product >> 16, 255);
}
WORD pack_light(int r, int g, int b, int mode) {
    switch (mode) {
    case 0: return WORD(((r & 248) << 7) | ((g & 248) << 2) | ((b & 248) >> 3));
    case 1: return WORD(((r & 248) << 8) | ((g & 248) << 3) | ((b & 252) >> 2));
    case 2: return WORD(((r & 248) << 8) | ((g & 252) << 3) | ((b & 248) >> 3));
    case 3: return WORD(((r & 252) << 8) | ((g & 248) << 2) | ((b & 248) >> 3));
    default: return Drawing::RGB_To_Int(BYTE(r), BYTE(g), BYTE(b));
    }
}
}
namespace game {
unsigned build_light_palette_row(const BytePalette* palette, WORD* destination, int red, int green,
    int blue, int unlit, const BYTE* indexes, int mode, bool use_mmx) noexcept {
    destination[0] = 0;
    const bool specialized = unsigned(mode) <= 3;
    const bool mmx = specialized && use_mmx;
    for (unsigned i = 1; i < 256; ++i) {
        const auto& c = palette->Entries[i];
        const bool lit = !indexes || indexes[i] != 0;
        destination[i] = pack_light(scale_channel(c.R, lit ? red : unlit, mmx, specialized),
            scale_channel(c.G, lit ? green : unlit, mmx, specialized),
            scale_channel(c.B, lit ? blue : unlit, mmx, specialized), mode);
    }
    return destination[255];
}
}
bool LightConvertClass::BuildColorTable(const BytePalette& palette,WORD* output,
    std::size_t capacity,int shades,int red,int green,int blue,const BYTE* indexes,
    std::size_t index_count,int mode,bool mmx) noexcept {
    if (shades<0 || (shades>=2 && shades<=13) || shades>INT_MAX/512 || (indexes && index_count<256)) return false;
    shades=std::max(shades,1);
    if (!output || std::size_t(shades)>capacity/256) return false;
    const int r = light_factor(red), g = light_factor(green), b = light_factor(blue);
    const int last = shades - 1;
    const int unlit_levels = std::min(std::max(wrap_product(30, shades) / 200 - 1, 0), last >> 1);
    for (int level = 0; level < shades; ++level) {
        const int rs = last ? wrap_product(wrap_product(2, r), level) / last : r;
        const int gs = last ? wrap_product(wrap_product(2, g), level) / last : g;
        const int bs = last ? wrap_product(wrap_product(2, b), level) / last : b;
        const int unlit = last && level <= unlit_levels ? wrap_product(level, 65536) / unlit_levels : 65536;
        game::build_light_palette_row(&palette, output,
            rs, gs, bs, unlit, indexes, mode, mmx);
        output += 256;
    }
    return true;
}

int LightConvertClass::NormalizeCellLight(DWORD& intensity,int& terrain,int& red,int& green,int& blue) noexcept {
    red=std::max(red,0); green=std::max(green,0); blue=std::max(blue,0);
    red=std::min(red,2000); green=std::min(green,2000); blue=std::min(blue,2000);
    intensity=0x10000;
    if (red!=1000 || green!=red || blue!=red) {
        const int maximum=std::max({red,green,blue});
        // For the bounded 0..2000 input, the original x87 multiplication by
        // positive-neighbor binary64 0.001 truncates exactly to this quotient.
        intensity=static_cast<DWORD>(maximum*65536/1000);
        if (intensity<66) { intensity=0x10000; red=green=blue=1000; terrain=0; }
        else {
            // Keep the original dominant-channel tie order and exact 1000.
            if (red>=green && red>=blue) {
                // 5559B0/5559C1 retain the floating-point denominator;
                // the other two branches reload the integer intensity.
                green=red_dominant_channel(green,maximum);
                blue=red_dominant_channel(blue,maximum);
                red=1000;
            } else if (green>=blue) {
                green=1000; red=red*65536/static_cast<int>(intensity); blue=blue*65536/static_cast<int>(intensity);
            } else {
                blue=1000; red=red*65536/static_cast<int>(intensity); green=green*65536/static_cast<int>(intensity);
            }
            terrain=std::bit_cast<std::int32_t>(static_cast<DWORD>(terrain)*intensity)>>16;
        }
    }
    terrain=std::min(terrain,2000);
    return terrain;
}

int LightConvertClass::PrepareCellTint(int& red,int& green,int& blue,int quality) noexcept {
    const int mask=quality==0 ? ~127 : quality==1 ? ~63 : quality==2 ? ~31 : ~0;
    red=std::clamp(red,0,1000)&mask; green=std::clamp(green,0,1000)&mask; blue=std::clamp(blue,0,1000)&mask;
    return red+green+blue<2000 ? 27 : 53;
}
