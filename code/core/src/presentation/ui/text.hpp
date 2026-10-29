#pragma once
#include "yrpp/BasicStructures.h"
class BitFont; class Surface;
namespace game {
int build_fixed_text(unsigned color, BitFont*, RectangleStruct*, const wchar_t*, int length, int horizontal,
                     int vertical, Surface*, int animation) noexcept;
int build_edit_text(Surface*, RectangleStruct*, const wchar_t*, int caret, BitFont*, unsigned color,
                    int* scroll, int focused, int password, int background, int animation,
                    bool suppress_caret) noexcept;
}
