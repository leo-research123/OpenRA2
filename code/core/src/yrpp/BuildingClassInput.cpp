#include "debug/debug_state.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp What_Action / Active_Click_With.
// YR 0x447210/0x447540/0x443410/0x4436F0: stationary and garrison input.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/VoxClass.h"
#include "type_resources.hpp"

namespace {
bool yard_redeploy_allowed(const BuildingClass& building) {
    if (!(building.Owner->IsControlledByHuman() || game::debug_can_control_house(building.Owner))) return false;
    const auto& session = SessionClass::Instance;
    return session.GameMode != GameMode::Campaign && session.Config.MCVRedeploy && !building.MindControlledBy;
}
}

bool BuildingClass::IsUnitFactory() const {
    return Type->Factory == AbstractType::UnitType || Type->Factory == AbstractType::InfantryType || Type->Cloning ||
           Type->UnitRepair;
}
bool BuildingClass::IsControllable() const {
    if (static_cast<int>(EMPLockRemaining) > 0) return false;
    if (IsUnitFactory()) return true;
    if (Type->ConstructionYard && !yard_redeploy_allowed(*this)) return false;
    return Type->UndeploysInto != nullptr;
}
Action BuildingClass::MouseOverObject(const ObjectClass* target, bool ignoreForce) const {
    if (Type->InvisibleInGame || (target->WhatAmI() == AbstractType::Building &&
                                  static_cast<const BuildingClass*>(target)->Type->InvisibleInGame))
        return Action::None;
    auto action = TechnoClass::MouseOverObject(target, ignoreForce);
    if (action == Action::ToggleSelect) return action;
    if (action == Action::Self_Deploy) {
        if (Type->GapGenerator && Owner == HouseClass::CurrentPlayer) {
            if (!GapSuperCharged) {
                const auto drain = Owner->Power_Drain();
                if (Owner->Power_Output() - drain <= Type->ExtraPowerDrain) return Action::None;
            }
        } else if (!((Type->UnitAbsorb || Type->InfantryAbsorb) && Passengers.NumPassengers > 0) &&
                   GetOccupantCount() <= 0 && Type->Factory == AbstractType::None)
            return Action::None;
        return Type->Factory != AbstractType::None && IsPrimaryFactory ? Action::None : Action::Self_Deploy;
    }
    if (action == Action::Attack) {
        if (!GetWeapon(0)->WeaponType) return target->GetType()->Immune ? Action::NoMove : action;
        if (!GetWeapon(0)->WeaponType->Projectile->AG || Type->EMPulseCannon) action = Action::None;
        if (CurrentMission == Mission::Selling) return Action::None;
    }
    if (action == Action::Move || action == Action::NoMove) {
        if (!IsControllable()) return Action::Select;
        if (Type->Factory == AbstractType::InfantryType || Type->Factory == AbstractType::UnitType ||
            Type->Factory == AbstractType::AircraftType) {
            if (!InputManagerClass::Instance->IsForceMoveKeyPressed()) return Action::Select;
            const auto coords = target->GetCoords();
            const CellStruct cell{short(coords.X / 256), short(coords.Y / 256)};
            if (Type->Factory != AbstractType::AircraftType) {
                if (!const_cast<BuildingClass*>(this)->IsInSameZoneAsCoords(
                        {int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0}))
                    action = Action::NoMove;
                if (!MapClass::Instance.GetCellAt(cell)->ContainsBridge() &&
                    MapClass::Instance.GetCellAt(cell)->FirstObject)
                    return Action::NoMove;
            }
        }
    }
    if (action == Action::Attack && target->GetType()->Immune) return Action::NoMove;
    return action;
}
Action BuildingClass::MouseOverCell(const CellStruct* cell, bool checkFog, bool ignoreForce) const {
    if (Type->InvisibleInGame) return Action::None;
    auto action = TechnoClass::MouseOverCell(cell, Type->UndeploysInto && checkFog ? false : checkFog, ignoreForce);
    if (Type->UndeploysInto && checkFog &&
        !game::debug_player_location_shrouded(MapClass::Instance.GetCellAt(*cell)->GetCoords()) &&
        action == Action::Move && MapClass::Instance.GetCellAt(*cell)->LandType == LandType::Tiberium &&
        Type->ResourceDestination && Type->ResourceGatherer)
        return Action::Harvest;
    if (action == Action::Move || action == Action::NoMove) {
        if (!IsControllable() && action == Action::Move)
            action = Action::None;
        else if (IsUnitFactory()) {
            if (!InputManagerClass::Instance->IsForceMoveKeyPressed())
                action = Action::None;
            else if (Type->Factory != AbstractType::AircraftType) {
                if (!const_cast<BuildingClass*>(this)->IsInSameZoneAsCoords(
                        {int(cell->X) * 256 + 128, int(cell->Y) * 256 + 128, 0}))
                    action = Action::NoMove;
                if (!MapClass::Instance.GetCellAt(*cell)->ContainsBridge() &&
                    MapClass::Instance.GetCellAt(*cell)->FirstObject)
                    action = Action::NoMove;
            }
        }
    }
    if (action == Action::Attack) {
        if (!GetWeapon(0)->WeaponType) return action;
        if (!GetWeapon(0)->WeaponType->Projectile->AG || Type->EMPulseCannon) action = Action::None;
        if (CurrentMission == Mission::Selling) action = Action::None;
        if (action == Action::Attack) return action;
    }
    if (IsUnitFactory() && game::debug_can_control_house(Owner)) return Action::Move;
    return action;
}
bool BuildingClass::ObjectClickedAction(Action action, ObjectClass* target, bool) {
    if (action == Action::Attack) {
        if (target) ClickedMission(Mission::Attack, target, nullptr, nullptr);
        return true;
    }
    if (action == Action::Self_Deploy && GetOccupantCount() > 0) {
        ClickedMission(Mission::Unload, target, nullptr, nullptr);
        return true;
    }
    return false;
}
bool BuildingClass::CellClickedAction(Action action, CellStruct* cell, CellStruct*, bool) {
    if (action == Action::Attack) {
        ClickedMission(Mission::Attack, MapClass::Instance.GetCellAt(*cell), nullptr, nullptr);
        return true;
    }
    if (action == Action::Move || action == Action::Harvest) {
        const bool blocked = Type->ConstructionYard && !yard_redeploy_allowed(*this);
        if (!blocked && (Type->UndeploysInto || IsUnitFactory())) SetRallypoint(cell, true);
        if (Type->UndeploysInto && !blocked) {
            // Archive precedes Sell in the original queue, including its capacity failures.
            const TargetClass target(this);
            const int issuingHouse = game::debug_can_control_house(Owner) && HouseClass::CurrentPlayer
                                         ? HouseClass::CurrentPlayer->ArrayIndex
                                         : GetOwningHouseIndex();
            EventClass event(issuingHouse, EventType::Sell, target.m_ID, target.m_RTTI);
            EventClass::OutList.Add(event);
        }
    }
    return true;
}

void BuildingClass::SetDestination(AbstractClass* target, bool immediate) {
    if (CurrentMission == Mission::Selling) return;
    if (IsUnitFactory() || Type->ConstructionYard) SetArchiveTarget(target);
    TechnoClass::SetDestination(target, immediate);
}

void BuildingClass::SetRallypoint(CellStruct* target, bool playEVA) {
    auto& map = MapClass::Instance;
    const bool bridge = (map.GetCellAt(*target)->Flags & CellFlags::BridgeHead) != CellFlags::Empty;
    const auto movement = Type->Naval                                   ? MovementZone::AmphibiousCrusher
                          : Type->Factory == AbstractType::AircraftType ? MovementZone::Fly
                                                                        : MovementZone::Normal;
    const auto speed = Type->Naval                                   ? SpeedType::Amphibious
                       : Type->Factory == AbstractType::AircraftType ? SpeedType::Winged
                                                                     : SpeedType::Foot;
    const int zone = map.GetMovementZoneType(CellClass::Coord2Cell(Location), movement, bridge);
    auto next = map.NearByLocation(*target, speed, zone, movement, bridge, 1, 1, false, false, false, true,
                                   CellStruct{0, 0}, false, false);
    if (next == CellStruct::Empty && Type->ConstructionYard && yard_redeploy_allowed(*this))
        next = CellClass::Coord2Cell(GetCoords());
    if (next != CellStruct::Empty) {
        EventClass event;
        event.Type = EventType::Archive;
        event.HouseIndex = static_cast<char>(GetOwningHouseIndex());
        event.Frame = Unsorted::CurrentFrame;
        event.Archive = {TargetClass(this), TargetClass(map.GetCellAt(next))};
        EventClass::OutList.Add(event);
    }
    if (playEVA && game::debug_can_control_house(Owner) && !Type->ConstructionYard && !Type->ResourceGatherer &&
        !game::type_resources().audio_unavailable)
        VoxClass::Play("EVA_NewRallyPointEstablished", -1, -1);
}
