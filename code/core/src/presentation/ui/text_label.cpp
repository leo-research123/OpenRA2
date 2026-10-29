// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/txtlabel.cpp Draw; calibrated to YR 0x0072A4A0.
// Existing Gadget gating plus YR ColorScheme, scrolling and type-on animation.
#include "yrpp/TextLabelClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Timer.h"
#include "game_ui_runtime.hpp"
#include <bit>
#include <cwchar>

#include "presentation/ui/messages.hpp"
#include "presentation/ui/text.hpp"
#include "ui_render_state.hpp"
namespace game {
bool build_text_label(GameUiFrame& context, const TextLabelClass& self, bool forced) {
    if (self.SkipDraw || (!forced && !self.NeedsRedraw)) return false;
    auto& colors = ColorScheme::Array;
    if (!colors.Count) return false;
    const int index = self.ColorSchemeIndex <= 0 || self.ColorSchemeIndex >= colors.Count ? 0 : self.ColorSchemeIndex;
    const auto* scheme = colors[index];
    if (!scheme) return false;
    auto* font = BitFont::Instance;
    auto* frame = &context;
    Surface* surface = frame ? nullptr : DSurface::Temp;
    if (!font || !self.Text || (!surface && !frame)) return false;
    const int surface_width = surface ? surface->GetWidth() : DSurface::WindowBounds.Width;
    RectangleStruct rect{self.X, self.Y, std::bit_cast<int>(self.PixWidth) < 0 ? surface_width : int(self.PixWidth), font->field_1C};
    // 0x0072A549 writes the reduced width back to the local draw rectangle.
    // The downstream font clip must use this reduced width.
    if (rect.Width + self.X - surface_width > 0) {
        rect.Width = surface_width - self.X;
        if (rect.Width <= 0 || rect.Height <= 0) return false;
    }
    const auto rgb = ColorScheme::HSVToRGB(scheme->BaseColor);
    const unsigned color = unsigned(rgb.R) | (unsigned(rgb.G) << 8) | (unsigned(rgb.B) << 16);
    const int length = int(std::wcslen(self.Text));
    int scroll = std::bit_cast<int>(self.anim_dword3C);
    build_edit_text(surface, &rect, self.Text, length, font, color, &scroll, GadgetClass::Focused == &self, false, true,
                            std::bit_cast<int>(self.AnimPos), ui_label_suppressed(context.render_state, self));
    return true;
}

}
