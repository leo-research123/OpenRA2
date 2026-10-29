// BitFont 0x434780 MSB-first glyph coverage and original advance.
// Font choice, measurement and glyph interpretation remain in the original
// core class. No Godot font, text or UI node is involved.
#include "bitfont_drawing.hpp"
#include "yrpp/BitFont.h"
#include <algorithm>
#include <cstdint>
#include <climits>

namespace game {
DrawingStatus submit_bitfont_glyph(BitFont& font, const TypeDrawingContext& drawing, wchar_t character, int x, int y,
                                   const RectangleStruct& clip, WORD color, int& next_x) noexcept {
    next_x = x;
    if (character == L'\t') {
        const auto advance = font.Unknown_28 ? std::int64_t(font.Unknown_28) + x -
                                                   (std::int64_t(font.Unknown_28) + x - font.field_20) % font.Unknown_28
                                             : x;
        if (advance < INT_MIN || advance > INT_MAX) return DrawingStatus::invalid_argument;
        next_x = int(advance);
        return DrawingStatus::skipped;
    }
    const auto* glyph = font.GetCharacterBitmap(character);
    if (!glyph) {
        glyph = static_cast<const unsigned char*>(font.Pointer_8);
        color ^= 0x5555;
    }
    if (!glyph || !font.InternalPTR) return DrawingStatus::unavailable;
    const int width = glyph[0], height = font.InternalPTR->FontHeight;
    const auto next = std::int64_t(x) + width + font.State_2C;
    if (next < INT_MIN || next > INT_MAX || std::int64_t(y) + height > INT_MAX ||
        width > std::int64_t(font.InternalPTR->Stride) * 8 || height < 0 || font.field_18 < (width + 7) / 8 ||
        (height > 0 && std::uint64_t(height - 1) * font.field_18 + (width + 7) / 8 > UINT32_MAX))
        return DrawingStatus::invalid_argument;
    next_x = static_cast<int>(next);
    MaskDrawingRequest request;
    request.target = drawing.target;
    request.position = {x, y};
    request.clip = clip;
    request.width = width;
    request.height = height;
    request.pitch = font.field_18;
    request.bits = glyph + 1;
    request.byte_count = height > 0 ? std::uint32_t(std::uint64_t(height - 1) * font.field_18 + (width + 7) / 8) : 0;
    request.color = color;
    return submit_type_mask(drawing, request);
}
}
