#pragma once
#include "api/type_drawing.hpp"
namespace game {
// Optional CPU implementation; ownership belongs to IsometricTileTypeClass.
DrawingStatus draw_tmp_software(const TileDrawingRequest&);
}
