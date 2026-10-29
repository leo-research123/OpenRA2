#include "ui_cycle.hpp"
#include "ui_render_state.hpp"
#include "game_ui_runtime.hpp"
#include "type_drawing.hpp"
#include "edit_text_layout.hpp"
#include "sidebar_movie.hpp"
#include "tooltip_platform.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Unsorted.h"
#include <bit>
#include <cstring>
#include <cwchar>
namespace game {
namespace {
void finish_button(ShapeButtonClass& button) noexcept {
    button.NeedsRedraw = false;
    if (button.ShapeData) button.IsDrawn = true;
}
}
bool ui_label_suppressed(const UiRenderState* state, const TextLabelClass& label) noexcept {
    if (state) {
        const auto found = state->label_caret_suppressed.find(&label);
        if (found != state->label_caret_suppressed.end()) return found->second;
    }
    return OwnerDraw::SuppressCaret != 0;
}
void initialize_ui_screen(GameUiFrame& frame, bool iteration) {
    auto* state = frame.render_state;
    const int side = ScenarioClass::Instance ? ScenarioClass::Instance->PlayerSideIndex : -1;
    const auto& assets = frame.resources;
    const bool available = assets.image(UiImage::rightcap) && assets.image(UiImage::leftcap) &&
        assets.image(UiImage::button_background) && assets.image(UiImage::radar) &&
        assets.image(UiImage::briefing) && assets.image(UiImage::options);
    const bool changed = !state || !state->initialized ||
        std::memcmp(&state->initialized_bounds, &DSurface::WindowBounds, sizeof(RectangleStruct)) ||
        state->initialized_side != side;
    if (available && (iteration || changed)) {
        RadarClass::Instance.InitializeButtons();
        SidebarClass::Instance.InitializeButtons();
        TabClass::Instance.InitializeCommandButtons();
        if (state) {
            state->initialized = true;
            state->initialized_bounds = DSurface::WindowBounds;
            state->initialized_side = side;
        }
    }
    if (iteration) tooltip_poll_timer();
}
void advance_ui_radar(GameUiFrame& frame, RadarClass& radar, unsigned force, bool map_only) noexcept {
    auto* state = frame.render_state;
    if (!state) return;
    state->radar_status = DrawingStatus::skipped;
    state->movie.pixels.clear();
    if (Unsorted::ArmageddonMode && !map_only) {
        radar.unknown_bool_14D9 = radar.unknown_bool_14DA = false;
        return;
    }
    if (!map_only) {
        radar.unknown_bool_14DA = radar.unknown_bool_14DA || force == 1;
        radar.unknown_bool_14D9 = radar.unknown_bool_14D9 || radar.unknown_bool_14DA;
        radar.AdvanceRadarAnimation();
        RadarEventClass::UpdateAll();
    }
    state->radar_active = map_only || (radar.unknown_14B0 == 1 && radar.unknown_14AC == 1);
    state->radar_names = radar.IsPlayerNames();
    state->radar_movie = radar.IsPlayingMovie();
    state->radar_frame = radar.unknown_14AC == 1 ? (radar.unknown_14B0 ? 32 : 0) : int(radar.unknown_14FC);
    bool changed = false;
    if (radar.unknown_123C && !radar.UpdateTerrainRadar(changed)) {
        state->radar_status = DrawingStatus::unavailable;
        return;
    }
    if (state->radar_active && !radar.UpdateTerrainRadar(changed)) {
        state->radar_status = DrawingStatus::unavailable;
        return;
    }
    if ((state->radar_active || radar.unknown_121C) && !radar.UpdateRadarImage()) {
        state->radar_status = DrawingStatus::backend_failure;
        return;
    }
    state->radar_viewport = false;
    if (state->radar_active) {
        CellStruct center;
        const auto& view = TacticalClass::ViewBounds;
        if (auto* tactical = TacticalClass::Instance;
            tactical && tactical->PickTerrainCell({view.Width / 2, view.Height / 2}, view, center) &&
            radar.UpdateViewportFrame(center, {view.Width, view.Height})) {
            state->radar_viewport = true;
            const auto& r = radar.unknown_rect_14DC;
            radar.unknown_14EC = r.X;
            radar.unknown_14F0 = r.Y;
            radar.unknown_14F4 = r.Width;
            radar.unknown_14F8 = r.Height;
        }
    }
    if (!map_only && state->radar_movie) capture_sidebar_movie(state->movie);
}
void finish_ui_radar(GameUiFrame& frame, RadarClass& radar, bool map_only) noexcept {
    if (map_only) return;
    RadarEventClass::RemoveFinished();
    if (!frame.render_state || !drawing_completed(frame.render_state->radar_status) ||
        !drawing_completed(frame.status) || Unsorted::ArmageddonMode) return;
    const auto* credits = frame.resources.image(UiImage::credits);
    const auto* top = frame.resources.image(UiImage::top);
    const auto* art = frame.resources.image(UiImage::radar);
    if (!credits || !top || !art || !frame.resources.image(UiImage::briefing) || !frame.resources.image(UiImage::options)) return;
    finish_button(radar.DiplomacyButton);
    finish_button(radar.OptionsButton);
    radar.unknown_bool_14D9 = radar.unknown_bool_14DA = false;
    radar.unknown_points_125C.Count = 0;
    if (radar.unknown_1274)
        std::memset(radar.unknown_1274, 0, radar.unknown_rect_149C.Width * radar.unknown_rect_149C.Height / 8 + 1);
    radar.unknown_rect_120C = {0, credits->Height, DSurface::SidebarBounds.Width, top->Height + art->Height};
}
void advance_ui_sidebar(SidebarClass& sidebar) noexcept { sidebar.UpdateCommandButtons(); }
void finish_ui_sidebar(SidebarClass& sidebar) noexcept {
    finish_button(sidebar.ToggleRepairButton);
    finish_button(sidebar.ToggleSellButton);
    for (auto& button : sidebar.TabButtons) finish_button(button);
    auto& tab = TabClass::Instance;
    if (tab.ThumbActive) {
        for (auto& button : tab.CommandButtons) if (button.ShapeData) finish_button(button);
        finish_button(tab.CollapseButton);
    } else finish_button(tab.ExpandButton);
    sidebar.Tabs[sidebar.ActiveTabIndex].NeedsRedraw = false;
    sidebar.SidebarNeedsRedraw = sidebar.SidebarBackgroundNeedsRedraw = false;
    Game::SidebarChanged = false;
}
void finish_ui_power(PowerClass& power, unsigned force) noexcept {
    if ((force || power.PowerNeedRedraw) && SidebarClass::Instance.IsSidebarActive) power.PowerNeedRedraw = false;
}
void advance_ui_label(GameUiFrame& frame, TextLabelClass& label, bool forced) {
    if (label.SkipDraw || (!forced && !label.NeedsRedraw)) return;
    label.NeedsRedraw = false;
    const auto& colors = ColorScheme::Array;
    if (!colors.Count) return;
    const int index = label.ColorSchemeIndex <= 0 || label.ColorSchemeIndex >= colors.Count ? 0 : label.ColorSchemeIndex;
    if (!colors[index]) return;
    auto* font = BitFont::Instance;
    if (!font || !label.Text) return;
    const int width = DSurface::WindowBounds.Width;
    RectangleStruct rect{label.X, label.Y, std::bit_cast<int>(label.PixWidth) < 0 ? width : int(label.PixWidth), font->field_1C};
    if (rect.Width + label.X > width) {
        rect.Width = width - label.X;
        if (rect.Width <= 0 || rect.Height <= 0) return;
    }
    int length = int(std::wcslen(label.Text));
    if (label.Animate) {
        const DWORD now = SystemTimer::GetMilliseconds(), previous = label.AnimPos;
        label.AnimPos = previous ? previous + ((now - label.AnimTiming) >> 4) : 1;
        if (previous != label.AnimPos) label.AnimTiming = now;
        if (std::bit_cast<int>(label.AnimPos) <= length && RulesClass::Instance)
            VocClass::PlayGlobal(RulesClass::Instance->MessageCharTyped, 0x2000, 1.0f, nullptr);
    }
    const bool focused = label.IsFocused();
    if (focused) OwnerDraw::UpdateIMECompositionString();
    int caret = length, scroll = std::bit_cast<int>(label.anim_dword3C), ime = 0, total = 0, effective = 0;
    wchar_t buffer[0x800];
    if (!layout_edit_text(font, &rect, label.Text, caret, focused, false, scroll, buffer, length, ime, total, effective)) return;
    label.anim_dword3C = DWORD(scroll);
    if (frame.render_state) frame.render_state->label_caret_suppressed[&label] = OwnerDraw::SuppressCaret != 0;
    OwnerDraw::SuppressCaret = 0;
}
void advance_ui_messages(GameUiFrame& frame, const MessageListClass& list) {
    if (frame.render_state) frame.render_state->label_caret_suppressed.clear();
    if (list.IsEdit && list.EditLabel) advance_ui_label(frame, *list.EditLabel, true);
    for (auto* label = list.MessageList; label; label = static_cast<TextLabelClass*>(label->Next))
        advance_ui_label(frame, *label, true);
}
void advance_ui_tooltip(CCToolTip& tips) { tips.Process(); }
}
