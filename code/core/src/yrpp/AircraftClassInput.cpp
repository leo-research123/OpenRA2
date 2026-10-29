#include "debug/debug_state.hpp"
// YR 0x417CC0 / 0x417F80: aircraft hover actions, including dock radio queries.
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"

Action AircraftClass::MouseOverCell(const CellStruct* cell, bool checkFog, bool ignoreForce) const {
    if (!game::debug_can_control_house(Owner)) return Action::None;
    const auto action = FootClass::MouseOverCell(cell, checkFog, ignoreForce);
    if (action == Action::Attack && !GetWeapon(0)->WeaponType) return Action::None;
    return action;
}
Action AircraftClass::MouseOverObject(const ObjectClass* object, bool ignoreForce) const {
    auto action = FootClass::MouseOverObject(object, ignoreForce);
    if (action == Action::ToggleSelect) return action;
    auto* target = const_cast<ObjectClass*>(object);
    if (action == Action::GuardArea && Type->AirportBound) action = Action::None;
    if (Type->Carryall && game::debug_can_control_house(Owner) &&
        (action == Action::Select || action == Action::None) && Owner->IsAlliedWith(target) &&
        ((target->AbstractFlags & ::AbstractFlags::Techno) == ::AbstractFlags::None ||
         target->GetOwningHouse()->IsAlliedWith(this)) &&
        !Passengers.FirstPassenger && target->WhatAmI() == AbstractType::Unit)
        action = Action::Tote;
    if (action == Action::None) {
        const auto at = target->GetCoords();
        const CellStruct cell{short(at.X / 256), short(at.Y / 256)};
        action = MouseOverCell(&cell, false, ignoreForce);
    }
    if (action == Action::Self_Deploy) {
        if (!Passengers.NumPassengers) action = Action::None;
    } else if (action == Action::Attack && !GetWeapon(0)->WeaponType)
        action = Action::None;
    if (game::debug_can_control_house(Owner) && action == Action::Select &&
        target->WhatAmI() == AbstractType::Building) {
        auto* building = static_cast<BuildingClass*>(target);
        if (building->Type->UnitRepair) {
            if (Type->Dock.FindItemIndex(building->Type) < 0 || !building->HasFreeLink(this) ||
                building->Passengers.FirstPassenger ||
                const_cast<AircraftClass*>(this)->SendCommand(static_cast<RadioCommand>(15), building) !=
                    RadioCommand::AnswerPositive)
                action = Action::NoEnter;
            else
                action = Action::Enter;
        }
    }
    if (Type->Carryall && action == Action::Tote) {
        const auto cell = target->GetMapCoords();
        if (cell != CellStruct::Empty)
            if (auto* building = MapClass::Instance.GetCellAt(cell)->GetBuilding();
                building && building->Type->WeaponsFactory)
                return Action::None;
    } else if (action == Action::NoMove) {
        if (target->IsDisguised() && !target->GetDisguiseHouse(true) &&
            target->GetDisguise(true)->WhatAmI() == AbstractType::TerrainType)
            return Action::Move;
    } else if (action == Action::Attack && target->GetType()->Immune)
        return Action::NoMove;
    return action;
}
