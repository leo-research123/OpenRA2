// Original Mouse/Scroll and Gadget chain; the host submits device events only.
#include "yrpp/MouseClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/Unsorted.h"
#include "game_ui_runtime.hpp"
#include "player_commands.hpp"

namespace game {
static void cancel_planning_pointer() noexcept {
    // A host cancellation has no matching button-up. Drop only the gesture;
    // the existing planning mode and graph still belong to PlanMgr.
    Game::PlanningNodeAtAC4CCC = Game::PlanningNodeAtAC4C38 = nullptr;
    Game::PlanningBlockedNode = nullptr;
    Game::PlanningInputBlocked = false;
}
void reset_mouse_input() noexcept {
    auto& mouse = MouseClass::Instance;
    game::cancel_player_hotkeys();
    GadgetClass::ResetInput();
    mouse.ClearDragBand();
    mouse.ResetScrollInput();
    cancel_planning_pointer();
    if (InputManagerClass::Instance) InputManagerClass::Instance->Reset();
}
void process_mouse_input(const GameInputEvent& event, GameInputResult& result) noexcept {
    auto& mouse = MouseClass::Instance;
    result = {};
    if (!InputManagerClass::Instance) return;
    if (event.kind == GameInputKind::focus_lost || event.kind == GameInputKind::pointer_leave) {
        reset_mouse_input();
        result.consumed = true;
        return;
    }
    auto& keyboard = *InputManagerClass::Instance;
    const Point2D point{event.x, event.y};
    if (WWMouseClass::Instance && event.kind != GameInputKind::key) WWMouseClass::Instance->SetCoords(point);
    const auto& map = DSurface::ViewBounds.Width > 0 && DSurface::ViewBounds.Height > 0 ? DSurface::ViewBounds
                                                                                        : TacticalClass::ViewBounds;
    const bool inside =
        point.X >= map.X && point.Y >= map.Y && point.X < map.X + map.Width && point.Y < map.Y + map.Height;
    GameUiInput input{point};
    input.modifier = static_cast<KeyModifier>(event.modifiers & 7);
    keyboard.SetKeyState(16, (event.modifiers & 1) != 0);
    keyboard.SetKeyState(17, (event.modifiers & 2) != 0);
    keyboard.SetKeyState(18, (event.modifiers & 4) != 0);
    if (event.kind == GameInputKind::key) {
        keyboard.SetKeyState(event.code, event.pressed);
        input.key = event.code | ((event.modifiers & 7) << 8) | (event.pressed ? 0 : 0x800);
        input.flags = GadgetFlag::Keyboard;
    } else if (event.kind == GameInputKind::pointer_button) {
        keyboard.SetKeyState(event.code, event.pressed);
        keyboard.SetClickPosition(point);
        if (event.code == 1) input.flags = event.pressed ? GadgetFlag::LeftPress : GadgetFlag::LeftRelease;
        if (event.code == 2) input.flags = event.pressed ? GadgetFlag::RightPress : GadgetFlag::RightRelease;
    } else {
        input.flags = (keyboard.IsKeyPressed(1) ? GadgetFlag::LeftHeld : GadgetFlag::LeftUp) |
                      (keyboard.IsKeyPressed(2) ? GadgetFlag::RightHeld : GadgetFlag::RightUp);
    }
    if (Unsorted::UserInputLocked) {
        mouse.ClearDragBand();
        mouse.ResetScrollInput();
        game::cancel_player_hotkeys();
        cancel_planning_pointer();
        result.consumed = event.kind != GameInputKind::key || GadgetClass::Focused;
        if (event.kind == GameInputKind::key && !GadgetClass::Focused)
            result.consumed = game::dispatch_player_hotkey(static_cast<WWKey>(input.key));
        return;
    }
    // Escape cancels an active operation before dispatching an Options binding.
    // Decide before polling gadgets: that can itself clear command modes.
    // Keyboard coordinates/capture must not swallow an idle host menu key.
    if (event.kind == GameInputKind::key && event.code == 27 && !GadgetClass::Focused) {
        if (event.pressed) {
            const bool cancel_mode = mouse.CurrentBuilding || mouse.RepairMode || mouse.SellMode ||
                                     mouse.PowerToggleMode || mouse.PlanningMode || mouse.PlaceBeaconMode ||
                                     mouse.CurrentSWTypeIndex != -1;
            const bool cancel_drag = mouse.LeftPressAndDraggingRectangle || mouse.unknown_bool_11D0;
            if (cancel_mode) mouse.DisplayClass::RightMouseButtonUp(0);
            if (cancel_mode || cancel_drag) mouse.ClearDragBand();
            result.consumed = cancel_mode || cancel_drag;
        }
        if (result.consumed) return;
    }
    if (GScreenClass::Buttons)
        with_game_ui_input(input, [] { TabClass::Instance.ProcessButtonKey(GScreenClass::Buttons->Input()); });
    if (GadgetClass::Focused) game::cancel_player_hotkeys();
    result.consumed =
        event.kind == GameInputKind::key ? GadgetClass::Focused != nullptr : GadgetClass::StuckOn || !inside;
    if (event.kind == GameInputKind::key && !GadgetClass::Focused)
        result.consumed = game::dispatch_player_hotkey(static_cast<WWKey>(input.key)) || result.consumed;
    if (event.kind == GameInputKind::pointer_button && (event.code == 1 || event.code == 2)) {
        const bool held = mouse.unknown_byte_554A != 0;
        if (!event.pressed && event.code == 2 && held && !keyboard.IsKeyPressed(1) && !mouse.unknown_byte_554C &&
            GameOptionsClass::Instance.ScrollMethod == 0) {
            // Preserve a complete gesture submitted between two host frames.
            keyboard.SetKeyState(2, true);
            Point2D warp;
            bool should_warp = false;
            mouse.DragScroll({point.X - map.X, point.Y - map.Y}, warp, should_warp);
            keyboard.SetKeyState(2, false);
            if (TacticalClass::Instance &&
                TacticalClass::Instance->TacticalCoord2 != TacticalClass::Instance->TacticalCoord1)
                TacticalClass::Instance->FocusView(TacticalClass::Instance->TacticalCoord2);
        }
        // 0x693225/0x69338A dispatch planner releases before the ordinary drag
        // gate. A planner press deliberately leaves unknown_byte_554A clear.
        if ((event.pressed && inside && !GadgetClass::StuckOn) || !event.pressed) {
            Point2D local{point.X - map.X, point.Y - map.Y};
            // Terrain-only inspection has no player or actor commands. It may
            // still pan the camera with the same Scroll state.
            if (!HouseClass::CurrentPlayer) {
                if (event.pressed) {
                    if (event.code == 2 && !held) mouse.RightMouseButtonClick(&local);
                    mouse.unknown_byte_554A = 1;
                } else
                    mouse.ClearDragBand();
                result.consumed = true;
            } else {
                // A host may deliver the final motion and release between two
                // frames. Sample the held point before the original release.
                if (event.code == 1 && !event.pressed && held && !Game::PlanningNodeAtAC4C38)
                    mouse.DisplayClass::LeftMouseButtonDown(local);
                result.consumed = mouse.ProcessMouseButton(event.code, event.pressed, local) || result.consumed;
            }
        }
        if (!event.pressed && event.code == 2) {
            mouse.ResetScrollInput();
            if (keyboard.IsKeyPressed(1)) mouse.unknown_byte_554A = 1;
        }
    }
    if (event.kind == GameInputKind::pointer_move && mouse.unknown_byte_554A && !GadgetClass::StuckOn) {
        Point2D warp;
        bool should_warp = false;
        // Relative methods return their OS warp synchronously with the event.
        if (GameOptionsClass::Instance.ScrollMethod && !keyboard.IsKeyPressed(1) && keyboard.IsKeyPressed(2))
            mouse.DragScroll({point.X - map.X, point.Y - map.Y}, warp, should_warp);
        if (TacticalClass::Instance &&
            TacticalClass::Instance->TacticalCoord2 != TacticalClass::Instance->TacticalCoord1)
            TacticalClass::Instance->FocusView(TacticalClass::Instance->TacticalCoord2);
        if (keyboard.IsKeyPressed(1) && HouseClass::CurrentPlayer)
            mouse.DisplayClass::LeftMouseButtonDown({point.X - map.X, point.Y - map.Y});
        result.consumed = true;
        result.warp_pointer = should_warp;
        if (should_warp) {
            result.pointer_x = warp.X + map.X;
            result.pointer_y = warp.Y + map.Y;
            if (WWMouseClass::Instance) WWMouseClass::Instance->SetCoords({result.pointer_x, result.pointer_y});
        }
    }
}
void update_mouse_input(const Point2D& point, bool inside, void (*core_input)()) noexcept {
    auto& mouse = MouseClass::Instance;
    // The gadget list reads this borrowed device state during GScreenClass input. The scope is
    // open only while the pointer is over the map area and input is not locked or captured.
    game::GameUiInput input{point};
    bool gadget_input = false;
    if (inside && !Unsorted::UserInputLocked && InputManagerClass::Instance && GScreenClass::Buttons) {
        const auto& keyboard = *InputManagerClass::Instance;
        input.modifier =
            static_cast<KeyModifier>((keyboard.IsKeyPressed(16) ? 1 : 0) | (keyboard.IsKeyPressed(17) ? 2 : 0) |
                                     (keyboard.IsKeyPressed(18) ? 4 : 0));
        input.flags = (keyboard.IsKeyPressed(1) ? GadgetFlag::LeftHeld : GadgetFlag::LeftUp) |
                      (keyboard.IsKeyPressed(2) ? GadgetFlag::RightHeld : GadgetFlag::RightUp);
        gadget_input = true;
    }
    const bool locked = Unsorted::UserInputLocked;
    if (!inside || GadgetClass::StuckOn) Unsorted::UserInputLocked = true;
    // A terrain-only native view has no original player/cursor device yet.
    // Preserve its camera gesture outside the initialized-game polling body.
    if (!Unsorted::UserInputLocked && (!WWMouseClass::Instance || !HouseClass::CurrentPlayer)) {
        const Point2D local{point.X - DSurface::ViewBounds.X, point.Y - DSurface::ViewBounds.Y};
        if (mouse.unknown_byte_554A == 1 && !InputManagerClass::Instance->IsKeyPressed(1) &&
            InputManagerClass::Instance->IsKeyPressed(2)) {
            Point2D warp;
            bool should_warp = false;
            mouse.DragScroll(local, warp, should_warp);
        } else if (mouse.unknown_byte_554A != 1 && mouse.unknown_byte_5549 && GameOptionsClass::Instance.AutoScroll &&
                   !Unsorted::ArmageddonMode)
            mouse.ScrollAtEdge(local);
    }
    // Outside the map the original sees a pointer outside every region.
    const Point2D seen = inside ? point : Point2D{-1, -1};
    if (WWMouseClass::Instance && core_input) {
        // GScreenClass input samples the driver position and the gadget list, then updates
        // the screen chain with both.
        WWMouseClass::Instance->SetCoords(seen);
        if (gadget_input)
            game::with_game_ui_input(input, core_input);
        else
            core_input();
        WWMouseClass::Instance->SetCoords(point);
    } else {
        // No device (headless or terrain-only view): drive the chain directly.
        if (gadget_input)
            game::with_game_ui_input(input, [] { TabClass::Instance.ProcessButtonKey(GScreenClass::Buttons->Input()); });
        if (WWMouseClass::Instance) WWMouseClass::Instance->SetCoords(point);
        mouse.Update(0, seen);
    }
    Unsorted::UserInputLocked = locked;
    if (TacticalClass::Instance && TacticalClass::Instance->TacticalCoord2 != TacticalClass::Instance->TacticalCoord1)
        TacticalClass::Instance->FocusView(TacticalClass::Instance->TacticalCoord2);
}
}
