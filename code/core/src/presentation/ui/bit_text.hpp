#pragma once
#include "presentation/context.hpp"
class BitFont;
class Surface;
namespace game {
// BitText::DrawText (YR 0x00434CD0) on an explicit context: surface-less text goes to ctx.type.
void build_bit_text(PresentationContext&, BitFont*, Surface*, const wchar_t*, int x, int y, int width, int height,
                    char alignment, int limit, int fade);
}
