// Foundation geometry only, original 4A94F0. No display/placement side effects.
#include "yrpp/DisplayClass.h"
#include <algorithm>

CellStruct* DisplayClass::FoundationBoundsSize(CellStruct& dest,
    CellStruct const* const foundation) const {
    if (!foundation) { dest = {0, 0}; return &dest; }
    int min_x = 512, min_y = 512, max_x = -512, max_y = -512;
    for (auto* cell = foundation; cell->X != 0x7fff || cell->Y != 0x7fff; ++cell) {
        min_x = std::min(min_x, static_cast<int>(cell->X));
        min_y = std::min(min_y, static_cast<int>(cell->Y));
        max_x = std::max(max_x, static_cast<int>(cell->X));
        max_y = std::max(max_y, static_cast<int>(cell->Y));
    }
    dest.X = static_cast<short>(std::max(1, max_x - min_x + 1));
    dest.Y = static_cast<short>(std::max(1, max_y - min_y + 1));
    return &dest;
}

// Non-template interface helpers; bodies retained from the corresponding header.

CellStruct DisplayClass::FoundationBoundsSize(CellStruct const* const pFoundationData) const
{
    CellStruct outBuffer;
    FoundationBoundsSize(outBuffer, pFoundationData);
    return outBuffer;
}

DisplayClass::DisplayClass()
    : MapClass(), CurrentFoundation_CenterCell{}, CurrentFoundation_TopLeftOffset{}, CurrentFoundation_Data{}, unknown_1180{},
      unknown_1181{}, CurrentFoundationCopy_CenterCell{}, CurrentFoundationCopy_TopLeftOffset{}, CurrentFoundationCopy_Data{},
      CurrentBuildingCopy{}, CurrentBuildingTypeCopy{}, CurrentBuildingOwnerArrayIndexCopy{}, FollowObject{},
      ObjectToFollow{}, CurrentBuilding{}, CurrentBuildingType{}, CurrentBuildingOwnerArrayIndex{},
      RepairMode{}, SellMode{}, PowerToggleMode{}, PlanningMode{},
      PlaceBeaconMode{}, CurrentSWTypeIndex{}, DraggedWaypoint{}, DraggedWaypointCoords{},
      WaypointColorRed{}, WaypointColorGreen{}, WaypointColorBlue{},
      LeftPressAndDraggingRectangle{}, unknown_bool_11D0{}, unknown_bool_11D1{}, LeftDownPosition{},
      unknown_11DC{} {
    CurrentBuildingOwnerArrayIndexCopy = CurrentBuildingOwnerArrayIndex = -1;
    CurrentSWTypeIndex = -1;
}
DisplayClass::~DisplayClass() = default;
