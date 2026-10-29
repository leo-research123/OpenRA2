// SPDX-License-Identifier: GPL-3.0-or-later
// BuildingClass tank bunker handoffs, calibrated to YR 0x458E50..0x45983C.
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdlib>

namespace {
void bunker_animation(BuildingClass& building, BuildingAnimSlot slot) {
    const bool damaged = building.GetHealthPercentage() <= RulesClass::Instance->ConditionYellow;
    const auto& animation = building.Type->GetBuildingAnim(slot);
    if ((damaged ? animation.Damaged : animation.Anim)[0]) building.PlayNthAnim(slot, damaged, false);
}
void bunker_sound(BuildingClass& building, int sound) {
    if (sound != -1) VocClass::PlayAt(sound, building.Location, nullptr);
}
void lower_bunker_walls(BuildingClass& building) {
    bunker_sound(building, RulesClass::Instance->BunkerWallsDownSound);
    bunker_animation(building, BuildingAnimSlot::SpecialThree);
    bunker_animation(building, BuildingAnimSlot::SpecialFour);
}
void bunker_exit_track(BuildingClass& building, UnitClass& tank) {
    if (!tank.Locomotor) std::abort();
    tank.Locomotor->Power_On();
    auto at = building.GetCoords();
    at.X = std::bit_cast<int>(unsigned(at.X) - 128u);
    at.Y = std::bit_cast<int>(unsigned(at.Y) + 128u);
    if (!tank.Locomotor) std::abort();
    tank.Locomotor->Force_Track(71, at);
    tank.SetSpeedPercentage(1.0);
}
}

bool BuildingClass::UpdateBunker() {
    auto* client = BunkerLinkedItem ? BunkerLinkedItem : GetNthLink();
    if (!client || client->WhatAmI() != AbstractType::Unit) {
        TankBunkerState = TankBunkerState::Idle;
        QueueMission(Mission::Guard, false);
        return false;
    }
    auto* tank = static_cast<UnitClass*>(client);
    switch (TankBunkerState) {
        case TankBunkerState::Idle: {
            if (tank->GetCell()->GetBuilding() != this) break;
            if (!tank->Locomotor) std::abort();
            if (tank->Locomotor->Is_Moving()) break;
            const auto* foundation = GetFoundationData(false);
            const auto origin = GetMapCoords();
            for (; *foundation != CellStruct{0x7FFF, 0x7FFF}; ++foundation) {
                const CellStruct at{short(origin.X + foundation->X), short(origin.Y + foundation->Y)};
                auto* other = MapClass::Instance.GetCellAt(at)->FindTechnoNearestTo(Point2D{128, 128}, false, this);
                if (other && other != tank) other->Scatter(GetCoords(), true, false);
            }
            TankBunkerState = TankBunkerState::CheckOccupants;
            break;
        }
        case TankBunkerState::CheckOccupants: {
            bool blocked = false;
            const auto* foundation = GetFoundationData(false);
            const auto origin = GetMapCoords();
            for (; *foundation != CellStruct{0x7FFF, 0x7FFF}; ++foundation) {
                const CellStruct at{short(origin.X + foundation->X), short(origin.Y + foundation->Y)};
                auto* other = MapClass::Instance.GetCellAt(at)->FindTechnoNearestTo(Point2D{128, 128}, false, this);
                if (other && other != tank) {
                    blocked = true;
                    if ((other->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None &&
                        !static_cast<FootClass*>(other)->Destination)
                        return false;
                }
            }
            if (blocked) break;
            const auto from = tank->GetCoords();
            const auto to = GetCoords();
            const double angle = Math::atan2(double(to.Y) - double(from.Y), double(from.X) - double(to.X));
            const auto facing = game::x87_integer((angle - Math::HalfPi) * -10430.06004058427);
            const unsigned packed = (unsigned(from.X) & 0xFFFF0000u) | ((unsigned(facing) + 0x7FFFu) & 0xFFFFu);
            tank->PrimaryFacing.SetDesired(std::bit_cast<DirStruct>(packed));
            TankBunkerState = TankBunkerState::RotateToTrack;
            break;
        }
        case TankBunkerState::RotateToTrack:
            if (!tank->PrimaryFacing.IsRotating()) {
                const unsigned facing = ((unsigned(tank->PrimaryFacing.Current().Raw) >> 7) + 1u) & 0x1FEu;
                const int track = facing == 64 ? 67 : facing == 448 ? 70 : facing == 192 ? 68 : 69;
                const auto at = GetCoords();
                if (!tank->Locomotor) std::abort();
                tank->Locomotor->Force_Track(track, at);
                tank->SetSpeedPercentage(1.0);
                TankBunkerState = TankBunkerState::MoveOnTrack;
            }
            break;
        case TankBunkerState::MoveOnTrack:
            if (tank->GetCell()->GetBuilding() == this) {
                if (!tank->Locomotor) std::abort();
                if (!tank->Locomotor->Is_Moving()) {
                    tank->PrimaryFacing.SetDesired(DirStruct(0x8000));
                    TankBunkerState = TankBunkerState::RotateInBunker;
                }
            }
            break;
        case TankBunkerState::RotateInBunker:
            if (!tank->PrimaryFacing.IsRotating()) {
                bunker_animation(*this, BuildingAnimSlot::Special);
                bunker_animation(*this, BuildingAnimSlot::SpecialTwo);
                TankBunkerState = TankBunkerState::RaiseWalls;
            }
            break;
        case TankBunkerState::RaiseWalls:
            BunkerLinkedItem = tank;
            tank->BunkerLinkedItem = this;
            tank->Group = -1;
            tank->Deselect();
            TankBunkerState = TankBunkerState::Bunkered;
            BunkerLinkedItem->QueueMission(Mission::Guard, true);
            bunker_sound(*this, RulesClass::Instance->BunkerWallsUpSound);
            break;
        default:
            break;
    }
    // The original caller ignores AL; the public declaration is retained.
    return false;
}

void BuildingClass::UnloadBunker() {
    auto* client = BunkerLinkedItem;
    if (!client || client->WhatAmI() != AbstractType::Unit) return;
    auto* tank = static_cast<UnitClass*>(client);
    bunker_exit_track(*this, *tank);
    tank->BunkerLinkedItem = nullptr;
    BunkerLinkedItem = nullptr;
    SendToFirstLink(RadioCommand::NotifyUnlink);
}

void BuildingClass::ClearBunker() {
    DestroyNthAnim(BuildingAnimSlot::Special);
    DestroyNthAnim(BuildingAnimSlot::SpecialTwo);
    if (!BunkerLinkedItem) return;
    lower_bunker_walls(*this);
    SendToFirstLink(RadioCommand::NotifyUnlink);
    BunkerLinkedItem->BunkerLinkedItem = nullptr;
    BunkerLinkedItem = nullptr;
    TankBunkerState = TankBunkerState::Idle;
    QueueMission(Mission::Guard, false);
}

void BuildingClass::EmptyBunker() {
    DestroyNthAnim(BuildingAnimSlot::Special);
    DestroyNthAnim(BuildingAnimSlot::SpecialTwo);
    lower_bunker_walls(*this);
    auto* client = BunkerLinkedItem;
    if (!client) {
        TankBunkerState = TankBunkerState::Idle;
        QueueMission(Mission::Guard, false);
        return;
    }
    if (client->WhatAmI() != AbstractType::Unit) return;
    auto* tank = static_cast<UnitClass*>(client);
    tank->BunkerLinkedItem = nullptr;
    bunker_exit_track(*this, *tank);
    const auto origin = GetMapCoords();
    CellStruct exit;
    MapClass::Instance.NearByLocation(exit, CellStruct{short(origin.X - 1), short(origin.Y + 1)},
                                      tank->GetTechnoType()->SpeedType, -1, MovementZone::Normal, false, 1, 1, false,
                                      false, false, true, CellStruct{0, 0}, false, false);
    tank->SetDestination(MapClass::Instance.GetCellAt(exit), true);
    tank->QueueMission(Mission::Move, false);
    BunkerLinkedItem = nullptr;
    TankBunkerState = TankBunkerState::Idle;
    QueueMission(Mission::Guard, false);
    SendToFirstLink(RadioCommand::NotifyUnlink);
}
