#include "yrpp/IsometricTileTypeClass.h"
namespace {
int starts[256]{}, set_count = 0, shadows[5]{};
#define YR_TILE_FIELD(name, address, initial, is_set) int value_##name = initial;
#include "IsometricTileTypeClassTheaterData.inc"
#undef YR_TILE_FIELD
}
int (&IsometricTileTypeClass::TileSetStarts)[256] = starts;
int& IsometricTileTypeClass::TileSetCount = set_count;
int (&IsometricTileTypeClass::ShadowTileSets)[5] = shadows;
#define YR_TILE_FIELD(name, address, initial, is_set) int& IsometricTileTypeClass::name = value_##name;
#include "IsometricTileTypeClassTheaterData.inc"
#undef YR_TILE_FIELD
