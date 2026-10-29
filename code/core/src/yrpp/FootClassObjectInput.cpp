#include "debug/debug_state.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 foot.cpp::Active_Click_With, calibrated to YR 0x4D74E0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x4D74E0: object orders, including movement-zone fallback and follow cells.
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BombClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/Unsorted.h"

#if !defined(RA2_YRPP_GAME)
bool FootClass::ObjectClickedAction(Action action, ObjectClass* target, bool ignoreForce) {
    if (Berzerk || (DirectRockerLinkedUnit && !GetTechnoType()->Pushy)) return false;
    // 0x4D74E0 consumes the action chosen by the derived entry; unlike the
    // cell overload it must NOT call MouseOverObject again here.
    switch (action) {
        case Action::Self_Deploy:
        case Action::AreaAttack:
        case Action::DetonateAll:
            ClickedMission(Mission::Unload, nullptr, nullptr, nullptr);
            return true;
        case Action::Attack:
        case Action::IvanBomb:
        case Action::AttackMoveTar:
        case Action::Demolish:
        case Action::Airstrike:
            if (!IsActive()) return false;
            ClickedMission(Mission::Attack, target, nullptr, nullptr);
            return true;
        case Action::PatrolWaypoint:
        case Action::NoIvanBomb:
            return false;
        case Action::AttackSupport: {
            auto* slot = GetWeapon(0);
            const bool heals = slot && slot->WeaponType && slot->WeaponType->Damage < 0;
            ClickedMission(heals ? Mission::Area_Guard : Mission::Guard, this, nullptr, nullptr);
            return true;
        }
        case Action::Capture:
            if (!IsControllable()) return false;
            ClickedMission(Mission::Capture, nullptr, target, nullptr);
            return true;
        case Action::Enter:
            if (!IsControllable() || !target) return false;
            switch (target->WhatAmI()) {
                case AbstractType::Aircraft:
                case AbstractType::Infantry:
                case AbstractType::Unit:
                case AbstractType::Building:
                    break;
                default:
                    return false;
            }
            ClickedMission(Mission::Enter, nullptr, target, nullptr);
            return true;
        case Action::Repair:
            if (!IsControllable()) return false;
            ClickedMission(Mission::Eaten, nullptr, target, nullptr);
            return true;
        case Action::Move:
        case Action::NoMove:
        case Action::AttackMoveNav: {
            auto& map = MapClass::Instance;
            if (action == Action::NoMove && game::debug_player_location_shrouded(target->GetCoords()) &&
                !GetTechnoType()->MoveToShroud)
                return false;
            if (!IsControllable()) return false;
            AbstractClass* destination = target;
            auto at = target->GetCoords();
            if (at.Z <= map.GetCellFloorHeight(at)) at.Z = map.GetCellFloorHeight(at);
            if (((GetTechnoType()->IsSubterranean && !RulesClass::Instance->AllowShroudedSubteranneanMoves) ||
                 WhatAmI() == AbstractType::Aircraft) &&
                game::debug_player_location_shrouded(at))
                return true;
            auto zone = GetTechnoType()->MovementZone;
            auto wanted = target->GetDestination();
            CellStruct cell{short(wanted.X / 256), short(wanted.Y / 256)};
            const bool inMap = map.IsWithinUsableArea(cell, true);
            auto* from = map.GetCellAt(GetDestination());
            if (zone == MovementZone::Subterrannean)
                zone = MovementZone::Normal;
            else if (zone == MovementZone::Fly)
                zone = MovementZone::Infantry;
            const bool freeZone =
                IsInAir() || GetTechnoType()->IsSubterranean ||
                (WhatAmI() == AbstractType::Infantry && static_cast<InfantryClass*>(this)->Type->JumpJet) ||
                WhatAmI() == AbstractType::Aircraft;
            const bool fromBridge = IsOnBridge(nullptr);
            CoordStruct center{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
            center.Z = map.GetCellFloorHeight(center);
            const bool shrouded = game::debug_player_location_shrouded(center);
            const bool forceMove = !ignoreForce && InputManagerClass::Instance->IsForceMoveKeyPressed();
            if (freeZone && action == Action::NoMove) {
                const bool underground = GetTechnoType()->IsSubterranean && GetHeight() < 0;
                const bool bridge = (unsigned(map.GetCellAt(cell)->Flags) & 0x100u) != 0;
                const auto nearby = map.NearByLocation(cell, GetTechnoType()->SpeedType, -1, zone, bridge, 1, 1, false,
                                                       true, underground, true, {0, 0}, false, false);
                if (nearby != CellStruct::Empty) destination = map.GetCellAt(nearby);
            } else if (!inMap ||
                       (!freeZone &&
                        (shrouded || action == Action::NoMove || (forceMove && WhatAmI() == AbstractType::Infantry) ||
                         !map.IsSameCellZone(from->MapCoords, cell, zone, fromBridge,
                                             (unsigned(map.GetCellAt(cell)->Flags) & 0x100u) != 0, false)))) {
                const bool bridge = (unsigned(map.GetCellAt(cell)->Flags) & 0x100u) != 0;
                const int sourceZone = map.GetMovementZoneType(from->MapCoords, zone, fromBridge);
                const auto nearby = map.NearByLocation(cell, GetTechnoType()->SpeedType, sourceZone, zone, bridge, 1, 1,
                                                       false, true, false, true, {0, 0}, false, false);
                if (nearby != CellStruct::Empty) destination = map.GetCellAt(nearby);
            }
            if (destination) {
                CellClass* follow = nullptr;
                if (destination != target && target) {
                    const auto coords = target->GetCoords();
                    follow = map.GetCellAt(CellStruct{short(coords.X / 256), short(coords.Y / 256)});
                }
                ClickedMission(Mission::Move, nullptr, destination, follow);
            }
            return true;
        }
        case Action::Sabotage:
            if (!IsControllable()) return false;
            ClickedMission(Mission::Sabotage, nullptr, target, nullptr);
            return true;
        case Action::GuardArea:
            if (!IsActive() || !IsControllable()) return false;
            if (((WhatAmI() == AbstractType::Infantry && static_cast<InfantryClass*>(this)->Type->C4) ||
                 HasAbility(Ability::C4)) &&
                target->WhatAmI() == AbstractType::Building && !Owner->IsAlliedWith(target))
                ClickedMission(Mission::Sabotage, nullptr, target, nullptr);
            else
                ClickedMission(Mission::Area_Guard, target, nullptr, nullptr);
            return true;
        case Action::Detonate: {
            const auto self = TargetClass(this);
            EventClass event(AttachedBomb->OwnerHouse->ArrayIndex, EventType::Detonate, self.m_ID, self.m_RTTI);
            EventClass::OutList.Add(event);
            return true;
        }
        default:
            return true;
    }
}

#endif
