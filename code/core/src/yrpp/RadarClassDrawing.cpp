// RadarClass chrome and terrain/viewport drawing, calibrated to 656EC0.
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <array>
#include <cstring>

void RadarClass::Draw(DWORD force) {
    using namespace game;
    if (Unsorted::ArmageddonMode) {
        unknown_bool_14D9=unknown_bool_14DA=false;
        return;
    }
    auto* frame=game_ui_frame();
    if (!frame) return;
    unknown_bool_14DA=unknown_bool_14DA || force==1;
    unknown_bool_14D9=unknown_bool_14D9 || unknown_bool_14DA;
    // 0x006531DB: system-time animation belongs to drawing, including the
    // logic-paused main-loop branch. Querying/input alone never advances it.
    AdvanceRadarAnimation();
    if (frame->advance_presentation) RadarEventClass::UpdateAll();
    struct FinishEvents {
        bool advance;
        ~FinishEvents(){if (advance) RadarEventClass::RemoveFinished();}
    } finish_events{frame->advance_presentation};
    // Terrain refresh belongs to presentation even when the emblem or another
    // mode covers it. Extra repaints only consume actual queued content changes.
    if (unknown_123C) {
        bool changed=false;
        if (!UpdateTerrainRadar(changed)) {record_ui_drawing(DrawingStatus::unavailable);return;}
    }
    const int x=DSurface::SidebarBounds.X;
    const auto& resources=frame->resources;
    const auto* credits=resources.image(UiImage::credits);
    const auto* top=resources.image(UiImage::top);
    const auto* radar=resources.image(UiImage::radar);
    if (!credits || !top || !radar || radar->Frames<1 ||
        !resources.image(UiImage::briefing) || !resources.image(UiImage::options)) {
        record_ui_drawing(DrawingStatus::unavailable); return;
    }
    draw_ui_shape(UiImage::top,{x,credits->Height});
    // The active radar frame is SHP frame 32 (6575FC).
    const int art_frame=unknown_14AC==1 ? (unknown_14B0 ? 32 : 0) : int(unknown_14FC);
    draw_ui_shape(UiImage::radar,{x,credits->Height+top->Height},std::clamp(art_frame,0,int(radar->Frames)-1));
    DiplomacyButton.Draw(true);
    OptionsButton.Draw(true);
    if (unknown_14B0==1 && unknown_14AC==1 && !RenderRadar()) return;
    if (IsPlayerNames()) DrawNames();
    // Failed submissions must retain pending invalidations for a later frame.
    if (frame->status!=DrawingStatus::drawn && frame->status!=DrawingStatus::skipped) return;
    unknown_bool_14D9=unknown_bool_14DA=false;
    unknown_points_125C.Count=0;
    if (unknown_1274) std::memset(unknown_1274,0,unknown_rect_149C.Width*unknown_rect_149C.Height/8+1);
    unknown_rect_120C={0,credits->Height,DSurface::SidebarBounds.Width,top->Height+radar->Height};
}

bool RadarClass::RenderRadar() noexcept {
    using namespace game;
    auto* frame=game_ui_frame();
    if (!frame) return false;
    bool changed=false;
    if (!UpdateTerrainRadar(changed)) {
        record_ui_drawing(DrawingStatus::unavailable); return false;
    }
    const auto panel=GetPanelBounds();
    draw_ui_fill(panel,0);
    std::array<WORD,140*108> pixels{};
    if (!CopyTerrainRadar(pixels.data(),pixels.size())) {
        record_ui_drawing(DrawingStatus::backend_failure); return false;
    }
    const auto& terrain=unknown_rect_149C;
    if (!ApplyTerrainVisibility(pixels.data(),pixels.size())) {record_ui_drawing(DrawingStatus::backend_failure);return false;}
    if (!ApplyTrackedObjects(pixels.data(),pixels.size())) {record_ui_drawing(DrawingStatus::backend_failure);return false;}
    const auto& sidebar=DSurface::SidebarBounds;
    draw_ui_bitmap({sidebar.X+terrain.X,terrain.Y},terrain.Width,terrain.Height,pixels.data(),pixels.size());
    RadarEventClass::DrawAll();
    BeaconManagerClass::Instance.DrawRadar(nullptr,{sidebar.X+terrain.X,terrain.Y,terrain.Width,terrain.Height});
    auto* tactical=TacticalClass::Instance;
    CellStruct center;
    const auto& view=TacticalClass::ViewBounds;
    if (!tactical || !tactical->PickTerrainCell({view.Width/2,view.Height/2},view,center) ||
        !UpdateViewportFrame(center,{view.Width,view.Height}))
        return frame->status==DrawingStatus::drawn || frame->status==DrawingStatus::skipped;
    const auto& raw=unknown_rect_14DC;
    const int left=std::max(raw.X,terrain.X),top_edge=std::max(raw.Y,terrain.Y);
    const int right=std::min(raw.X+raw.Width,terrain.X+terrain.Width);
    const int bottom=std::min(raw.Y+raw.Height,terrain.Y+terrain.Height);
    if (right>left && bottom>top_edge) {
        draw_ui_fill({sidebar.X+left,top_edge,right-left,1},WORD(unknown_1208));
        draw_ui_fill({sidebar.X+left,bottom-1,right-left,1},WORD(unknown_1208));
        draw_ui_fill({sidebar.X+left,top_edge,1,bottom-top_edge},WORD(unknown_1208));
        draw_ui_fill({sidebar.X+right-1,top_edge,1,bottom-top_edge},WORD(unknown_1208));
    }
    const int x=sidebar.X+terrain.X-1,y=terrain.Y-1,w=terrain.Width+2,h=terrain.Height+2;
    draw_ui_fill({x,y,w,1},WORD(unknown_1208)); draw_ui_fill({x,y+h-1,w,1},WORD(unknown_1208));
    draw_ui_fill({x,y,1,h},WORD(unknown_1208)); draw_ui_fill({x+w-1,y,1,h},WORD(unknown_1208));
    unknown_14EC=raw.X; unknown_14F0=raw.Y;
    unknown_14F4=raw.Width; unknown_14F8=raw.Height;
    return frame->status==DrawingStatus::drawn || frame->status==DrawingStatus::skipped;
}
