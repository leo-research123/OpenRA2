// Original Mouse/Scroll and Gadget chain; the host submits device events only.
#include "yrpp/MouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include "game_ui_runtime.hpp"
#include "player_commands.hpp"

void MouseClass::ResetInput() noexcept {
    GadgetClass::ResetInput(); ResetScrollInput();
    if (InputManagerClass::Instance) InputManagerClass::Instance->Reset();
}
void MouseClass::ProcessInput(const game::GameInputEvent& event,game::GameInputResult& result) noexcept {
    using namespace game;
    result={};
    if (!InputManagerClass::Instance) return;
    if (event.kind==GameInputKind::focus_lost || event.kind==GameInputKind::pointer_leave) {
        ResetInput(); result.consumed=true; return;
    }
    auto& keyboard=*InputManagerClass::Instance;
    const Point2D point{event.x,event.y};
    const auto& map=DSurface::ViewBounds.Width>0 && DSurface::ViewBounds.Height>0
        ? DSurface::ViewBounds : TacticalClass::ViewBounds;
    const bool inside=point.X>=map.X && point.Y>=map.Y && point.X<map.X+map.Width && point.Y<map.Y+map.Height;
    GameUiInput input{point};
    input.modifier=static_cast<KeyModifier>(event.modifiers&7);
    keyboard.SetKeyState(16,(event.modifiers&1)!=0);
    keyboard.SetKeyState(17,(event.modifiers&2)!=0);
    keyboard.SetKeyState(18,(event.modifiers&4)!=0);
    if (event.kind==GameInputKind::key) {
        keyboard.SetKeyState(event.code,event.pressed);
        input.key=event.code|((event.modifiers&7)<<8)|(event.pressed ? 0 : 0x800);
        input.flags=GadgetFlag::Keyboard;
    } else if (event.kind==GameInputKind::pointer_button) {
        keyboard.SetKeyState(event.code,event.pressed); keyboard.SetClickPosition(point);
        if (event.code==1) input.flags=event.pressed ? GadgetFlag::LeftPress : GadgetFlag::LeftRelease;
        if (event.code==2) input.flags=event.pressed ? GadgetFlag::RightPress : GadgetFlag::RightRelease;
    } else {
        input.flags=(keyboard.IsKeyPressed(1) ? GadgetFlag::LeftHeld : GadgetFlag::LeftUp)|
            (keyboard.IsKeyPressed(2) ? GadgetFlag::RightHeld : GadgetFlag::RightUp);
    }
    if (Buttons) with_game_ui_input(input,[] { TabClass::Instance.ProcessButtonKey(Buttons->Input()); });
    result.consumed=GadgetClass::StuckOn || !inside;
    if(event.kind==GameInputKind::key&&!GadgetClass::Focused)
        result.consumed=game::dispatch_player_hotkey(static_cast<WWKey>(input.key))||result.consumed;
    if (event.kind==GameInputKind::pointer_button && event.code==2) {
        if (event.pressed && inside && !GadgetClass::StuckOn) {
            unknown_byte_554A=1;
            unknown_int_5550=static_cast<DWORD>(point.X); unknown_int_5554=static_cast<DWORD>(point.Y);
        } else if (!event.pressed) {
            // Mouse.RightMouseButtonUp 0x693840 reaches Display's cancel
            // path only when no drag occurred. Capture this before reset.
            result.consumed=result.consumed||unknown_byte_5548;
            ResetScrollInput();
        }
    }
    if (event.kind==GameInputKind::pointer_move && unknown_byte_554A && !GadgetClass::StuckOn) {
        Point2D warp; bool should_warp=false;
        DragScroll(point,warp,should_warp);
        result.consumed=true; result.warp_pointer=should_warp;
        if (should_warp) { result.pointer_x=warp.X; result.pointer_y=warp.Y; }
    }
}
void MouseClass::UpdateInput(const Point2D& point,bool inside) noexcept {
    if (!inside || GadgetClass::StuckOn || (ScenarioClass::Instance && ScenarioClass::Instance->unknown_62C!=0)) return;
    ScrollAtEdge(point);
}
