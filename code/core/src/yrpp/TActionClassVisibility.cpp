// Original waypoint visibility actions, YR 0x006E0FE0 / 0x006E1A70.
#include "yrpp/TActionClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include <bit>

namespace {
CoordStruct waypoint_position(int waypoint) {
    const auto cell = ScenarioClass::Instance->GetWaypointCoords(waypoint);
    auto& map = MapClass::Instance;
    const int bridge =
        (unsigned(map.GetCellAt(cell)->Flags) & 0x100u) || (unsigned(map.GetCellAt(cell)->Flags) & 0x400u) ? 4 : 0;
    const int height = bridge + static_cast<signed char>(map.GetCellAt(cell)->Level);
    const auto center = [height](short coordinate) {
        const short projected = std::bit_cast<short>(static_cast<unsigned short>(int(coordinate) - height / 2));
        return int(projected) * 256 + 128;
    };
    return {center(cell.X), center(cell.Y), height * Unsorted::LevelHeight};
}
}

bool TActionClass::RevealAroundWaypoint(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (HouseClass::CurrentPlayer->Visionary) return true;
    auto position = waypoint_position(Value);
    const auto original = position;
    MapClass::Instance.RevealArea2(&position, RulesClass::Instance->RevealTriggerRadius, HouseClass::CurrentPlayer, 0,
                                   0, 0, 1, 0);
    // The second call returns the temporary sight reference, leaving the
    // explored cells mapped without manufacturing a permanent sight source.
    position = original;
    MapClass::Instance.RevealArea2(&position, RulesClass::Instance->RevealTriggerRadius, HouseClass::CurrentPlayer, 0,
                                   0, 0, 1, 1);
    return true;
}

bool TActionClass::ReshroudMapAroundWaypoint(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (HouseClass::CurrentPlayer->Visionary) return true;
    auto position = waypoint_position(Value);
    MapClass::ShroudArea(&position, RulesClass::Instance->RevealTriggerRadius);
    MapClass::Instance.RecalculateShroud();
    return true;
}
