// Copyright 2025 Electronic Arts Inc.; SPDX-License-Identifier: GPL-3.0-or-later
// Palette portion of pinned Renegade WWLib convert.cpp, calibrated to YR
// 48E740. Extracted from ConvertClassLifecycle.cpp without Blitter dependencies.
#include "ConvertClassHelpers.hpp"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <bit>
#include <climits>

namespace {
WORD pack(BYTE r,BYTE g,BYTE b,int mode) noexcept {
    switch(mode) {
    case 0:return WORD(((r&248)<<7)|((g&248)<<2)|((b&248)>>3));
    case 1:return WORD(((r&248)<<8)|((g&248)<<3)|((b&252)>>2));
    case 2:return WORD(((r&248)<<8)|((g&252)<<3)|((b&248)>>3));
    case 3:return WORD(((r&252)<<8)|((g&248)<<2)|((b&248)>>3));
    default:return WORD(Drawing::RGB_To_Int(r,g,b));
    }
}
}
bool ConvertClass::BuildColorTable(const BytePalette& palette,WORD* output,
    std::size_t capacity,int shades,int mode) noexcept {
    if (shades<0 || shades>INT_MAX/512) return false;
    shades=std::max(shades,1);
    if (!output || std::size_t(shades)>capacity/256) return false;
    std::uint32_t step=0;
    for (int level=0;level<shades;++level,step+=0x20000u) {
        const auto multiplier=shades<=1 ? 0x10000 : std::bit_cast<std::int32_t>(step)/(shades-1);
        for (const auto& c : palette.Entries) {
            const int r=std::bit_cast<std::int32_t>(std::uint32_t(multiplier)*c.R)>>16;
            const int g=std::bit_cast<std::int32_t>(std::uint32_t(multiplier)*c.G)>>16;
            const int b=std::bit_cast<std::int32_t>(std::uint32_t(multiplier)*c.B)>>16;
            *output++=pack(BYTE(std::min(r,255)),BYTE(std::min(g,255)),BYTE(std::min(b,255)),mode);
        }
    }
    return true;
}
namespace game {
void build_palette_table(WORD* destination,int shades,const BytePalette& palette) {
    ConvertClass::BuildColorTable(palette,destination,std::size_t(std::max(shades,1))*256,shades,-1);
}
}
