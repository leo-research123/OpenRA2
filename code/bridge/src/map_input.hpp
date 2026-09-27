#pragma once
#include "api/map_view.hpp"
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/vector2.hpp>
namespace game {
// Device encoding only: no game regions, camera or command policy. The caller
// owns and resets the macOS Ctrl-click latch when focus/capture is lost.
bool encode_canvas_input(const godot::Ref<godot::InputEvent>&,
    const godot::Vector2& surface_size,const godot::Vector2& canvas_size,GameInputEvent&,
    bool& control_click) noexcept;
}
