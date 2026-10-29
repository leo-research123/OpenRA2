// Retreat-cell search, YR 0x4CBC40.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "x87_integer.hpp"

CellStruct* FootClass::FindRetreatCell(CellStruct* output, const CellStruct& origin, const CellStruct& center,
                                       int minimum, int maximum) {
    const short raw = static_cast<short>(
        game::x87_integer((Math::atan2(double(origin.Y) - center.Y, double(center.X) - origin.X) - 1.570796326794897) *
                          -10430.06004058427));
    const unsigned direction = ((((unsigned(static_cast<unsigned short>(raw)) >> 12) + 1u) >> 1) ^ 0xFCu) - 1u;
    const unsigned facing = direction & 7u;
    for (int radius = minimum; radius < maximum; ++radius) {
        CellStruct at = center;
        const auto advance = [&](unsigned index) {
            const auto offset = Unsorted::AdjacentCell[index & 7u];
            at.X = short(at.X + offset.X);
            at.Y = short(at.Y + offset.Y);
        };
        for (int i = 0; i < radius; ++i)
            advance(facing);
        const auto check = [&](unsigned index, int count) {
            for (int i = 0; i < count; ++i) {
                advance(index);
                if (IsCellOccupied(MapClass::Instance.GetCellAt(at), static_cast<FacingType>(index & 7u), -1, nullptr,
                                   true) == Move::OK) {
                    *output = at;
                    return true;
                }
            }
            return false;
        };
        if (facing & 1u) {
            if (check(facing + 3, 2 * radius)) return output;
        } else if (check(facing + 2, radius) || check(facing - 4, radius))
            return output;
    }
    *output = CellStruct::Empty;
    return output;
}
