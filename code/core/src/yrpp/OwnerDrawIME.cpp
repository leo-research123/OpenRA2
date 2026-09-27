// Original OwnerDraw IME globals and 0x00777EA0 Unicode snapshot adapter.
// An absent device composition is the original null-hIMC path. Native editing
// and host IME event delivery are integrated separately from message drawing.
#include "yrpp/OwnerDraw.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <cwchar>
namespace {
wchar_t composition[0x101]{};
int composition_length=0,composition_cursor=0,composing=0,suppress_caret=0;
COLORREF ime_color=0x9F9F,caret_color=0xFFFFFF; // 0x0060059C / 0x006005A6.
}
wchar_t (&OwnerDraw::IMECompositionString)[0x101]=composition;
int& OwnerDraw::IMECompositionStringLength=composition_length;
int& OwnerDraw::IMECompositionCursorPos=composition_cursor;
int& OwnerDraw::IMEComposing=composing;
int& OwnerDraw::SuppressCaret=suppress_caret;
COLORREF& OwnerDraw::ImeCompositionTextColor=ime_color;
COLORREF& OwnerDraw::CaretColor=caret_color;
void YRPP_FASTCALL OwnerDraw::UpdateIMECompositionString() noexcept {
    IMECompositionString[0]=0;IMECompositionStringLength=0;IMECompositionCursorPos=0;
    const auto* frame=game::game_ui_frame();
    const auto* input=frame?frame->composition:nullptr;
    if(!input || !input->text)return;
    // Original buffer is 0x101 UTF-16 code units, with a terminator at 0x100.
    std::wcsncpy(IMECompositionString,input->text,0x100);IMECompositionString[0x100]=0;
    IMECompositionStringLength=int(std::wcslen(IMECompositionString));
    IMECompositionCursorPos=std::clamp(input->cursor,0,IMECompositionStringLength);
}
