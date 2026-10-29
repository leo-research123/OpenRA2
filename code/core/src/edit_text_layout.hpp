#pragma once
#include "yrpp/BasicStructures.h"
class BitFont;
namespace game {
// Original 0x623880 layout, without drawing or writes to original state.
bool layout_edit_text(BitFont*, const RectangleStruct*, const wchar_t*, int& caret, int focused, int password,
                      int& scroll, wchar_t (&buffer)[0x800], int& length, int& ime_length, int& total,
                      int& effective) noexcept;
}
