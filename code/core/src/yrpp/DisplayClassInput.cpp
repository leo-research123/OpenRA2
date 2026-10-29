// YR 0x692300 / 0x692610: tactical picking, visibility and action precedence.
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "map_runtime.hpp"
#include "debug/debug_state.hpp"

namespace {
TechnoClass* as_techno(ObjectClass* object) {
    if (!object) return nullptr;
    switch (object->WhatAmI()) {
        case AbstractType::Unit:
        case AbstractType::Aircraft:
        case AbstractType::Building:
        case AbstractType::Infantry:
            return static_cast<TechnoClass*>(object);
        default:
            return nullptr;
    }
}
bool sensed(const TechnoClass& object) {
    return MapClass::Instance.GetCellAt(object.GetCoords())->Sensors_InclHouse(HouseClass::CurrentPlayer->ArrayIndex);
}
}
bool DisplayClass::ProcessClickCoords(Point2D* point, CellStruct* cell, CoordStruct* coords, ObjectClass** target,
                                      BYTE* fogged, BYTE* shrouded) {
    *target = nullptr;
    *cell = {0, 0};
    *coords = {0, 0, 0};
    *fogged = *shrouded = 0;
    if (point->X < 0 || point->Y < 0) return false;
    auto* tactical = TacticalClass::Instance;
    if (!tactical) return false;
    const auto& bounds = DSurface::ViewBounds;
    const Point2D absolute{point->X + bounds.X, point->Y + bounds.Y};
    if (!tactical->PickTerrainCell(absolute, bounds, *cell)) return false;
    tactical->ClientToCoords(coords, absolute);
    if (!CoordinatesLegal(*coords)) return false;
    CoordStruct center{int(cell->X) * 256 + 128, int(cell->Y) * 256 + 128, 0};
    center.Z = GetCellFloorHeight(center);
    const auto window_present = game::map_runtime().has_window;
    const bool window = window_present && window_present();
    *shrouded = game::debug_player_location_shrouded(center) && window;
    if (!game::debug_fog_hidden() && ScenarioClass::Instance->SpecialFlags.FogOfWar) {
        if (auto* building = GetCellAt(center)->GetBuilding())
            *fogged = building->IsFogged;
        else
            *fogged = IsLocationFogged(center) && window;
    }
    if (!*shrouded && !*fogged) {
        *target = tactical->GetSelectableObject(*point);
        auto* techno = as_techno(*target);
        if (techno && !techno->IsOwnedByCurrentPlayer &&
            ((techno->CloakState == CloakState::Cloaked && !sensed(*techno)) || techno->GetTechnoType()->Invisible))
            *target = nullptr;
        if (*target && (*target)->WhatAmI() == AbstractType::Building) {
            const auto* building = static_cast<BuildingClass*>(*target);
            if (!building->IsOwnedByCurrentPlayer &&
                ((building->Translucency == 15 && !sensed(*building)) || building->Type->InvisibleInGame))
                *target = nullptr;
        }
    }
    return true;
}
Action DisplayClass::DecideAction(const CellStruct& location, ObjectClass* object, DWORD checkFog) {
    if (game::debug_selection_expanded() && object && object->WhatAmI() != AbstractType::Building &&
        game::debug_can_select_house(object->GetOwningHouse()) && object->CanBeSelected() &&
        InputManagerClass::Instance->IsForceSelectKeyPressed())
        return Action::ToggleSelect;
    if (ObjectClass::CurrentObjects.Count) {
        if (object) return Unsorted::BestSelectedObject(nullptr, object)->MouseOverObject(object, false);
        return Unsorted::BestSelectedObject(&location, nullptr)->MouseOverCell(&location, checkFog != 0, false);
    }
    auto action = Action::None;
    auto* unit = as_techno(object);
    bool visible = true;
    if (unit && !unit->IsOwnedByCurrentPlayer &&
        ((unit->CloakState == CloakState::Cloaked && !sensed(*unit)) || unit->GetTechnoType()->Invisible))
        visible = false;
    if (object && object->WhatAmI() == AbstractType::Building &&
        !static_cast<BuildingClass*>(object)->IsOwnedByCurrentPlayer &&
        static_cast<BuildingClass*>(object)->Translucency == 15 && !sensed(*unit))
        visible = false;
    auto* where = const_cast<CellStruct*>(&location);
    if (visible) {
        if (object && object->CanBeSelected() &&
            (object->WhatAmI() != AbstractType::Building || game::debug_fog_hidden() ||
             !static_cast<BuildingClass*>(object)->IsFogged) &&
            (!unit || !unit->IsALoaner))
            action = Action::Select;
        const auto* cell = GetCellAt(location);
        if (BeaconManagerClass::Instance.GetBeaconAt(int(location.X) * 256 + 128, int(location.Y) * 256 + 128,
                                                     Unsorted::LevelHeight * static_cast<signed char>(cell->Level)))
            action = Action::SelectBeacon;
    }
    if (RepairMode)
        action = object && object->GetOwningHouse() && game::debug_can_control_house(object->GetOwningHouse()) &&
                         object->CanBeRepaired()
                     ? Action::Eaten
                     : Action::NoRepair;
    if (PowerToggleMode) {
        action = Action::NoTogglePower;
        if (object && object->GetOwningHouse() && game::debug_can_control_house(object->GetOwningHouse()) &&
            object->WhatAmI() == AbstractType::Building && object->CanBeSelected() && !object->IsStrange() &&
            static_cast<BuildingClass*>(object)->Type->CanTogglePower())
            action = Action::TogglePower;
    }
    auto* house = HouseClass::CurrentPlayer;
    auto* waypoint = house->GetPlanningWaypointAt(where);
    if (PlanningMode) {
        const bool extend = InputManagerClass::Instance->IsForceSelectKeyPressed();
        int path = -1;
        BYTE index = 0;
        if (waypoint) house->GetPlanningWaypointProperties(waypoint, path, index);
        if (DraggedWaypoint)
            action = Action::DragWaypoint;
        else if (extend && waypoint && path == house->SelectedPathIndex && house->CanAddPlanningWaypoint() &&
                 house->PlanningPaths[path]->GetWaypointAfter(waypoint))
            action = Action::LoopWaypointPath;
        else if (waypoint)
            action = Action::SelectWaypoint;
        else
            action =
                house->SelectedPathIndex != -1 && house->CanAddPlanningWaypoint() && IsWithinUsableArea(location, true)
                    ? Action::PlaceWaypoint
                    : Action::TibSunBug;
    }
    if (PlaceBeaconMode) action = Action::PlaceBeacon;
    if (SellMode) {
        if (object && object->GetOwningHouse() && game::debug_can_control_house(object->GetOwningHouse()) &&
            object->CanBeSold())
            action =
                object->WhatAmI() == AbstractType::Building && !object->IsStrange() ? Action::Sell : Action::NoSell;
        else {
            CoordStruct at{int(location.X) * 256 + 128, int(location.Y) * 256 + 128, 0};
            at.Z = GetCellFloorHeight(at);
            const bool wall = !IsLocationShrouded(at) && !IsLocationFogged(at) &&
                              GetCellAt(location)->OverlayTypeIndex != -1 &&
                              OverlayTypeClass::Array[GetCellAt(location)->OverlayTypeIndex]->Wall &&
                              GetCellAt(location)->WallOwnerIndex != -1 &&
                              HouseClass::Array[GetCellAt(location)->WallOwnerIndex]->IsControlledByCurrentPlayer();
            action = wall ? Action::Sell : Action::NoSell;
        }
    }
    if (CurrentSWTypeIndex != -1) {
        const auto special = SuperWeaponTypeClass::Array[CurrentSWTypeIndex]->MouseOverObject(location, object);
        if (special != Action::None) action = special;
    }
    if (action == Action::None && waypoint) action = Action::EnterWaypointMode;
    return CurrentBuildingType ? Action::None : action;
}

bool DisplayClass::ScrollMap(DWORD direction, int* distance, bool apply) {
    if (!apply) return *distance && TacticalClass::Instance->GetScrollDirection(static_cast<int>(direction)) != -1;
    TacticalClass::Instance->ScrollBy(static_cast<int>(direction), *distance);
    return true;
}
