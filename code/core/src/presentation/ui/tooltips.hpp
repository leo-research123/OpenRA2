#pragma once
class CCToolTip; struct ToolTipManagerData;
namespace game {
struct GameUiFrame;
void build_tooltip_text(GameUiFrame&, const ToolTipManagerData&, bool on_sidebar);
void build_tooltip(GameUiFrame&, const CCToolTip&, bool on_sidebar);
}
