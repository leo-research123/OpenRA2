#pragma once
class TabClass; class SidebarClass; class PowerClass;
namespace game {
struct GameUiFrame;
void build_sidebar(GameUiFrame&, const SidebarClass&, unsigned force);
void build_tab(GameUiFrame&, const TabClass&, unsigned force);
void build_power(GameUiFrame&, const PowerClass&, unsigned force);
}
