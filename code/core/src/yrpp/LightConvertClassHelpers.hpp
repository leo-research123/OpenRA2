#pragma once
#include "yrpp/ConvertClass.h"
namespace game {
unsigned build_light_palette(const BytePalette*, WORD*, int, int, int, int, const BYTE*, int);
unsigned build_light_palette_row(const BytePalette*, WORD*, int, int, int, int,
    const BYTE*, int, bool) noexcept;
}
