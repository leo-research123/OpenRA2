// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/msglist.cpp Draw, calibrated to YR 0x005D49A0.
// The modern compositor submits all labels each frame. Cursor save/restore is
// unnecessary in its separate presentation layer; edit cursor rules are kept.
#include "yrpp/MessageListClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/Surface.h"
#include "yrpp/ColorScheme.h"
#include "game_ui_runtime.hpp"
#include <bit>
void MessageListClass::Draw() const {
    if(!game::game_ui_frame())return; // Native complete-frame compositor.
    if(IsEdit && EditLabel){
        EditLabel->Draw(true);
        if(auto* font=BitFont::Instance){
            const int width=font->GetTextWidth(EditLabel->Text);
            if(CursorCharacter && std::bit_cast<int>(EditCurrentPos-EditInitialPos)<MaxCharacters-1 && EditLabel->IsFocused()){
                const wchar_t text[]{CursorCharacter,0};const Point2D at{width+EditLabel->X,EditLabel->Y};
                auto* scheme=ColorScheme::Array.GetItemOrDefault(EditLabel->ColorSchemeIndex);
                game::draw_ui_text(text,DSurface::WindowBounds,at,scheme,TextPrintType(EditLabel->Style));
            }
        }
    }
    if(MessageList)MessageList->DrawAll(true);
}
