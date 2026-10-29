// Original 0x6D527C editor-preview relocation, advanced once per tactical round.
#include "tactical_placement_cycle.hpp"
#include "map_view.hpp"
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
namespace game {
void advance_tactical_placement(const MapViewHandle& view) noexcept {
    auto& display = DisplayClass::Instance;
    const auto& bounds = TacticalClass::ViewBounds;
    const auto point = view.pointer;
    if (!display.CurrentBuildingType || !view.pointer_inside || point.X < bounds.X || point.Y < bounds.Y ||
        point.X >= bounds.X + bounds.Width || point.Y >= bounds.Y + bounds.Height) return;
    if (!Unsorted::ArmageddonMode || !display.CurrentBuilding ||
        display.CurrentBuilding->WhatAmI() != AbstractType::Building) return;
    const auto center = display.CurrentFoundation_CenterCell, offset = display.CurrentFoundation_TopLeftOffset;
    const int gateOffset = static_cast<BuildingClass*>(display.CurrentBuilding)->Type->Gate ? -1 : 0;
    CoordStruct at{int(short(center.X + offset.X + gateOffset)) * 256,
                   int(short(center.Y + offset.Y + gateOffset)) * 256, 0};
    at.Z = display.GetCellFloorHeight(at);
    display.CurrentBuilding->SetLocation(at);
}
}
