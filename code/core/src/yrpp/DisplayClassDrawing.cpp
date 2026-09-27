// Original Display/Tactical terrain responsibility, using the selected backend.
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "game_ui_runtime.hpp"
#include "tactical_drawing.hpp"
#include "map_world.hpp"
void DisplayClass::Draw(DWORD) {
    auto* frame=game::game_ui_frame();
    if (!frame) return;
    if (!TacticalClass::Instance) { game::record_ui_drawing(game::DrawingStatus::unavailable); return; }
    game::record_ui_drawing(game::draw_tactical_view(*TacticalClass::Instance,game::current_map_world(),
        frame->drawing,TacticalClass::ViewBounds,frame->statistics));
}
