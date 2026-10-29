#pragma once
#include "api/type_drawing.hpp"
#include "render/frame.hpp"
class MapClass;
namespace game {
// Consumes an owned frame recorded for the optional original-software reference.
// Reconstructs SHP/TMP/converter storage locally and reuses the original blitters.
DrawingStatus draw_software_frame(const render::Frame&, Surface&) noexcept;

}
