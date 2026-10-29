#include "debug/debug_state.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp::What_Action / Can_Player_Move / Can_Player_Fire.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md. YR 0x700600/0x700C40/0x7010D0.
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BombClass.h"
#include "yrpp/FoggedObjectClass.h"
#include "yrpp/VeinholeMonsterClass.h"
#include "yrpp/SessionClass.h"
#include <cstdlib>

Action TechnoClass::MouseOverObject(const ObjectClass* object, bool ignoreForce) const {
    if (Berzerk || (DirectRockerLinkedUnit && !GetTechnoType()->Pushy) || !object) return Action::None;
    auto* target = const_cast<ObjectClass*>(object);
    const auto selectable = [&](bool checkWarp) {
        if ((!IsALoaner || !IsOwnedByCurrentPlayer) && target->CanBeSelected() && !target->IsSelected &&
            (!checkWarp || !target->IsBeingWarpedOut()))
            return Action::Select;
        return Action::None;
    };
    if (IsBeingWarpedOut())
        return (!IsArmed() || !game::debug_can_control_house(Owner) ||
                target->GetOwningHouseIndex() == GetOwningHouseIndex())
                   ? selectable(false)
                   : Action::None;
    if (target == this && CurrentObjects.Count == 1 && AttachedBomb &&
        AttachedBomb->OwnerHouse->IsControlledByCurrentPlayer()) {
        if (AttachedBomb->IsDeathBomb() == 1 && RulesClass::Instance->CanDetonateDeathBomb) return Action::Detonate;
        if (!AttachedBomb->IsDeathBomb() && RulesClass::Instance->CanDetonateTimeBomb) return Action::Detonate;
    }
    const auto* input = InputManagerClass::Instance;
    const bool move = !ignoreForce && input && input->IsForceMoveKeyPressed();
    bool fire = !ignoreForce && input && input->IsForceFireKeyPressed();
    if (!ignoreForce && input && input->IsForceSelectKeyPressed()) {
        if (fire)
            fire = false;
        else if (game::debug_can_control_house(Owner) && !IsALoaner)
            return Action::ToggleSelect;
    }
    if (target == this && CurrentObjects.Count == 1) {
        if (game::debug_can_control_house(Owner)) return Action::Self_Deploy;
        if (CurrentObjects.Count == 1 && Owner->IsInPlayerControl) {
            if (SessionClass::Instance.GameMode == GameMode::Campaign) {
                if (WhatAmI() == AbstractType::Unit && static_cast<const UnitClass*>(this)->Type->Passengers > 0 &&
                    Passengers.NumPassengers > 0)
                    return Action::Self_Deploy;
                if (WhatAmI() == AbstractType::Aircraft && static_cast<const AircraftClass*>(this)->HasPassengers)
                    return Action::Self_Deploy;
            }
        }
    }
    if (game::debug_can_control_house(Owner) && Owner->IsAlliedWith(target) && WhatAmI() != AbstractType::Aircraft &&
        fire && move && IsControllable())
        return Action::GuardArea;
    if (move && game::debug_can_control_house(Owner) && IsControllable()) return Action::Move;
    const auto* type = GetTechnoType();
    const int index = SelectWeapon(target);
    if (target->IsSurfaced()) {
        const auto* weapon = GetWeapon(index)->WeaponType;
        auto* techno = (target->AbstractFlags & ::AbstractFlags::Techno) != ::AbstractFlags::None
                           ? static_cast<TechnoClass*>(target)
                           : nullptr;
        const bool reveal =
            RulesClass::Instance->AttackCursorOnDisguise ||
            (techno && (techno->GetTechnoType()->PermaDisguise || techno->DisguiseBlinkTimer.GetTimeLeft()));
        const bool disguised = target->IsDisguisedAs(Owner) && !reveal;
        const bool unowned_disguise = disguised && !target->GetDisguiseHouse(true);
        const bool terrain = CanDisguiseAs(target);
        const bool clears_cargo =
            weapon && weapon->Warhead->IvanBomb && target->GetTechnoType() && target->GetTechnoType()->Passengers > 0;
        const bool parasite = weapon && weapon->Warhead->Sonic &&
                              (target->AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None &&
                              static_cast<FootClass*>(target)->ParasiteEatingMe;
        const bool friendly = GetTechnoType()->AttackFriendlies ||
                              (GetTechnoType()->AttackCursorOnFriendlies && !clears_cargo) || parasite;
        const bool enemy_or_friendly = !Owner->IsAlliedWith(target) || friendly;
        if (game::debug_can_control_house(Owner) &&
            (fire || (enemy_or_friendly && (!disguised || (unowned_disguise && terrain) || (disguised && friendly)) &&
                      (target->GetType()->LegalTarget || terrain)))) {
            const bool infiltrator =
                WhatAmI() == AbstractType::Infantry && static_cast<const InfantryTypeClass*>(type)->Infiltrate;
            if ((IsArmed() || infiltrator) && (IsControllable() || IsCloseEnough(target, index))) {
                bool special = false;
                if (infiltrator) {
                    const auto* inf = static_cast<const InfantryClass*>(this)->Type;
                    special = (!inf->Agent && target->WhatAmI() == AbstractType::Building &&
                               static_cast<BuildingClass*>(target)->Type->Capturable) ||
                              (inf->Agent && target->WhatAmI() == AbstractType::Building &&
                               static_cast<BuildingClass*>(target)->Type->Spyable) ||
                              (inf->C4 && target->WhatAmI() == AbstractType::Building &&
                               static_cast<BuildingClass*>(target)->Type->CanC4);
                }
                if (GetFireError(target, index, true) != FireError::ILLEGAL || special || fire) return Action::Attack;
            }
        }
    }
    return (!IsArmed() || !game::debug_can_control_house(Owner) ||
            target->GetOwningHouseIndex() == GetOwningHouseIndex())
               ? selectable(true)
               : Action::None;
}

bool TechnoClass::CanBeSelectedNow() const {
    if (SlaveOwner || Deactivated || BunkerLinkedItem) return false;
    if (IsTether && GetCell()->GetBuilding()) return false;
    return ObjectClass::CanBeSelectedNow();
}
bool TechnoClass::IsInSameZoneAsCoords(const CoordStruct& coords) {
    const auto movement = GetTechnoType()->MovementZone;
    if (movement == MovementZone::None || coords == CoordStruct::Empty) return true;
    const CellStruct to{short(coords.X / 256), short(coords.Y / 256)};
    const auto destination = GetDestination();
    const CellStruct from{short(destination.X / 256), short(destination.Y / 256)};
    const bool outside = vt_entry_320();
    const bool bridge = MapClass::Instance.GetCellAt(to)->ContainsBridge();
    return MapClass::Instance.IsSameCellZone(from, to, movement, IsOnBridge(nullptr), bridge, outside);
}
bool TechnoClass::IsActive() const {
    if (Deactivated || !game::debug_can_control_house(Owner)) return false;
    auto* weapon = GetTurretWeapon();
    return weapon && weapon->WeaponType && !IsUnderEMP();
}
bool TechnoClass::vt_entry_310() const {
    if (WhatAmI() != AbstractType::Unit) return true;
    const auto* unit = static_cast<const UnitClass*>(this);
    return (unit->TubeIndex < 0 || !unit->CanDeployNow()) && !IsUnderEMP();
}
bool TechnoClass::IsControllable() const {
    if (!game::debug_can_control_house(Owner) || IsUnderEMP() || BunkerLinkedItem || GetTechnoType()->Spawned ||
        IsParalyzed() || !IsNotWarping() || SlaveOwner || Deactivated)
        return false;
    if (SpawnManager && SpawnManager->CountLaunchingSpawns() > 0) {
        const auto* type = GetTechnoType();
        if (SpawnManager->CountLaunchingSpawns() < type->SpawnsNumber) return false;
    }
    return true;
}
bool TechnoClass::CanDisguiseAs(AbstractClass* target) const {
    // Despite the legacy name, 0x70EF00 means an attackable terrain target.
    auto* weapon = GetWeapon(SelectWeapon(target))->WeaponType;
    if (!weapon) return false;
    if (target->WhatAmI() == AbstractType::Terrain) return RulesClass::Instance->TreeTargeting || weapon->TerrainFire;
    if (!weapon->TerrainFire) return false;
    if (target->WhatAmI() == AbstractType::Cell) {
        const int index = static_cast<CellClass*>(target)->OverlayTypeIndex;
        if (index != -1 && OverlayTypeClass::Array[index]->IsARock) return true;
    }
    if ((target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None) {
        auto* object = static_cast<ObjectClass*>(target);
        return object->IsDisguised() && !object->GetDisguiseHouse(true);
    }
    return false;
}
Action TechnoClass::MouseOverCell(const CellStruct* where, bool checkFog, bool ignoreForce) const {
    if (Berzerk || (DirectRockerLinkedUnit && !GetTechnoType()->Pushy)) return Action::None;
    auto& map = MapClass::Instance;
    auto* cell = map.GetCellAt(*where);
    auto* input = InputManagerClass::Instance;
    bool fire = !ignoreForce && input && input->IsForceFireKeyPressed();
    bool select = !ignoreForce && input && input->IsForceSelectKeyPressed();
    const bool move = !ignoreForce && input && input->IsForceMoveKeyPressed();
    if (select && fire) {
        select = false;
        fire = false;
    }
    if (IsBeingWarpedOut() || IsWarpingIn()) return Action::None;
    int remembered = -1;
    bool remembered_target = false;
    if (ScenarioClass::Instance->SpecialFlags.FogOfWar && checkFog && cell->FoggedObjects) {
        for (int i = 0; i < cell->FoggedObjects->Count; ++i) {
            auto* object = (*cell->FoggedObjects)[i];
            if (!object->CanDraw) continue;
            if (object->RTTI == AbstractType::Overlay)
                remembered = int(object->Overlay);
            else if (object->RTTI == AbstractType::Building &&
                     (!object->House || !HouseClass::CurrentPlayer->IsAlliedWith(object->House)) &&
                     (!object->GetHeadRecordObjectType() || object->GetHeadRecordObjectType()->LegalTarget))
                remembered_target = true;
        }
    }
    const int overlay_index = remembered != -1 ? remembered : cell->OverlayTypeIndex;
    auto* overlay = overlay_index != -1 ? OverlayTypeClass::Array[overlay_index] : nullptr;
    const bool engineer = IsEngineer();
    if (game::debug_can_control_house(Owner) && ((fire && move) || unknown_bool_43A) && IsControllable() &&
        (IsActive() || engineer)) {
        auto location = *where;
        return HouseClass::CurrentPlayer->GetPlanningWaypointAt(&location) || unknown_bool_43A ? Action::PatrolWaypoint
                                                                                               : Action::GuardArea;
    }
    if (game::debug_can_control_house(Owner))
        if (auto* equipped = GetTurretWeapon(); equipped && equipped->WeaponType) {
            const int weapon = SelectWeapon(map.GetCellAt(*where));
            bool cliff = map.GetCellAt(*where)->Tile_Is_DestroyableCliff();
            if (cliff && GetTurretWeapon()->WeaponType->Warhead->Fire) cliff = false;
            if (fire || (overlay && overlay->LegalTarget) || cliff || remembered_target) {
                auto* warhead = GetTurretWeapon()->WeaponType->Warhead;
                const bool enemy =
                    cell->WallOwnerIndex != -1 && !HouseClass::Array[cell->WallOwnerIndex]->IsAlliedWith(Owner);
                auto* monster = VeinholeMonsterClass::GetVeinholeMonsterFrom(const_cast<CellStruct*>(where));
                const bool live_monster = monster && !monster->IsDead && monster->Health > 0;
                if (fire || !overlay || live_monster ||
                    (enemy && overlay->Wall && (warhead->Wall || (warhead->Wood && overlay->Armor == Armor::Wood)))) {
                    if (IsControllable() ||
                        IsCloseEnough3D({int(where->X) * 256 + 128, int(where->Y) * 256 + 128, 0}, weapon))
                        return Action::Attack;
                }
            }
            if (CanDisguiseAs(cell) &&
                (IsControllable() ||
                 IsCloseEnough3D({int(where->X) * 256 + 128, int(where->Y) * 256 + 128, 0}, weapon)))
                return Action::Attack;
        }
    if (!game::debug_can_control_house(Owner) || (!IsControllable() && !IsUnitFactory())) return Action::None;
    if (!map.IsWithinUsableArea(*where, true)) return Action::NoMove;
    if (select || (IsUnitFactory() && move)) return Action::Move;
    if (!IsControllable()) return Action::NoMove;
    if (checkFog || static_cast<int>(IsCellOccupied(map.GetCellAt(*where), FacingType::None, -1, nullptr, true)) <= 1)
        return Action::Move;
    if (WhatAmI() == AbstractType::Building && GetTechnoType()->UndeploysInto &&
        GetTechnoType()->UndeploysInto->ResourceGatherer)
        return Action::Move;
    if (GetTechnoType()->IsSubterranean &&
        static_cast<int>(IsCellOccupied(map.GetCellAt(*where), FacingType::None, -1, nullptr, false)) <= 1)
        return Action::Move;
    return Action::NoMove;
}

// Original Techno override; Object's default is deliberately false.
bool TechnoClass::IsSelectable() const {
    return Health > 0 && !InLimbo && IsInPlayfield && WhatAmI() != AbstractType::Building &&
           DiscoveredByCurrentPlayer && GetTechnoType()->Selectable && Owner->IsControlledByCurrentPlayer();
}
