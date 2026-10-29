#include "yrpp/CCToolTip.h"
#include "yrpp/BitFont.h"
#include "yrpp/BitText.h"
#include "yrpp/MouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/TacticalClass.h"
#include "tooltip_platform.hpp"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include "presentation/ui/tooltips.hpp"
#include "presentation/ui/bit_text.hpp"
namespace game {
void build_tooltip_text(GameUiFrame& context, const ToolTipManagerData& data, bool on_sidebar) {
    auto* frame = &context;
    auto* font = BitFont::Instance;
    if (!frame || !font || !font->InternalPTR || !BitText::Instance) return;
    const auto& tactical = DSurface::ViewBounds;
    const auto& sidebar = DSurface::SidebarBounds;
    const auto& r = data.Dimension;
    const int edge = GameOptionsClass::Instance.SidebarMode ? tactical.X + tactical.Width : sidebar.X + sidebar.Width;
    const bool map = GameOptionsClass::Instance.SidebarMode ? r.X + r.Width <= edge : r.X >= edge;
    const bool side = GameOptionsClass::Instance.SidebarMode ? r.X >= edge : r.X + r.Width < edge;
    if (!map && !(on_sidebar && side)) return;
    // 0x478EF5 uses Surface::GetWidth (vtable 0x7C); the draw box adds
    // 8x4 and reclamps X, distinct from Update's 4x3 measurement.
    const auto& region = map ? tactical : sidebar;
    int width = 0, height = 0;
    font->GetTextDimension(data.HelpText, &width, &height, region.Width);
    width += 8;
    height += 4;
    const int x = std::min(r.X, region.X + region.Width - width);
    const RectangleStruct box{x, r.Y, width, height};
    const auto& rgb = CCToolTip::ToolTipTextColor;
    const WORD color = WORD((rgb.Red >> 3) << 11 | (rgb.Green >> 2) << 5 | (rgb.Blue >> 3));
    game::draw_ui_fill(box, 0);
    game::draw_ui_fill({box.X, box.Y, box.Width, 1}, color);
    game::draw_ui_fill({box.X, box.Y + box.Height - 1, box.Width, 1}, color);
    game::draw_ui_fill({box.X, box.Y, 1, box.Height}, color);
    game::draw_ui_fill({box.X + box.Width - 1, box.Y, 1, box.Height}, color);
    LTRBStruct clip{box.X, box.Y, box.X + box.Width, box.Y + box.Height};
    font->SetClipMode(true);
    font->SetRectangle(&clip);
    font->SetColor(color);
    // The UI frame is the drawing target of the tooltip text.
    PresentationContext text;
    text.type = &context.drawing.types;
    build_bit_text(text, font, nullptr, data.HelpText, box.X + 4, box.Y + 2, width, height, 0, 0, 0);
}
void build_tooltip(GameUiFrame& context, const CCToolTip& tips, bool on_sidebar) {
    if (tips.CurrentToolTip) build_tooltip_text(context, tips.CurrentToolTipData, on_sidebar);
}
}
