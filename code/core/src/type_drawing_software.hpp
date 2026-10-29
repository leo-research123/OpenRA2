#pragma once
#include "type_drawing_context.hpp"
namespace game {
// Optional CPU implementation; ownership belongs to IsometricTileTypeClass.
DrawingStatus draw_tmp_software(const TileDrawingRequest&);
}
