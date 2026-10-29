// Original local aircraft guard target scan, YR 0x500300.
#include "yrpp/HouseClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/MapClass.h"
#include "yrpp/UnitClass.h"
#include "x87_integer.hpp"
TechnoClass* HouseClass::FindNearbyEnemy(const CoordStruct& at) {
    const auto origin = CellClass::Coord2Cell(at);
    TechnoClass* best = nullptr;
    int distance = 0;
    for (unsigned i = 0; i < 369; ++i) {
        const auto offset = CellSpread::GetCell(i);
        const CellStruct cell{short(origin.X + offset.X), short(origin.Y + offset.Y)};
        auto* target = MapClass::Instance.GetCellAt(cell)->GetUnit(true);
        if (!target || target->InLimbo) continue;
        auto* owner = target->GetOwningHouse();
        if (owner && (owner == this || IsAlliedWith(owner))) continue;
        const auto position = target->GetCoords();
        if (target->Owner->WhichZone(position) != -1 || target->GetHeight() < -20 || target->CloakState == CloakState::Cloaked)
            continue;
        const auto delta = at - position;
        const int candidate = game::x87_integer(Math::sqrt(double(delta.X) * delta.X +
                                                          double(delta.Z) * delta.Z + double(delta.Y) * delta.Y));
        if (!distance || candidate < distance) { distance = candidate; best = target; }
    }
    return best;
}
