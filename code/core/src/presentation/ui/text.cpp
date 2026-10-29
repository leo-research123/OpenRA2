// OwnerDraw original text path, YR 0x006211D0. Surface-less submissions use the
// existing native UI frame, with the same font state and inclusive clip bounds.
#include "yrpp/OwnerDraw.h"
#include "yrpp/BitFont.h"
#include "yrpp/BitText.h"
#include "yrpp/Drawing.h"
#include "game_ui_runtime.hpp"

#include "presentation/ui/text.hpp"
#include <algorithm>
namespace game {
int build_fixed_text(unsigned color, BitFont* font, RectangleStruct* rect, const wchar_t* text, int length,
                     int horizontal, int vertical, Surface* surface, int animation) noexcept {
    // Rectangle-relative and point-relative anchors have different pixel rules.
    enum class HorizontalAlign : int { RectLeft = 0, RectCenter = 1, PointCenter = 2, PointRight = 3, RectRight = 4 };
    enum class VerticalAlign : int { RectTop = 0, RectCenter = 1, PointCenter = 2, PointBottom = 3 };
    // Original surfaces require focus; an explicit native UI frame remains
    // drawable while the host window is in the background.
    if (!Game::IsFocused && (surface || !game::game_ui_frame())) return 0;
    if (!font) font = BitFont::Instance;
    if (!surface && !game::game_ui_frame()) surface = DSurface::Alternate;
    if (!font || !rect || !text || (!surface && !game::game_ui_frame())) return 0;
    int width = 0, height = 0;
    font->GetTextDimension(text, &width, &height, rect->Width);
    int x = rect->X, y = rect->Y;
    switch (static_cast<HorizontalAlign>(horizontal)) {
        case HorizontalAlign::RectCenter:
            x += (rect->Width - width + 1) / 2;
            break;
        case HorizontalAlign::PointCenter:
            x += (width + 1) / -2;
            break;
        case HorizontalAlign::PointRight:
            x += -1 - width;
            break;
        case HorizontalAlign::RectRight:
            x += rect->Width - width - 1;
            break;
        default:
            break;
    }
    switch (static_cast<VerticalAlign>(vertical)) {
        case VerticalAlign::RectCenter:
            y += (rect->Height - height + 1) / 2;
            break;
        case VerticalAlign::PointCenter:
            y += (height + 1) / -2;
            break;
        case VerticalAlign::PointBottom:
            y += -1 - height;
            break;
        default:
            break;
    }
    LTRBStruct bounds{rect->X, rect->Y, rect->X + rect->Width, rect->Y + rect->Height};
    // Original BitFont::Lock (0x4348F0) intersects inclusive font bounds
    // with the destination even if type-on animation emits no glyphs.
    if (!surface && game_ui_frame()) {
        const auto& window = DSurface::WindowBounds;
        bounds.Left = std::max(bounds.Left, 0);
        bounds.Top = std::max(bounds.Top, 0);
        bounds.Right = std::min(bounds.Right, window.Width - 1);
        bounds.Bottom = std::min(bounds.Bottom, window.Height - 1);
    }
    font->SetClipMode(true);
    font->SetRectangle(&bounds);
    RGBClass rgb(color);
    font->SetColor(WORD(rgb.ToInt()));
    return BitText::Print(font, surface, text, x, y, length, animation) - x;
}

}
