#include "presentation/ui/widgets.hpp"
// TabClass 6D0A20 and original SHP-derived rectangles from 72FC60.
#include "yrpp/MouseClass.h"
#include "bitfont_drawing.hpp"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <cwchar>
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "presentation/ui/chrome.hpp"
namespace game {
void build_tab(GameUiFrame& context, const TabClass& self, unsigned force) {
    using namespace game;
    auto* frame = &context;
    const auto& assets = frame->resources;
    draw_ui_shape(UiImage::credits, {DSurface::SidebarBounds.X, 0});
    // CreditsClass 4A2370: original LastValue, centered at sidebar width/2,
    // y=2, %ld. Identity is still terrain-browser startup until House exists.
    if (auto* font = assets.font()) {
        wchar_t text[32];
        std::swprintf(text, 32, L"%d", self.TabData.LastValue);
        int width = 0, height = 0;
        font->GetTextDimension(text, &width, &height, 0);
        int position = DSurface::SidebarBounds.X + (168 - width) / 2;
        // 72A940/72A960/72A980 -> 72F440 original tooltip/credits RGB.
        const bool other = ScenarioClass::Instance && ScenarioClass::Instance->PlayerSideIndex;
        const WORD color = other ? 0xffe0 : WORD(((164 >> 3) << 11) | ((210 >> 2) << 5) | (255 >> 3));
        for (const auto* character = text; *character; ++character) {
            int next = position;
            record_ui_drawing(game::submit_bitfont_glyph(*font, frame->drawing.types, *character, position, 2,
                                                         DSurface::WindowBounds, color, next));
            position = next;
        }
    }
    const auto bar = self.GetCommandBarBounds();
    const int end_width = assets.image(UiImage::rightcap)->Width;
    const int start_width = assets.image(UiImage::leftcap)->Width;
    const int pitch = assets.image(UiImage::button_background)->Width;
    const int end_x = bar.Width - end_width;
    const int count = std::max(0, (bar.Width - start_width - end_width) / pitch);
    const int begin = end_x - count * pitch;
    const int spacer_width = assets.image(UiImage::spacer)->Width;
    draw_ui_shape_strip(UiImage::spacer, {0, bar.Y}, {spacer_width, 0}, (bar.Width + spacer_width - 1) / spacer_width);
    if (self.ThumbActive) {
        draw_ui_shape_strip(UiImage::button_background, {begin, bar.Y}, {pitch, 0}, count);
        for (auto& button : self.CommandButtons)
            if (button.ShapeData) build_shape_button(context, button, true);
        build_shape_button(context, self.CollapseButton, true);
    } else
        build_shape_button(context, self.ExpandButton, true);
    draw_ui_shape(UiImage::rightcap, {end_x, bar.Y});
    build_sidebar(context, self, force);
}

}
