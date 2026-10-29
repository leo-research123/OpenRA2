// Original control-group mode retirement and selection camera (0x004AE290).
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>

void DisplayClass::CancelSelectionModes() noexcept {
    // Only the off branches of 0x4AC820 and 0x4AC700 are needed here. These
    // legacy modes are distinct from the PlanMgr graph.
    if (PowerToggleMode && !CurrentBuildingType) {
        SetCursor(MouseCursorType::Default, false);
        SellMode = RepairMode = PlanningMode = PlaceBeaconMode = PowerToggleMode = false;
        RestoreCursor();
    }
    if (PlanningMode && !CurrentBuildingType) {
        SellMode = RepairMode = PowerToggleMode = PlaceBeaconMode = PlanningMode = false;
        RestoreCursor();
        if (DraggedWaypoint) {
            DraggedWaypoint->Coords = DraggedWaypointCoords;
            DraggedWaypoint = nullptr;
            if (WWMouseClass::Instance) WWMouseClass::Instance->ShowCursor();
        }
    }
    SetRepairMode(0);
    SetSellMode(0);
    SetBeaconMode(0);
}
#if !defined(RA2_YRPP_GAME)
void DisplayClass::CenterOnSelection() noexcept {
    const int count = ObjectClass::CurrentObjects.Count;
    if (count <= 0) return;
    const auto add = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
    const auto sub = [](int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
    CoordStruct sum{0, 0, 0};
    for (int i = 0; i < count; ++i) {
        const auto& at = ObjectClass::CurrentObjects[i]->Location;
        sum = {add(sum.X, at.X), add(sum.Y, at.Y), add(sum.Z, at.Z)};
    }
    CoordStruct center{sum.X / count, sum.Y / count, sum.Z / count};
    if (count > 2) {
        int furthest = 0;
        CoordStruct excluded{0, 0, 0};
        for (int i = 0; i < count; ++i) {
            const auto& at = ObjectClass::CurrentObjects[i]->Location;
            const double x = sub(center.X, at.X), y = sub(center.Y, at.Y), z = sub(center.Z, at.Z);
            const int distance = game::x87_integer(Math::sqrt(x * x + y * y + z * z));
            if (distance > furthest) {
                furthest = distance;
                excluded = at;
            }
        }
        center = {sub(sum.X, excluded.X) / (count - 1), sub(sum.Y, excluded.Y) / (count - 1),
                  sub(sum.Z, excluded.Z) / (count - 1)};
    }
    TacticalClass::Instance->SetTacticalPosition(&center);
}
#endif
