#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"

#if !defined(RA2_YRPP_GAME)
CellStruct* TechnoClass::ScanForTiberium(CellStruct* output, int range, DWORD) const {
    auto& map = MapClass::Instance;
    const auto center = CellClass::Coord2Cell(GetCoords());
    *output = CellStruct::Empty;
    if (map.GetCellAt(center)->LandType == LandType::Tiberium) {
        *output = center;
        return output;
    }
    int value = -1;
    const auto inspect = [&](int dx, int dy) {
        const CellStruct at{short(center.X + dx), short(center.Y + dy)};
        if (map.GetCellAt(at)->LandType == LandType::Tiberium) {
            const int candidate = map.GetCellAt(at)->GetContainedTiberiumValue();
            if (candidate > value) {
                value = candidate;
                *output = at;
            }
        }
    };
    // The building scan has no Foot passability or campaign visibility filter.
    for (int radius = 1; radius < range && value == -1; ++radius)
        for (int offset = -radius; offset <= radius; ++offset) {
            inspect(offset, -radius);
            inspect(offset, radius);
            inspect(-radius, offset);
            inspect(radius, offset);
        }
    return output;
}
#endif
