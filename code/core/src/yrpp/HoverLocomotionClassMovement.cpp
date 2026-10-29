// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 hover.cpp, calibrated to YR 0x513D20 / 0x514310..0x516BE1.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HoverLocomotionClass.h"
#include "path_requests.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/VocClass.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#include <cstring>

namespace {
int wrap(unsigned value) { return std::bit_cast<int>(value); }
int delta(int a, int b) { return wrap(unsigned(a) - unsigned(b)); }
int distance(const CoordStruct& a, const CoordStruct& b) {
    const double x = delta(a.X, b.X), y = delta(a.Y, b.Y), z = delta(a.Z, b.Z);
    return game::x87_integer(Math::sqrt(x * x + z * z + y * y));
}
DirStruct direction(const CoordStruct& from, const CoordStruct& to) {
    const auto angle = Math::atan2(double(from.Y) - to.Y, double(to.X) - from.X);
    return DirStruct(game::x87_integer((angle - 1.5707963267948966) * -10430.06004058427));
}
unsigned turn_difference(DirStruct a, DirStruct b) {
    const short diff = std::bit_cast<short>(WORD(a.Raw - b.Raw));
    return WORD(diff < 0 ? -int(diff) : int(diff));
}
CoordStruct adjacent(CoordStruct at, int facing) {
    const auto d = Unsorted::AdjacentCoord[facing & 7];
    at.X = wrap(unsigned(at.X) + unsigned(d.X));
    at.Y = wrap(unsigned(at.Y) + unsigned(d.Y));
    return at;
}
void advance(FootClass& foot) {
    std::memmove(foot.PathDirections, foot.PathDirections + 1, 23 * sizeof(int));
    foot.PathDirections[23] = -1;
}
bool active(const FootClass* foot) { return foot->IsAlive && !foot->InLimbo && !foot->IsFallingDown; }
bool upper(const FootClass& foot, const CellClass& cell) {
    return cell.ContainsBridge() &&
           std::abs(foot.Location.Z / Unsorted::LevelHeight - static_cast<signed char>(cell.Level)) > 2;
}
}

void HoverLocomotionClass::Gravity_AI() {
    const int height = LinkedTo->GetHeight();
    int clearance = height;
    auto& map = MapClass::Instance;
    const auto& rules = *RulesClass::Instance;
    if (LinkedTo->PathDirections[0] != -1) {
        const int floor = map.GetCellFloorHeight(LinkedTo->Location);
        if (map.GetCellFloorHeight(adjacent(LinkedTo->Location, LinkedTo->PathDirections[0])) > floor)
            clearance = delta(height, rules.HoverHeight);
    }
    int next = game::x87_integer(double(height) + CurrentWobbles);
    const int id = LinkedTo->Fetch_ID();
    const double period = ((id & 1) ? 1.0 : 1.1) * rules.HoverBob * 900.0;
    const int frames = game::x87_integer(period);
    // Invalid zero-period rules must not cause a native integer divide fault.
    if (frames) {
        const int phase = wrap(unsigned(Unsorted::CurrentFrame) + 2u * unsigned(id)) % frames;
        const double bob = Math::sin(double(phase) * 6.283185307179586 / period);
        next = game::x87_integer(bob + bob + double(next));
    }
    if (next < 0) {
        CurrentWobbles = 0;
        next = 0;
    }
    const bool down = LinkedTo->IsOnMap;
    LinkedTo->IsOnMap = false;
    LinkedTo->SetHeight(next);
    LinkedTo->IsOnMap = down;
    if (clearance < rules.HoverHeight) {
        if (LocomotionClass::Is_Powered())
            CurrentWobbles += (double(rules.HoverHeight) + double(rules.HoverHeight) - double(clearance)) /
                              rules.HoverHeight * rules.Gravity;
        if (clearance < rules.HoverHeight / 4) CurrentWobbles += rules.Gravity / 3;
    }
    CurrentWobbles -= rules.Gravity;
    CurrentWobbles *= rules.HoverDampen;
}

void HoverLocomotionClass::Motion_AI() {
    if (Is_Moving() && HeadToCoord != CoordStruct::Empty) {
        const auto dir = direction(LinkedTo->GetCoords(), HeadToCoord);
        if (unknown_bool_70)
            LocomotionFacing.SetCurrent(dir);
        else
            LocomotionFacing.SetDesired(dir);
        if ((Is_Powered() && turn_difference(LocomotionFacing.Current(), dir) <= 0x2000u) || unknown_bool_70) {
            if ((DestinationCoord == CoordStruct::Empty && distance(LinkedTo->GetCoords(), HeadToCoord) < 256) ||
                (DestinationCoord != CoordStruct::Empty && distance(LinkedTo->GetCoords(), DestinationCoord) < 256))
                MaxSpeed = 0.5;
            else {
                MaxSpeed = 1.0;
                if (unknown_bool_70) CurrentSpeed = 1.0;
            }
        } else
            MaxSpeed = 0;
        if (MaxSpeed > 0) {
            BoostSpeed = 1.0;
            if (LinkedTo->PathDirections[0] != -1 && LinkedTo->PathDirections[0] == LinkedTo->PathDirections[1])
                BoostSpeed = RulesClass::Instance->HoverBoost;
        }
        const double target = std::min(BoostSpeed * MaxSpeed, 1.0);
        if (target > CurrentSpeed)
            CurrentSpeed = std::min(CurrentSpeed + 1.0 / (RulesClass::Instance->HoverAcceleration * 900.0), target);
        if (target < CurrentSpeed)
            CurrentSpeed = std::max(CurrentSpeed - 1.0 / (RulesClass::Instance->HoverBrake * 900.0), 0.0);
        if (MaxSpeed > 0 && !unknown_bool_70) {
            auto at = HeadToCoord;
            if (LinkedTo->PathDirections[0] != -1 && LinkedTo->PathDirections[0] != 8)
                at = adjacent(at, LinkedTo->PathDirections[0]);
            LinkedTo->PrimaryFacing.SetDesired(direction(LinkedTo->GetCoords(), at));
        }
    } else if (Is_Moving() && HeadToCoord == CoordStruct::Empty) {
        LinkedTo->SetSpeedPercentage(1.0);
        Start_Of_Move(0);
        While_Moving(true);
    }
}

bool YRPP_STDCALL HoverLocomotionClass::Process() {
    if (Is_Moving() && Is_Moving()) {
        Motion_AI();
        if (!LinkedTo->IsAlive) return false;
        const int z = LinkedTo->GetZ();
        int speed = game::x87_integer(double(LinkedTo->GetCurrentSpeed()) * CurrentSpeed);
        const auto dir = direction(LinkedTo->GetCoords(), HeadToCoord);
        if (!speed && !turn_difference(LocomotionFacing.Current(), dir)) {
            Start();
            speed = game::x87_integer(double(LinkedTo->GetCurrentSpeed()) * CurrentSpeed);
        }
        const double x = delta(LinkedTo->Location.X, HeadToCoord.X), y = delta(LinkedTo->Location.Y, HeadToCoord.Y);
        if (speed >= game::x87_integer(Math::sqrt(x * x + y * y))) {
            LinkedTo->FrozenStill = true;
            LinkedTo->IsWaitingBlockagePath = false;
            unknown_bool_70 = false;
            Start_Of_Move(0);
            if (LinkedTo->PathDirections[0] == -1 && DestinationCoord == CoordStruct::Empty)
                HeadToCoord = CoordStruct::Empty;
            auto* contact = LinkedTo->GetNthLink();
            const auto docking = contact ? CellClass::Coord2Cell(contact->GetDestination(LinkedTo)) : CellStruct::Empty;
            const bool atDock = LinkedTo->GetMapCoords() == docking;
            if (DestinationCoord != CoordStruct::Empty || atDock) {
                const auto result = While_Moving(true);
                if (!active(LinkedTo)) return true;
                if (result == Move::OK && HeadToCoord == CoordStruct::Empty) {
                    LinkedTo->MarkAllOccupationBits(LinkedTo->Location);
                    LinkedTo->UpdatePosition(PCPType::End);
                    if (!active(LinkedTo)) return false;
                }
                if (result == Move::No) {
                    if (LinkedTo->TubeIndex >= 0) return false;
                    Stop();
                } else if (result != Move::OK) {
                    Stop_Driver();
                    speed = 0;
                }
            }
        }
        if (speed > 0) {
            auto at = LinkedTo->Location;
            if (LinkedTo->IsStandingStill() && HeadToCoord != CoordStruct::Empty) {
                LinkedTo->UnmarkAllOccupationBits(at);
                LinkedTo->FrozenStill = false;
                LinkedTo->IsWaitingBlockagePath = false;
            }
            const double angle =
                (int(std::bit_cast<short>(LocomotionFacing.Current().Raw)) - 0x3FFF) * -0.00009587672516830327;
            at.Y = game::x87_integer(double(at.Y) - Math::sin(angle) * speed);
            at.X = game::x87_integer(Math::cos(angle) * speed + double(at.X));
            if (CellClass::Coord2Cell(at) != CellClass::Coord2Cell(LinkedTo->Location)) {
                LinkedTo->Mark(MarkType::Up);
                LinkedTo->SetLocation(at);
                LinkedTo->SetZ(z);
                auto* cell = MapClass::Instance.GetCellAt(at);
                if (!LinkedTo->OnBridge && cell->ContainsBridge() && LinkedTo->GetHeight() >= CellClass::BridgeHeight)
                    LinkedTo->OnBridge = true;
                if (LinkedTo->OnBridge && !cell->ContainsBridge()) LinkedTo->OnBridge = false;
                LinkedTo->Mark(MarkType::Down);
            } else {
                const bool down = LinkedTo->IsOnMap;
                LinkedTo->IsOnMap = false;
                LinkedTo->SetLocation(at);
                LinkedTo->SetZ(z);
                LinkedTo->IsOnMap = down;
            }
        }
    }
    if (Is_Moving_Now() && Unsorted::CurrentFrame % 10 == 0 && !LinkedTo->OnBridge &&
        LinkedTo->GetCell()->LandType == LandType::Water && RulesClass::Instance->Wake)
        if (void* storage = YRMemory::Allocate(sizeof(AnimClass)))
            ::new (storage) AnimClass(RulesClass::Instance->Wake, LinkedTo->Location);
    Gravity_AI();
    if (unknown_bool_68) {
        if (Is_Powered()) {
            LinkedTo->PrimaryFacing.SetCurrent(
                DirStruct(WORD(LinkedTo->PrimaryFacing.Current().Raw + (unsigned(unknown_int_6C) << 8))));
            const unsigned magnitude = unknown_int_6C < 0 ? 0u - unsigned(unknown_int_6C) : unsigned(unknown_int_6C);
            const int step = std::min(magnitude, 1u);
            unknown_int_6C += unknown_int_6C < 0 ? step : -step;
            if (!unknown_int_6C) unknown_bool_68 = false;
        } else
            unknown_bool_68 = false;
        if (!unknown_bool_68) {
            MapClass::Instance.GetCellAt(LinkedTo->Location)->ActivateVeins();
            if (MapClass::Instance.GetCellAt(LinkedTo->Location)->LandType == LandType::Water &&
                LinkedTo->Location.Z <
                    MapClass::Instance.GetCellFloorHeight(LinkedTo->Location) + Unsorted::LevelHeight)
                LinkedTo->DropAsBomb();
        }
    }
    return Is_Moving();
}

bool HoverLocomotionClass::Reduce_Path_Length() {
    if (!Is_Moving() || LinkedTo->PathDirections[0] == -1) return false;
    auto* target = LinkedTo->Destination;
    if (target && (target->WhatAmI() == AbstractType::Unit || target->WhatAmI() == AbstractType::Infantry)) {
        const int cells = distance(LinkedTo->GetCoords(), DestinationCoord) / 256;
        if (cells < 24) {
            LinkedTo->PathDirections[cells] = -1;
            return true;
        }
    }
    if (LinkedTo->Target && LinkedTo->IsCloseEnoughToAttack(LinkedTo->Target)) {
        LinkedTo->PathDirections[0] = -1;
        return true;
    }
    return false;
}
void HoverLocomotionClass::Start_Of_Move(int mode) {
    Reduce_Path_Length();
    const int facing = LinkedTo->PathDirections[0];
    if (DestinationCoord != CoordStruct::Empty && LinkedTo->GetMapCoords() == CellClass::Coord2Cell(DestinationCoord) &&
        std::abs(delta(LinkedTo->GetDestination().Z, DestinationCoord.Z)) < 2 * Unsorted::LevelHeight) {
        Stop_Moving();
        LinkedTo->SetDestination(nullptr, true);
        return;
    }
    if (facing != -1 || DestinationCoord == CoordStruct::Empty) return;
    if (LinkedTo->GetMapCoords() == CellClass::Coord2Cell(DestinationCoord) &&
        std::abs(delta(LinkedTo->GetDestination().Z, DestinationCoord.Z)) <= 2 * Unsorted::LevelHeight)
        return;
    if (LinkedTo->PathDelayTimer.GetTimeLeft()) return;
    const bool found = LinkedTo->UpdatePathfinding(CellClass::Coord2Cell(DestinationCoord), 0, mode);
    if (game::path_request_pending(*LinkedTo)) return;
    LinkedTo->PathDelayTimer.Start(game::x87_integer(RulesClass::Instance->PathDelay * 900.0));
    if (!found) {
        if (!LinkedTo->IsInSameZoneAsCoords(DestinationCoord)) {
            LinkedTo->SetDestination(nullptr, true);
            return;
        }
        if (LinkedTo->GetCurrentMission() != Mission::Enter &&
            distance(LinkedTo->GetCoords(), DestinationCoord) < RulesClass::Instance->CloseEnough &&
            (LinkedTo->GetCurrentMission() == Mission::Move || LinkedTo->GetCurrentMission() == Mission::Area_Guard)) {
            LinkedTo->SetDestination(nullptr, true);
            if (!LinkedTo->IsAlive) return;
        } else if (LinkedTo->PathWaitTimes > 0)
            --LinkedTo->PathWaitTimes;
        else {
            LinkedTo->SetDestination(nullptr, true);
            if (!LinkedTo->IsAlive) return;
            if (LinkedTo->ShouldScanForTarget)
                VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000, 1.0f, nullptr);
            LinkedTo->ShouldScanForTarget = false;
        }
        if (!Is_Moving() && LinkedTo->Target && !LinkedTo->IsCloseEnoughToAttack(LinkedTo->Target)) {
            LinkedTo->IsScanLimited = true;
            if (LinkedTo->Team) LinkedTo->Team->ScanLimit();
            LinkedTo->SetTarget(nullptr);
        }
        if (HeadToCoord != CoordStruct::Empty) {
            LinkedTo->UnmarkAllOccupationBits(HeadToCoord);
            HeadToCoord = CoordStruct::Empty;
        }
        Stop_Moving();
        return;
    }
    auto cell = CellClass::Coord2Cell(LinkedTo->GetCoords());
    const auto d = Unsorted::AdjacentCell[LinkedTo->PathDirections[0] & 7];
    cell.X = short(cell.X + d.X);
    cell.Y = short(cell.Y + d.Y);
    auto& map = MapClass::Instance;
    if (map.IsWithinUsableArea(cell, true)) {
        auto* next = map.GetCellAt(cell);
        if (LinkedTo->IsCellOccupied(next, FacingType(LinkedTo->PathDirections[0]), LinkedTo->GetCellLevel(), nullptr,
                                     true) == Move::Temp) {
            const CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
            auto* object = next->FindTechnoNearestTo(
                {0, 0}, LinkedTo->Location.Z > map.GetCellFloorHeight(at) + 3 * Unsorted::LevelHeight, nullptr);
            if (object && LinkedTo->Owner->IsAlliedWith(object)) {
                if (distance(LinkedTo->GetCoords(), DestinationCoord) < RulesClass::Instance->CloseEnough &&
                    !LinkedTo->HasAnyLink() &&
                    std::abs(delta(DestinationCoord.Z, LinkedTo->Location.Z)) < 2 * Unsorted::LevelHeight &&
                    map.GetCellAt(LinkedTo->Location)->LandType != LandType::Tunnel) {
                    Stop_Moving();
                    LinkedTo->SetDestination(nullptr, true);
                    return;
                }
                next->ScatterContent(CoordStruct::Empty, true, true, upper(*LinkedTo, *next));
            }
        }
    }
    LinkedTo->PathWaitTimes = 10;
}

Move HoverLocomotionClass::While_Moving(bool firstPass) {
    if (HeadToCoord != CoordStruct::Empty) {
        LinkedTo->UnmarkAllOccupationBits(HeadToCoord);
        HeadToCoord = CoordStruct::Empty;
    }
    if (LinkedTo->IsParalyzed()) return Move::No;
    if (!Is_Moving() || DestinationCoord == CoordStruct::Empty) return Move::OK;
    const int facing = LinkedTo->PathDirections[0];
    auto& map = MapClass::Instance;
    if (facing == 8) {
        const int index = map.GetCellAt(LinkedTo->Location)->TubeIndex;
        if (index < 0 || index >= TubeClass::Array.Count) {
            LinkedTo->PathDirections[0] = -1;
            HeadToCoord = CoordStruct::Empty;
            Stop();
            LinkedTo->SetDestination(nullptr, true);
        } else {
            LinkedTo->Mark(MarkType::Up);
            LinkedTo->UnmarkAllOccupationBits(LinkedTo->Location);
            auto* tube = TubeClass::Array[index];
            HeadToCoord = {int(tube->ExitCell.X) * 256 + 128, int(tube->ExitCell.Y) * 256 + 128, 0};
            advance(*LinkedTo);
            LinkedTo->CurrentMapCoords = CellClass::Coord2Cell(HeadToCoord);
            LinkedTo->TubeIndex = static_cast<signed char>(index);
            LinkedTo->TubeFaceIndex = 0;
            auto cell = tube->EnterCell;
            const auto d = Unsorted::AdjacentCell[tube->Faces[0] & 7];
            cell.X = short(cell.X + d.X);
            cell.Y = short(cell.Y + d.Y);
            const auto at = map.GetCellAt(cell)->GetCellCoords();
            LinkedTo->CurrentTunnelCoords = {at.X - (int(tube->EnterCell.X) * 256 + 128) + LinkedTo->Location.X,
                                             at.Y - (int(tube->EnterCell.Y) * 256 + 128) + LinkedTo->Location.Y,
                                             at.Z + LinkedTo->Location.Z};
            LinkedTo->CurrentTunnelCoords.Z =
                LinkedTo->GetZ() +
                (map.GetCellFloorHeight(HeadToCoord) - map.GetCellFloorHeight(LinkedTo->Location)) / tube->FaceCount;
        }
        return Move::No;
    }
    if (facing == -1) {
        Start_Of_Move(0);
        return Move::OK;
    }
    auto cell = LinkedTo->GetMapCoords();
    const auto d = Unsorted::AdjacentCell[facing & 7];
    cell.X = short(cell.X + d.X);
    cell.Y = short(cell.Y + d.Y);
    HeadToCoord = {int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
    HeadToCoord.Z = map.GetCellFloorHeight(HeadToCoord);
    if (LinkedTo->Location.Z >= HeadToCoord.Z + 3 * Unsorted::LevelHeight) HeadToCoord.Z += CellClass::BridgeHeight;
    if (!map.GetCellAt(HeadToCoord)->CollectCrate(LinkedTo) && !LinkedTo->InLimbo) {
        if (LinkedTo->IsAlive && !LinkedTo->IsFallingDown) {
            LinkedTo->PathDirections[0] = -1;
            HeadToCoord = CoordStruct::Empty;
            Stop();
        }
        return Move::No;
    }
    if (!active(LinkedTo)) return Move::No;
    if (LinkedTo->OnBridge != map.GetCellAt(adjacent(LinkedTo->Location, facing))->ContainsBridge())
        LinkedTo->unknown_bool_68B = true;
    cell = CellClass::Coord2Cell(HeadToCoord);
    auto* next = map.GetCellAt(cell);
    auto result = LinkedTo->IsCellOccupied(next, FacingType(facing), LinkedTo->GetCellLevel(), nullptr, true);
    const auto retry = [&] {
        Stop_Driver();
        LinkedTo->PathDelayTimer.Start(0);
        LinkedTo->PathDirections[0] = -1;
        Start_Of_Move(0);
        return While_Moving(false);
    };
    switch (result) {
        case Move::OK:
            LinkedTo->UpdatePosition(PCPType::End);
            if (!active(LinkedTo)) return Move::No;
            if (LinkedTo->IsInPlayfield) LinkedTo->MarkAllOccupationBits(HeadToCoord);
            advance(*LinkedTo);
            LinkedTo->CurrentMapCoords = CellClass::Coord2Cell(HeadToCoord);
            return result;
        case Move::Cloak:
        case Move::No:
            map.GetCellAt(HeadToCoord)->RevealCellObjects();
            if (firstPass) result = retry();
            if (result != Move::OK) {
                HeadToCoord = CoordStruct::Empty;
                Stop_Driver();
            }
            return result;
        case Move::ClosedGate:
            HeadToCoord = CoordStruct::Empty;
            map.MakeTraversable(LinkedTo, cell);
            Stop_Driver();
            return result;
        case Move::Temp:
            if (firstPass) {
                HeadToCoord = CoordStruct::Empty;
                return retry();
            }
            if (distance(LinkedTo->GetCoords(), DestinationCoord) < RulesClass::Instance->CloseEnough &&
                std::abs(delta(DestinationCoord.Z, LinkedTo->Location.Z)) < 2 * Unsorted::LevelHeight &&
                map.GetCellAt(LinkedTo->Location)->LandType != LandType::Tunnel) {
                Stop_Moving();
                LinkedTo->SetDestination(nullptr, true);
                return Move::No;
            }
            map.GetCellAt(HeadToCoord)
                ->ScatterContent(CoordStruct::Empty, true, true, upper(*LinkedTo, *map.GetCellAt(HeadToCoord)));
            HeadToCoord = CoordStruct::Empty;
            Stop_Driver();
            return result;
        case Move::FriendlyDestroyable:
        case Move::Destroyable:
            HeadToCoord = CoordStruct::Empty;
            if (firstPass) return retry();
            if (auto* object = next->GetSomeObject({0, 0}, false)) {
                if (!LinkedTo->Owner->IsAlliedWith(object))
                    LinkedTo->Override_Mission(Mission::Attack, object, nullptr);
            } else if (next->OverlayTypeIndex != -1 && OverlayTypeClass::Array[next->OverlayTypeIndex]->Wall)
                LinkedTo->Override_Mission(Mission::Attack, next, nullptr);
            return result;
        case Move::MovingBlock:
            HeadToCoord = CoordStruct::Empty;
            Stop_Driver();
            if (game::wait_for_moving_blocker(*LinkedTo)) return result;
            if (!LinkedTo->IsWaitingBlockagePath) {
                LinkedTo->IsWaitingBlockagePath = true;
                LinkedTo->BlockagePathTimer.Start(RulesClass::Instance->BlockagePathDelay);
            }
            if (!LinkedTo->PathDelayTimer.GetTimeLeft()) LinkedTo->PathDirections[0] = -1;
            Start_Of_Move(1 + int(LinkedTo->IsWaitingBlockagePath && !LinkedTo->BlockagePathTimer.GetTimeLeft()));
            return result;
        default:
            return result;
    }
}
