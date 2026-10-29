// GScreenClass 4F4480 coordinates tactical and UI drawing into one output.
// Device buffers, messages/dialogs and world passes retain separate migration
// entries; this frame covers terrain and the original radar/sidebar/command UI.
#include "yrpp/GScreenClass.h"
#include "tactical_drawing.hpp"
#include "map_world.hpp"
#include "yrpp/TacticalClass.h"
#include "presentation/ui/screen.hpp"
#include "presentation/ui/chrome.hpp"
#include "presentation/ui/radar.hpp"
#include "presentation/ui/messages.hpp"
#include "presentation/ui/tooltips.hpp"
#include "yrpp/MouseClass.h"
#include "game_ui_runtime.hpp"
#include "screen_effect.hpp"
#include "ui_cycle.hpp"
#include "render_phase.hpp"
#include "verify/draw_purity.hpp"
#include "tooltip_platform.hpp"
#include "verify/draw_coverage.hpp"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Surface.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/SuperClass.h"
namespace game {
namespace {
// 0x004F44C2..0x004F4593 after the effect gate. While the Chrono transition
// is active the Composite keeps its output: Tactical and the regular UI are
// skipped, messages and tooltips still cover it.
void draw_layers(bool transition_active) {
    auto& ui = *game_ui_frame();
    const bool advancing = render_phase_ui_active();
    if (!transition_active) {
        if (TacticalClass::Instance)
            record_ui_drawing(draw_tactical_view(*TacticalClass::Instance, current_map_world(),
                                                 game_ui_frame()->drawing, TacticalClass::ViewBounds,
                                                 game_ui_frame()->statistics));
        else
            record_ui_drawing(DrawingStatus::unavailable);
        if (advancing) advance_ui_radar(ui, RadarClass::Instance, 1);
        {
            DrawPurityScope purity;
            game::DrawCategoryScope category(game::DrawCategory::ui_radar);
            build_radar(*game_ui_frame(), RadarClass::Instance, 1);
        }
        if (advancing) {
            finish_ui_radar(ui, RadarClass::Instance);
            advance_ui_sidebar(SidebarClass::Instance);
        }
        {
            DrawPurityScope purity;
            game::DrawCategoryScope category(game::DrawCategory::ui_sidebar);
            build_tab(*game_ui_frame(), MouseClass::Instance, 1);
        }
        if (advancing) finish_ui_sidebar(SidebarClass::Instance);
        {
            DrawPurityScope purity;
            game::DrawCategoryScope category(game::DrawCategory::ui_power);
            build_power(*game_ui_frame(), PowerClass::Instance, 1);
        }
        if (advancing) finish_ui_power(PowerClass::Instance, 1);
    }
    // 0x004F455D: messages cover the scene and are covered by tooltips.
    if (advancing) advance_ui_messages(ui, MessageListClass::Instance);
    {
        DrawPurityScope purity;
        game::DrawCategoryScope category(game::DrawCategory::ui_messages);
        build_messages(*game_ui_frame(), MessageListClass::Instance);
    }
    if (advancing && CCToolTip::Instance) advance_ui_tooltip(*CCToolTip::Instance);
    {
    DrawPurityScope purity;
    game::DrawCategoryScope tooltips(game::DrawCategory::ui_tooltips);
    // YR 0x004F4570: tooltips cover the completed battlefield/UI. The native
    // target combines both original surfaces, so draw the sidebar pass here too.
    if (auto* tips = CCToolTip::Instance) {
        build_tooltip(*game_ui_frame(), *tips, false);
        const auto& r = tips->CurrentToolTipData.Dimension;
        const auto& map = DSurface::ViewBounds;
        const auto& side = DSurface::SidebarBounds;
        const bool on_map =
            GameOptionsClass::Instance.SidebarMode ? r.X + r.Width <= map.X + map.Width : r.X >= side.X + side.Width;
        if (tips->IsToolTipShowing() && !on_map) build_tooltip(*game_ui_frame(), *tips, true);
    }
    }
    render_phase_ui_round_complete();
}
}
void present_game_screen() {
    if (!game::game_ui_frame()) return;
    initialize_ui_screen(*game_ui_frame(), false);
    // 0x53B560 ends the white stage by rendering the new world with Status=0
    // into the snapshot (presentation suppressed by 0xA9FA94) before this
    // frame's own render. The host captures exactly this pass.
    if (game::chrono_capture_pending()) {
        game::submit_chrono_capture(true);
        draw_layers(false);
        game::submit_chrono_capture(false);
    }
    // ChronoScreenEffect::Update wrote the Composite before Render; submit
    // its stage here so later layers are drawn over it.
    game::draw_chrono_screen();
    // 0x004F44C2: 0x53BAE0 is ChronoScreenEffect::Status != 0.
    draw_layers(ChronoScreenEffect::Status != 0);
}

}
