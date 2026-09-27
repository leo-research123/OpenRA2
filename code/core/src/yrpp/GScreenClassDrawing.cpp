// GScreenClass 4F4480 coordinates tactical and UI drawing into one output.
// Device buffers, messages/dialogs and world passes retain separate migration
// entries; this frame covers terrain and the original radar/sidebar/command UI.
#include "yrpp/GScreenClass.h"
#include "yrpp/MouseClass.h"
#include "game_ui_runtime.hpp"
#include "tooltip_platform.hpp"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Surface.h"
#include "yrpp/MessageListClass.h"
void GScreenClass::Render() {
    if (!game::game_ui_frame()) return;
    RadarClass::Instance.InitializeButtons();
    SidebarClass::Instance.InitializeButtons();
    TabClass::Instance.InitializeCommandButtons();
    game::tooltip_poll_timer();
    DisplayClass::Instance.DisplayClass::Draw(1);
    RadarClass::Instance.RadarClass::Draw(1);
    MouseClass::Instance.Draw(1);
    PowerClass::Instance.PowerClass::Draw(1);
    // 0x004F455D: messages cover the scene and are covered by tooltips.
    MessageListClass::Instance.Draw();
    // YR 0x004F4570: tooltips cover the completed battlefield/UI. The native
    // target combines both original surfaces, so draw the sidebar pass here too.
    if(auto* tips=CCToolTip::Instance){
        tips->Draw(false);
        const auto& r=tips->CurrentToolTipData.Dimension;
        const auto& map=DSurface::ViewBounds;const auto& side=DSurface::SidebarBounds;
        const bool on_map=GameOptionsClass::Instance.SidebarMode?r.X+r.Width<=map.X+map.Width:r.X>=side.X+side.Width;
        if(tips->IsToolTipShowing() && !on_map)tips->Draw(true);
    }
}
