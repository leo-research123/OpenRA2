#pragma once
#include "api/map_view.hpp"
#include "type_drawing_context.hpp"
class RadarClass; class SidebarClass; class PowerClass; class TextLabelClass; class MessageListClass; class CCToolTip;
namespace game {
struct GameUiFrame;
// Simulation-side operations at the original render position. Extraction can
// run repeatedly between them without advancing original UI objects.
void initialize_ui_screen(GameUiFrame&, bool iteration);
void advance_ui_radar(GameUiFrame&, RadarClass&, unsigned force, bool map_only = false) noexcept;
void finish_ui_radar(GameUiFrame&, RadarClass&, bool map_only = false) noexcept;
void advance_ui_sidebar(SidebarClass&) noexcept;
void finish_ui_sidebar(SidebarClass&) noexcept;
void finish_ui_power(PowerClass&, unsigned force) noexcept;
void advance_ui_label(GameUiFrame&, TextLabelClass&, bool forced);
void advance_ui_messages(GameUiFrame&, const MessageListClass&);
void advance_ui_tooltip(CCToolTip&);
}
