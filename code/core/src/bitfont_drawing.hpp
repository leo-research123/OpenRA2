#pragma once
#include "type_drawing_context.hpp"
class BitFont;
namespace game {
struct GameUiFrame;
// Internal destination adapter for the original BitFont string algorithm.
// No ownership, additional text state or public YRpp/host ABI.
int submit_bitfont_string(BitFont&, const wchar_t*, int, int, int, int, GameUiFrame&);
// BitFont 0x434780 glyph (MSB-first coverage, original advance) submitted as a
// mask request on an explicit context. next_x receives the pen position after
// the glyph; a tab advances without drawing (skipped).
DrawingStatus submit_bitfont_glyph(BitFont&, const TypeDrawingContext&, wchar_t, int x, int y,
                                   const RectangleStruct& clip, WORD color, int& next_x) noexcept;
}
