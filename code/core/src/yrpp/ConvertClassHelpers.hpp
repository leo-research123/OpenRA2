#pragma once
#include "yrpp/ConvertClass.h"
namespace game {
// Allocating callers transfer storage ownership, including construction failure.
ConvertClass* construct_convert(void* storage, const BytePalette&, const BytePalette&,
    DSurface*, int shades, bool skip_blitters);
void initialize_blitters(ConvertClass* self);
void clear_blitters(ConvertClass* self);
void build_palette_table(WORD* destination, int shades, const BytePalette& palette);
}
