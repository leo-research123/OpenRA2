// Original MapClass ground-height query, 0x00578080. Uses the existing
// original slope/level calculation, not the display object's rounded level.
#include "yrpp/MapClass.h"
int MapClass::GetCellFloorHeight(const CoordStruct& coord) const {
    return GetCellAt(coord)->GetFloorHeight({coord.X, coord.Y});
}
