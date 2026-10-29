// Base perimeter mask, YR 0x00486D90.
#include "yrpp/CellClass.h"
int CellClass::GetBaseSpacingMask(unsigned houseIndex) const {
    const unsigned mask = 1u << (houseIndex & 31u);
    if (!(BaseSpacerOfHouses & mask)) return -1;
    int neighbours = 0;
    for (int i = 0; i < 8; ++i)
        if (GetNeighbourCell(static_cast<FacingType>(i))->BaseSpacerOfHouses & mask) neighbours |= 1 << i;
    return neighbours;
}
