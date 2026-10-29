#include "edit_text_layout.hpp"
#include "yrpp/BitFont.h"
#include "yrpp/OwnerDraw.h"
#include <algorithm>
#include <cwchar>
namespace game {
bool layout_edit_text(BitFont* font, const RectangleStruct* rect, const wchar_t* text, int& caret, int focused,
                      int password, int& scroll, wchar_t (&buffer)[0x800], int& length, int& ime_length,
                      int& total, int& effective) noexcept {
    if (!font || !rect) return false;
    if (!text) text = L"";
    length = int(std::wcslen(text));
    if (caret < 0 || caret > length) caret = 0;
    ime_length = 0;
    int ime_active = 0;
    if (focused) {
        ime_length = OwnerDraw::IMECompositionStringLength;
        ime_active = OwnerDraw::IMEComposing;
    }
    // Reject inputs that would overrun the executable's 0x800-unit scratch area.
    // MessageList's 162-unit lines and 256-unit edit buffer cannot reach this.
    if (caret + ime_length >= 0x800 || scroll < 0) return false;
    std::wcsncpy(buffer, text, 0x800);
    if (ime_length) {
        std::wcsncpy(buffer + caret, OwnerDraw::IMECompositionString, 0x800 - caret);
        std::wcsncpy(buffer + caret + ime_length, text + caret, 0x800 - caret - ime_length);
    }
    buffer[0x7FF] = 0;
    total = int(std::wcslen(buffer));
    if (password) std::wmemset(buffer, L'*', total);
    effective = caret + ((ime_active || ime_length) ? OwnerDraw::IMECompositionCursorPos : 0);
    if (effective < scroll + 5) scroll = std::max(0, effective - 5);
    while (scroll < total && font->GetTextFit(buffer + scroll, rect->Width - 5, 0, false) < effective - scroll)
        ++scroll;
    // Invalid externally supplied offsets have no safe pointer interpretation.
    if (scroll > total) return false;
    return true;
}
}
