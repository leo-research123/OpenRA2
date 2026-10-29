#pragma once
#include "yrpp/ConvertClass.h"
namespace game {
// Allocating callers transfer storage ownership, including construction failure.
ConvertClass* construct_convert(void* storage, const BytePalette&, const BytePalette&, DSurface*, int shades,
                                bool skip_blitters);
void initialize_blitters(ConvertClass* self);
void clear_blitters(ConvertClass* self);
// Initialize original fields/tables; caller registers only after its own setup.
ConvertClass* initialize_convert_storage(ConvertClass*, const BytePalette&, const BytePalette&, int, int, bool);
void destroy_convert(ConvertClass*);
void build_palette_table(WORD* destination, int shades, const BytePalette& palette);
}
