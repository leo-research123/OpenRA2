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
#include "presentation/ui/messages.hpp"
namespace game {
void build_messages(GameUiFrame& context, const MessageListClass& self) {
    if (self.IsEdit && self.EditLabel) {
        build_text_label(context, *self.EditLabel, true);
        if (auto* font = BitFont::Instance) {
            const int width = font->GetTextWidth(self.EditLabel->Text);
            if (self.CursorCharacter && std::bit_cast<int>(self.EditCurrentPos - self.EditInitialPos) < self.MaxCharacters - 1 &&
                self.EditLabel->IsFocused()) {
                const wchar_t text[]{self.CursorCharacter, 0};
                const Point2D at{width + self.EditLabel->X, self.EditLabel->Y};
                auto* scheme = ColorScheme::Array.GetItemOrDefault(self.EditLabel->ColorSchemeIndex);
                game::draw_ui_text(text, DSurface::WindowBounds, at, scheme, TextPrintType(self.EditLabel->Style));
            }
        }
    }
    for (auto* label = self.MessageList; label; label = static_cast<TextLabelClass*>(label->Next))
        build_text_label(context, *label, true);
}

}
