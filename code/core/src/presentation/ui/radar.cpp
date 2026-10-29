#include "presentation/ui/widgets.hpp"
// RadarClass chrome and terrain/viewport drawing, calibrated to 656EC0.
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "presentation/builders/beacon.hpp"
#include "game_ui_runtime.hpp"
#include "type_drawing.hpp"
#include "yrpp/OwnerDraw.h"
#include <algorithm>
#include <array>
#include <cstring>

#include "presentation/ui/radar.hpp"
#include "ui_render_state.hpp"
#include "sidebar_movie.hpp"
#include "presentation/ui/radar_events.hpp"
namespace game {
void build_radar(GameUiFrame& context, const RadarClass& self, DWORD force) {
    using namespace game;
    if (Unsorted::ArmageddonMode) return;
    auto* frame = &context;
    const auto* state = context.render_state;
    if (state && !drawing_completed(state->radar_status)) {
        record_ui_drawing(state->radar_status);
        return;
    }
    const int x = DSurface::SidebarBounds.X;
    const auto& resources = frame->resources;
    const auto* credits = resources.image(UiImage::credits);
    const auto* top = resources.image(UiImage::top);
    const auto* radar = resources.image(UiImage::radar);
    if (!credits || !top || !radar || radar->Frames < 1 || !resources.image(UiImage::briefing) ||
        !resources.image(UiImage::options)) {
        record_ui_drawing(DrawingStatus::unavailable);
        return;
    }
    draw_ui_shape(UiImage::top, {x, credits->Height});
    // The active radar frame is SHP frame 32 (6575FC).
    const int art_frame = state ? state->radar_frame : (self.unknown_14AC == 1 ? (self.unknown_14B0 ? 32 : 0) : int(self.unknown_14FC));
    draw_ui_shape(UiImage::radar, {x, credits->Height + top->Height}, std::clamp(art_frame, 0, int(radar->Frames) - 1));
    build_shape_button(context, self.DiplomacyButton, true);
    build_shape_button(context, self.OptionsButton, true);
    // 0x00656EC0 keeps the foreground current in every mode; only the active
    // tactical radar presents it.
    const bool active = state ? state->radar_active : self.unknown_14B0 == 1 && self.unknown_14AC == 1;
    if (active && !build_radar_map(context, self)) return;
    if (state ? state->radar_names : self.IsPlayerNames()) build_radar_names(context, self);
    if (state) draw_sidebar_movie(state->movie);
    else if (self.IsPlayingMovie() && WWMovieHandle::Array.Count > 0 && WWMovieHandle::Array[0])
        WWMovieHandle::Array[0]->VTable->Blit(WWMovieHandle::Array[0]);
}
bool build_radar_map(GameUiFrame& context, const RadarClass& self) noexcept {
    using namespace game;
    auto* frame = &context;
    if (context.render_state && !drawing_completed(context.render_state->radar_status)) {
        record_ui_drawing(context.render_state->radar_status);
        return false;
    }
    if (!self.unknown_121C) {
        record_ui_drawing(DrawingStatus::unavailable);
        return false;
    }
    const auto panel = self.GetPanelBounds();
    draw_ui_fill(panel, 0);
    // The host target is rebuilt every frame, so the whole retained
    // foreground is submitted; only invalidated pixels were recomposed.
    std::array<WORD, 140 * 108> pixels{};
    const auto& terrain = self.unknown_rect_149C;
    auto* foreground = self.unknown_121C;
    const int pitch = foreground->GetPitch() / 2;
    if (terrain.Width > 140 || terrain.Height > 108 || pitch < terrain.Width) {
        record_ui_drawing(DrawingStatus::backend_failure);
        return false;
    }
    const auto* source = static_cast<const WORD*>(foreground->Lock(0, 0));
    if (!source) {
        foreground->Unlock();
        record_ui_drawing(DrawingStatus::backend_failure);
        return false;
    }
    for (int row = 0; row < terrain.Height; ++row)
        std::memcpy(pixels.data() + row * terrain.Width, source + row * pitch, std::size_t(terrain.Width) * 2);
    foreground->Unlock();
    const auto& sidebar = DSurface::SidebarBounds;
    draw_ui_bitmap({sidebar.X + terrain.X, terrain.Y}, terrain.Width, terrain.Height, pixels.data(), pixels.size());
    build_radar_events(context);
    build_radar_beacons(context, BeaconManagerClass::Instance, nullptr, {sidebar.X + terrain.X, terrain.Y, terrain.Width, terrain.Height});
    if (context.render_state && !context.render_state->radar_viewport)
        return drawing_completed(frame->status);
    const auto& raw = self.unknown_rect_14DC;
    const int left = std::max(raw.X, terrain.X), top_edge = std::max(raw.Y, terrain.Y);
    const int right = std::min(raw.X + raw.Width, terrain.X + terrain.Width);
    const int bottom = std::min(raw.Y + raw.Height, terrain.Y + terrain.Height);
    if (right > left && bottom > top_edge) {
        draw_ui_fill({sidebar.X + left, top_edge, right - left, 1}, WORD(self.unknown_1208));
        draw_ui_fill({sidebar.X + left, bottom - 1, right - left, 1}, WORD(self.unknown_1208));
        draw_ui_fill({sidebar.X + left, top_edge, 1, bottom - top_edge}, WORD(self.unknown_1208));
        draw_ui_fill({sidebar.X + right - 1, top_edge, 1, bottom - top_edge}, WORD(self.unknown_1208));
    }
    const int x = sidebar.X + terrain.X - 1, y = terrain.Y - 1, w = terrain.Width + 2, h = terrain.Height + 2;
    draw_ui_fill({x, y, w, 1}, WORD(self.unknown_1208));
    draw_ui_fill({x, y + h - 1, w, 1}, WORD(self.unknown_1208));
    draw_ui_fill({x, y, 1, h}, WORD(self.unknown_1208));
    draw_ui_fill({x + w - 1, y, 1, h}, WORD(self.unknown_1208));
    return frame->status == DrawingStatus::drawn || frame->status == DrawingStatus::skipped;
}
}
