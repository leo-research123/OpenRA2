#pragma once
#include "api/type_drawing.hpp"
class MapClass;
namespace game {
// Link ra2_software_render, not just ra2_core. Borrowed objects must outlive the
// synchronous call. No ownership transfer, allocation, or original EXE lookup.
// Height projection is deliberately unbound (the supplied export omits the
// target constant); set height_to_pixels for nonzero Smudge heights.
TypeDrawingContext make_software_type_drawing(Surface* target, ConvertClass* palette,
    MapClass* map = nullptr) noexcept;
}
