#include "map_input.hpp"
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <cmath>
namespace game {
namespace {
unsigned virtual_key(godot::Key key) noexcept {
    using namespace godot;
    const unsigned value=static_cast<unsigned>(key);
    if (value>=32 && value<=126) return value;
    switch (key) {
        case KEY_ESCAPE:return 27; case KEY_ENTER:return 13; case KEY_TAB:return 9;
        case KEY_BACKSPACE:return 8; case KEY_DELETE:return 46; case KEY_INSERT:return 45;
        case KEY_HOME:return 36; case KEY_END:return 35; case KEY_PAGEUP:return 33; case KEY_PAGEDOWN:return 34;
        case KEY_LEFT:return 37; case KEY_UP:return 38; case KEY_RIGHT:return 39; case KEY_DOWN:return 40;
        case KEY_SHIFT:return 16; case KEY_CTRL:return 17; case KEY_ALT:return 18;
        default: if (key>=KEY_F1 && key<=KEY_F24) return 112+value-static_cast<unsigned>(KEY_F1);
    }
    return 0;
}
}
bool encode_canvas_input(const godot::Ref<godot::InputEvent>& source,
    const godot::Vector2& size,const godot::Vector2& canvas,GameInputEvent& output,bool& control_click) noexcept {
    using namespace godot;
    if (source.is_null() || size.x<=0 || size.y<=0 || canvas.x<=0 || canvas.y<=0) return false;
    output={};
    const Ref<InputEventWithModifiers> modifiers=source;
    if (modifiers.is_valid()) output.modifiers=(modifiers->is_shift_pressed()?1u:0u)|
        (modifiers->is_ctrl_pressed()?2u:0u)|(modifiers->is_alt_pressed()?4u:0u);
    const Ref<InputEventKey> key=source;
    if (key.is_valid()) {
        if (key->is_echo()) return false;
        output.kind=GameInputKind::key; output.code=virtual_key(key->get_keycode()); output.pressed=key->is_pressed();
        return output.code!=0;
    }
    const Ref<InputEventMouse> mouse=source;
    if (mouse.is_null()) return false;
    const auto point=mouse->get_position()*canvas/size;
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || std::abs(point.x)>65536 || std::abs(point.y)>65536) return false;
    output.x=static_cast<int>(std::floor(point.x)); output.y=static_cast<int>(std::floor(point.y));
    const Ref<InputEventMouseButton> button=source;
    if (button.is_valid()) {
        auto index=button->get_button_index();
#if defined(__APPLE__)
        // Godot's macOS mouseDown changes Ctrl+left to RIGHT, but preserves
        // NSEvent's physical button mask. Restore left at this device boundary;
        // a genuine right button (also with Ctrl) must retain cancel/scroll.
        // mouseUp uses the down-time mapping even if Ctrl was released first.
        const auto mask=button->get_button_mask();
        if(index==MOUSE_BUTTON_RIGHT) {
            if(button->is_pressed()) {
                if(button->is_ctrl_pressed() && mask.has_flag(MOUSE_BUTTON_MASK_LEFT)
                    && !mask.has_flag(MOUSE_BUTTON_MASK_RIGHT)) {
                    control_click=true;index=MOUSE_BUTTON_LEFT;
                }
            } else if(control_click && !mask.has_flag(MOUSE_BUTTON_MASK_LEFT)) {
                control_click=false;index=MOUSE_BUTTON_LEFT;
            }
        }
#else
        (void)control_click;
#endif
        if (index==MOUSE_BUTTON_WHEEL_UP || index==MOUSE_BUTTON_WHEEL_DOWN) {
            if (!button->is_pressed()) return false;
            output.kind=GameInputKind::wheel; output.wheel=index==MOUSE_BUTTON_WHEEL_UP ? 1 : -1; return true;
        }
        output.kind=GameInputKind::pointer_button;
        output.code=index==MOUSE_BUTTON_LEFT ? 1 : index==MOUSE_BUTTON_RIGHT ? 2 : index==MOUSE_BUTTON_MIDDLE ? 3 : 0;
        output.pressed=button->is_pressed(); return output.code!=0;
    }
    const Ref<InputEventMouseMotion> motion=source;
    output.kind=GameInputKind::pointer_move;
    return motion.is_valid();
}
}
