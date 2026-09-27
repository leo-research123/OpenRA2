#pragma once
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/string.hpp>
// Manual-test adapter only. The core never includes this Godot header.
class RA2TypeDrawingCheck : public godot::Node2D {
    GDCLASS(RA2TypeDrawingCheck, godot::Node2D)
protected:
    static void _bind_methods();
public:
    // Synchronous sink parameters: 20 little-endian int32 words and packed
    // indexed texels. Returns a DrawingStatus numeric value after GPU dispatch.
    godot::String render_packets(const godot::Callable& sink, int frame,
        int intensity, bool depth, bool unsupported_mode, godot::Rect2i clip);
};
