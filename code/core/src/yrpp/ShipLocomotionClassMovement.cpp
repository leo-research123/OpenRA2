// SPDX-License-Identifier: GPL-3.0-or-later
// Movement scaffolding follows OpenTS 44fac744 drive.cpp.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Ship movement, reconstructed from YR 0x6A05F0.
#include "yrpp/ShipLocomotionClass.h"
#include "path_requests.hpp"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/TubeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/AnimClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdint>
#include <cstring>

namespace {
int add(int a, int b) { return std::bit_cast<std::int32_t>(std::uint32_t(a) + std::uint32_t(b)); }
int subtract(int a, int b) { return std::bit_cast<std::int32_t>(std::uint32_t(a) - std::uint32_t(b)); }
int absolute(int a) { return a < 0 ? subtract(0, a) : a; }
CellStruct cell_of(const CoordStruct& p) { return {short(p.X / 256), short(p.Y / 256)}; }
bool bridge(const CellClass* cell) { return (unsigned(cell->Flags) & 0x100u) != 0; }
void update_bridge(FootClass* foot, const CellClass* old, const CellClass* next) {
    if (static_cast<signed char>(next->Level) == static_cast<signed char>(old->Level) - 4 && bridge(next))
        foot->OnBridge = true;
    if (!bridge(next) && bridge(old)) foot->OnBridge = false;
}
void advance_path(FootClass* foot) {
    std::memmove(foot->PathDirections, foot->PathDirections + 1, 23 * sizeof(int));
    foot->PathDirections[23] = -1;
}
bool active(const FootClass* foot) { return foot->IsAlive && !foot->InLimbo && !foot->IsFallingDown; }
int distance(const CoordStruct& a, const CoordStruct& b) {
    const double x = subtract(a.X, b.X), y = subtract(a.Y, b.Y), z = subtract(a.Z, b.Z);
    return game::x87_integer(Math::sqrt(z * z + y * y + x * x));
}
}

bool YRPP_STDCALL ShipLocomotionClass::Process() {
    Set_Slope(LinkedTo->GetCell()->SlopeIndex);
    if (TrackNumber != -1 && IsDriving) {
        if (While_Moving() || !LinkedTo->IsAlive) return false;
        if (TrackNumber == -1 && (Is_Moving() || LinkedTo->PathDirections[0] != -1) &&
            (LinkedTo->WhatAmI() != AbstractType::Unit || !static_cast<UnitClass*>(LinkedTo)->Unloading)) {
            bool stop = false;
            Start_Of_Move(stop);
            if (stop || !LinkedTo->IsAlive) return false;
            While_Moving(true);
            if (!LinkedTo->IsAlive) return false;
        }
    } else {
        if (LinkedTo->Destination && LinkedTo->Destination->WhatAmI() == AbstractType::Cell &&
            LinkedTo->GetMapCoords() == static_cast<CellClass*>(LinkedTo->Destination)->MapCoords) {
            Abandon_Navigation();
            return false;
        }
        if (LinkedTo->CurrentMission == Mission::Guard && !IsDriving && DestinationCoord != CoordStruct::Empty &&
            LinkedTo->Location == DestinationCoord) {
            Abandon_Navigation();
            return false;
        }
        if (LinkedTo->PrimaryFacing.IsRotating())
            IsRotating = true;
        else {
            if (IsRotating) {
                IsRotating = false;
                LinkedTo->UpdatePosition(PCPType::Rotation);
                if (!active(LinkedTo)) return false;
            }
            if ((LinkedTo->GetCurrentMission() != Mission::Guard || Is_Moving()) &&
                LinkedTo->GetCurrentMission() != Mission::Unload) {
                if (!Is_Moving() && LinkedTo->PathDirections[0] == -1) {
                    if (LinkedTo->IsSinking) {
                        Stop_Driver();
                        movementspeed_50 = 0;
                    } else if (LinkedTo->Destination) {
                        CoordStruct destination;
                        LinkedTo->Destination->GetDestination(&destination, LinkedTo);
                        Move_To(destination);
                    }
                } else if (LinkedTo->IsInPlayfield && LinkedTo->GetCurrentMission() != Mission::Enter && Is_Moving() &&
                           !LinkedTo->IsInSameZoneAsCoords(DestinationCoord)) {
                    Stop_Driver();
                    if (Abandon_Navigation()) return false;
                } else {
                    bool stop = false;
                    Start_Of_Move(stop);
                    if (stop || !LinkedTo->IsAlive) return false;
                    While_Moving();
                    if (!LinkedTo->IsAlive) return false;
                }
            }
        }
    }
    if (Is_Moving_Now() && Unsorted::CurrentFrame % 8 == 0 && !LinkedTo->GetTechnoType()->Underwater &&
        !LinkedTo->OnBridge && LinkedTo->GetCell()->LandType == LandType::Water && RulesClass::Instance->Wake) {
        // The original permits allocation failure and simply omits this wake.
        if (void* storage = YRMemory::Allocate(sizeof(AnimClass))) {
            const auto location = LinkedTo->Location;
            new (storage) AnimClass(RulesClass::Instance->Wake, location);
        }
    }
    if (DestinationCoord == CoordStruct::Empty && HeadToCoord == CoordStruct::Empty &&
        LinkedTo->PathDirections[0] == -1 && LinkedTo->SpeedPercentage > 0)
        LinkedTo->SetSpeedPercentage(0);
    return Is_Moving();
}

bool ShipLocomotionClass::While_Moving(bool justStarted) {
    if (((!IsDriving || TrackNumber == -1) && LinkedTo->PathDirections[0] != 8) ||
        (IsRotating && !LinkedTo->GetTechnoType()->Turret)) {
        SpeedAccum = 0;
        return false;
    }
    auto& map = MapClass::Instance;
    if (LinkedTo->GetTechnoType()->Accelerates) {
        const bool accelerates =
            LinkedTo->WhatAmI() != AbstractType::Unit || !static_cast<UnitClass*>(LinkedTo)->Type->Passive;
        if (TrackNumber < 64 && accelerates) {
            auto destination = DestinationCoord;
            const int height = bridge(map.GetCellAt(destination)) ? CellClass::BridgeHeight : 0;
            destination.Z = add(height, map.GetCellFloorHeight(destination));
            const int remaining = distance(LinkedTo->Location, destination);
            double speed = LinkedTo->SpeedPercentage;
            const auto* type = LinkedTo->GetTechnoType();
            bool forced = false;
            if (remaining < type->SlowdownDistance) {
                speed -= type->Speed * type->DecelerationFactor;
                if (speed < double(0.3f)) speed = double(0.3f);
                forced = true;
            } else if (LinkedTo->IsSinking) {
                speed -= type->Speed * double(0.0015f);
                if (speed < double(0.1f)) speed = double(0.1f);
                forced = true;
            }
            if (LinkedTo->IsCrushingSomething) {
                if (movementspeed_50 >= 0.2) movementspeed_50 = 0.2;
                LinkedTo->SetSpeedPercentage(movementspeed_50);
            } else if (forced)
                LinkedTo->SetSpeedPercentage(speed);
            else if (LinkedTo->SpeedPercentage < movementspeed_50) {
                speed = type->AccelerationFactor + LinkedTo->SpeedPercentage;
                if (movementspeed_50 < speed) speed = movementspeed_50;
                LinkedTo->SetSpeedPercentage(speed);
            } else if (LinkedTo->SpeedPercentage > movementspeed_50) {
                speed = LinkedTo->SpeedPercentage - type->Speed * type->DecelerationFactor;
                if (movementspeed_50 > speed) speed = movementspeed_50;
                LinkedTo->SetSpeedPercentage(speed);
            }
            if (LinkedTo->WhatAmI() == AbstractType::Unit) {
                auto* follower = static_cast<UnitClass*>(LinkedTo)->FollowerCar;
                while (follower) {
                    follower->SetSpeedPercentage(LinkedTo->SpeedPercentage);
                    follower = follower->FollowerCar;
                    if (follower && follower == follower->FollowerCar) break;
                }
            }
        }
    } else
        LinkedTo->SetSpeedPercentage(movementspeed_50);
    const int currentSpeed = LinkedTo->GetCurrentSpeed();
    int actual = add(SpeedAccum, justStarted ? 0 : currentSpeed);
    const int nextFace = LinkedTo->PathDirections[0];
    if (nextFace == 8 && TrackNumber == -1) {
        LinkedTo->Mark(MarkType::Up);
        Stop_Driver();
        const int index = LinkedTo->GetCell()->TubeIndex;
        if (index >= 0 && index < TubeClass::Array.Count) {
            const auto* tube = TubeClass::Array[index];
            HeadToCoord = {tube->ExitCell.X * 256 + 128, tube->ExitCell.Y * 256 + 128, 0};
            advance_path(LinkedTo);
            LinkedTo->TubeIndex = static_cast<signed char>(index);
            LinkedTo->TubeFaceIndex = 0;
            const auto offset = Unsorted::AdjacentCell[tube->Faces[0] & 7];
            const CellStruct entry{short(tube->EnterCell.X + offset.X), short(tube->EnterCell.Y + offset.Y)};
            LinkedTo->CurrentTunnelCoords = map.GetCellAt(entry)->GetCellCoords();
            const int z = map.GetCellFloorHeight(LinkedTo->Location);
            const CoordStruct exit{tube->ExitCell.X * 256 + 128, tube->ExitCell.Y * 256 + 128, 0};
            LinkedTo->CurrentTunnelCoords.Z = add(z, subtract(map.GetCellFloorHeight(exit), z) / tube->FaceCount);
            IsDriving = true;
            TrackNumber = -1;
        } else {
            LinkedTo->PathDirections[0] = -1;
            TrackNumber = -1;
            HeadToCoord = CoordStruct::Empty;
        }
        return false;
    }
    if (actual > 7) {
        const auto* control = &TurnTrack[TrackNumber];
        auto* raw = &RawTrack[IsOnShortTrack ? control->ShortTrackStructIndex : control->NormalTrackStructIndex];
        if (nextFace < -1 || nextFace > 8) {
            LinkedTo->PathDirections[0] = -1;
            return false;
        }
        bool changing = nextFace != 8 && nextFace != -1 && int(((unsigned(control->Face) + 16) >> 5) & 7) != nextFace;
        while (actual > 7) {
            actual = subtract(actual, 7);
            const auto& point = raw->TrackPoint[TrackIndex];
            if (point.Point == Point2D{0, 0} && TrackIndex) {
                const int gap = add(absolute(subtract(HeadToCoord.Y, LinkedTo->Location.Y)),
                                    absolute(subtract(HeadToCoord.X, LinkedTo->Location.X)));
                const int correction = game::x87_integer((1.0 - double(gap) * 0.09090909090909091) * 7.0);
                LinkedTo->FrozenStill = true;
                actual = add(actual, correction);
                LinkedTo->IsWaitingBlockagePath = false;
                if (cell_of(LinkedTo->Location) == cell_of(HeadToCoord)) {
                    const bool onMap = LinkedTo->IsOnMap;
                    LinkedTo->IsOnMap = false;
                    LinkedTo->SetLocation(HeadToCoord);
                    LinkedTo->SetHeight(0);
                    LinkedTo->IsOnMap = onMap;
                } else {
                    LinkedTo->Mark(MarkType::Up);
                    LinkedTo->SetLocation(HeadToCoord);
                    LinkedTo->SetHeight(0);
                    LinkedTo->Mark(MarkType::Down);
                }
                Stop_Driver();
                TrackNumber = -1;
                TrackIndex = 0;
                bool arrived = false;
                if (LinkedTo->Destination) {
                    CoordStruct target;
                    LinkedTo->Destination->GetDestination(&target, LinkedTo);
                    const auto cell = cell_of(target);
                    if (LinkedTo->GetMapCoords() == cell) {
                        CoordStruct location;
                        LinkedTo->GetDestination(&location, LinkedTo);
                        if (absolute(subtract(location.Z, DestinationCoord.Z)) < 2 * Unsorted::LevelHeight) {
                            arrived = true;
                            DestinationCoord = CoordStruct::Empty;
                            Stop_Driver();
                            IsDriving = false;
                        }
                    }
                }
                LinkedTo->UpdatePosition(PCPType::End);
                if (!active(LinkedTo)) return true;
                if (arrived) {
                    LinkedTo->AbortMotion();
                    LinkedTo->PathDirections[0] = -1;
                    if (LinkedTo->GetCurrentMission() == Mission::Move && LinkedTo->EnterIdleMode(false, true))
                        return true;
                }
                if (LinkedTo->TryEnterIdle()) return true;
                if (!LinkedTo->IsAlive) return false;
                break;
            }
            if (LinkedTo->IsStandingStill()) {
                LinkedTo->UnmarkAllOccupationBits(LinkedTo->Location);
                LinkedTo->FrozenStill = false;
                LinkedTo->IsWaitingBlockagePath = false;
            }
            CellStruct previous;
            if (TrackIndex) {
                const auto& last = raw->TrackPoint[TrackIndex - 1];
                int facing = last.Face;
                const auto p = Smooth_Turn(last.Point, facing);
                previous = {short(p.X / 256), short(p.Y / 256)};
            } else
                previous = LinkedTo->GetMapCoords();
            int facing = point.Face;
            const auto p = Smooth_Turn(point.Point, facing);
            const CoordStruct next{p.X, p.Y, 0};
            const auto nextCell = cell_of(next);
            if (nextCell != cell_of(LinkedTo->Location)) {
                LinkedTo->Mark(MarkType::Up);
                LinkedTo->SetLocation(next);
                const auto* old = map.GetCellAt(previous);
                auto* cell = map.GetCellAt(nextCell);
                update_bridge(LinkedTo, old, cell);
                if (LinkedTo->GetTechnoType()->IsTrain) {
                    auto* unit = static_cast<UnitClass*>(LinkedTo);
                    if (!unit->IsFollowerCar) {
                        const bool upper =
                            unit->OnBridge ||
                            unit->Location.Z >= add(CellClass::BridgeHeight, map.GetCellFloorHeight(unit->Location));
                        auto* objects = map.GetCellAt(nextCell);
                        auto* object = upper ? objects->AltObject : objects->FirstObject;
                        while (object) {
                            auto* following = object->NextObject;
                            if (!object->IsCrushable(LinkedTo)) {
                                int damage = 10000;
                                object->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, true,
                                                      nullptr);
                                damage = 20;
                                unit->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false,
                                                    nullptr);
                            }
                            object = following;
                        }
                    }
                }
                if (!LinkedTo->IsAlive) return false;
                LinkedTo->Mark(MarkType::Down);
                if (cell->OverlayTypeIndex != -1) {
                    const auto* overlay = OverlayTypeClass::Array[cell->OverlayTypeIndex];
                    if (IsRocking && (LinkedTo->GetTechnoType()->Crusher || LinkedTo->HasAbility(Ability::Crusher)) &&
                        !overlay->ArrayIndex) {
                        LinkedTo->IsCrushingSomething = true;
                        if (LinkedTo->GetTechnoType()->TiltsWhenCrushes)
                            LinkedTo->RockingForwardsPerFrame = std::bit_cast<float>(std::uint32_t(0xBCA3D70A));
                    }
                }
            } else {
                const bool onMap = LinkedTo->IsOnMap;
                LinkedTo->IsOnMap = false;
                LinkedTo->SetLocation(next);
                LinkedTo->IsOnMap = onMap;
            }
            if (!LinkedTo->IsAlive) return false;
            const bool onMap = LinkedTo->IsOnMap;
            LinkedTo->IsOnMap = false;
            LinkedTo->SetHeight(0);
            LinkedTo->IsOnMap = onMap;
            LinkedTo->PrimaryFacing.SetCurrent(DirStruct(static_cast<unsigned short>(unsigned(facing) << 8)));
            if (TrackIndex && raw->CellIndex == TrackIndex) LinkedTo->UnmarkAllOccupationBits(LinkedTo->Location);
            if (changing && raw->JumpIndex == TrackIndex && TrackIndex) {
                const int number = int(((unsigned(control->Face) + 16) >> 5) & 7) * 8 + nextFace;
                const auto* nextControl = &TurnTrack[number];
                const int index = nextControl->NormalTrackStructIndex;
                if (index && RawTrack[index].EntryIndex) {
                    const double savedSpeed = LinkedTo->SpeedPercentage;
                    const auto offset = Unsorted::AdjacentCoord[nextFace & 7];
                    const CoordStruct head{add(HeadToCoord.X, offset.X), add(HeadToCoord.Y, offset.Y), HeadToCoord.Z};
                    const auto cell = cell_of(head);
                    const int level = LinkedTo->GetCellLevel();
                    const auto move = LinkedTo->IsCellOccupied(map.GetCellAt(cell), static_cast<FacingType>(nextFace),
                                                               level, nullptr, true);
                    switch (move) {
                        case Move::OK:
                        case Move::MovingBlock:
                            if (LinkedTo->WhatAmI() == AbstractType::Unit &&
                                !static_cast<UnitClass*>(LinkedTo)->Type->Passive)
                                break;
                            IsOnShortTrack = false;
                            TrackNumber = number;
                            control = nextControl;
                            raw = &RawTrack[index];
                            changing = false;
                            TrackIndex = raw->EntryIndex - 1;
                            Stop_Driver();
                            IsDriving = true;
                            LinkedTo->UpdatePosition(PCPType::End);
                            IsDriving = false;
                            if (!active(LinkedTo)) return false;
                            if (Start_Driver(head)) {
                                LinkedTo->SetSpeedPercentage(savedSpeed);
                                advance_path(LinkedTo);
                            }
                            break;
                        case Move::Cloak:
                            map.GetCellAt(head)->RevealCellObjects();
                            break;
                        case Move::ClosedGate:
                            map.MakeTraversable(LinkedTo, cell);
                            break;
                        case Move::Temp: {
                            bool upper = false;
                            if (bridge(map.GetCellAt(head))) {
                                const int z = LinkedTo->Location.Z;
                                const int level = static_cast<signed char>(map.GetCellAt(head)->Level);
                                upper = absolute(subtract(z / Unsorted::LevelHeight, level)) > 2;
                            }
                            map.GetCellAt(HeadToCoord)->ScatterContent(CoordStruct::Empty, true, true, upper);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            ++TrackIndex;
        }
    }
    SpeedAccum = actual;
    if (actual <= 0 || TrackNumber < 0) return false;
    const auto& control = TurnTrack[TrackNumber];
    const auto& point = RawTrack[IsOnShortTrack ? control.ShortTrackStructIndex : control.NormalTrackStructIndex]
                            .TrackPoint[TrackIndex];
    if (point.Point == Point2D{0, 0} && TrackIndex) return false;
    const auto old = LinkedTo->Location;
    int facing = point.Face;
    const auto p = Smooth_Turn(point.Point, facing);
    const CoordStruct delta{subtract(p.X, old.X), subtract(p.Y, old.Y), 0};
    const CoordStruct full{add(LinkedTo->Location.X, delta.X), add(LinkedTo->Location.Y, delta.Y),
                           LinkedTo->Location.Z};
    const auto* fullCell = map.GetCellAt(full);
    // Coord scale 0x75F540 receives a float, rounded before its first ftol call.
    const float factor = game::x87_multiply_to_float(double(SpeedAccum), 0.1428571428571428);
    const int dx = game::x87_integer(delta.X * double(factor)), dy = game::x87_integer(delta.Y * double(factor)),
              dz = game::x87_integer(delta.Z * double(factor));
    const CoordStruct partial{add(old.X, dx), add(old.Y, dy), add(old.Z, dz)};
    const auto* oldCell = map.GetCellAt(old);
    const auto* partialCell = map.GetCellAt(partial);
    auto chosen = LinkedTo->Location;
    if (partialCell == fullCell || partialCell == oldCell)
        chosen = partial;
    else if (SpeedAccum > 3)
        chosen = full;
    const auto* chosenCell = map.GetCellAt(chosen);
    if (cell_of(chosen) == cell_of(LinkedTo->Location)) {
        const bool onMap = LinkedTo->IsOnMap;
        LinkedTo->IsOnMap = false;
        LinkedTo->SetLocation(chosen);
        LinkedTo->IsOnMap = onMap;
    } else {
        LinkedTo->Mark(MarkType::Up);
        LinkedTo->SetLocation(chosen);
        update_bridge(LinkedTo, oldCell, chosenCell);
        LinkedTo->Mark(MarkType::Down);
    }
    return false;
}

// YR 0x6A1C80. Retry changes are owned by the original call sites, not
// an arbitrary recursion budget. The world/path callbacks must honor
// their original lifetime and path-result contracts.
bool ShipLocomotionClass::Start_Of_Move(bool& stopProcessing, bool retry, bool forceStraight) {
    int facing = LinkedTo->PathDirections[0];
    if (!Is_Moving() && facing == -1) {
        IsTurretLockedDown = 0;
        Stop_Driver();
        if (LinkedTo->GetCurrentMission() == Mission::Move) stopProcessing = LinkedTo->EnterIdleMode(false, true);
        return false;
    }
    if (DestinationCoord == CoordStruct::Empty) return false;
    if (LinkedTo->SpawnManager && LinkedTo->SpawnManager->CountLaunchingSpawns()) return true;
    if (LinkedTo->IsBeingWarpedOut() || LinkedTo->IsWarpingIn()) return false;
    if (LinkedTo->IsUnderEMP() || LinkedTo->IsParalyzed()) return true;
    auto& map = MapClass::Instance;
    const auto coords = [&] {
        CoordStruct p;
        LinkedTo->GetCoords(&p);
        return p;
    };
    const auto length = [&] {
        const auto p = coords();
        const double x = subtract(p.X, DestinationCoord.X), y = subtract(p.Y, DestinationCoord.Y),
                     z = subtract(p.Z, DestinationCoord.Z);
        return game::x87_integer(Math::sqrt(x * x + y * y + z * z));
    };
    const auto reset_path = [&] {
        LinkedTo->PathDirections[0] = -1;
        LinkedTo->PathDelayTimer.Start(0);
        return Start_Of_Move(stopProcessing, false, false);
    };
    const auto scold = [&] {
        if (LinkedTo->ShouldScanForTarget)
            VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000, 1.0f, nullptr);
    };
    if (facing != -1) {
        if ((LinkedTo->Destination && LinkedTo->Destination->WhatAmI() == AbstractType::Unit) ||
            (LinkedTo->Destination && LinkedTo->Destination->WhatAmI() == AbstractType::Infantry)) {
            const int count = length() / 256;
            if (count < 24) {
                LinkedTo->PathDirections[count] = -1;
                facing = LinkedTo->PathDirections[0];
            }
        }
    }
    if (facing == -1) {
        if (LinkedTo->PathDelayTimer.GetTimeLeft()) return false;
        LinkedTo->PathDelayTimer.Start(game::x87_integer(RulesClass::Instance->PathDelay * 900.0));
        const bool found = LinkedTo->UpdatePathfinding(cell_of(DestinationCoord), 0, 0);
        if (LinkedTo && game::path_request_pending(*LinkedTo)) return false;
        if (!LinkedTo) {
            stopProcessing = true;
            return false;
        }
        // The initial path search inspects a neighbor both on failure and
        // success. A terminal result must return directly from this method.
        const auto inspect_neighbor = [&](CellStruct next, int direction, bool refreshDirection) {
            if (!map.IsWithinUsableArea(next, true)) return -1;
            if (refreshDirection) direction = int(LinkedTo->PrimaryFacing.Current().GetValue<3>());
            const int level = LinkedTo->GetCellLevel();
            const auto move =
                LinkedTo->IsCellOccupied(map.GetCellAt(next), static_cast<FacingType>(direction), level, nullptr, true);
            if (move == Move::ClosedGate)
                map.MakeTraversable(LinkedTo, next);
            else if (move == Move::Temp) {
                auto* tile = map.GetCellAt(next);
                const int height = LinkedTo->Location.Z;
                const CoordStruct center{next.X * 256 + 128, next.Y * 256 + 128, 0};
                const int floor = map.GetCellFloorHeight(center);
                auto* blocker =
                    tile->FindTechnoNearestTo(Point2D{0, 0}, height > add(floor, 2 * Unsorted::LevelHeight), nullptr);
                if (blocker && LinkedTo->Owner->IsAlliedWith(blocker) && !LinkedTo->GetTechnoType()->IsTrain) {
                    if (length() < RulesClass::Instance->CloseEnough && !LinkedTo->HasAnyLink() &&
                        absolute(subtract(DestinationCoord.Z, LinkedTo->Location.Z)) < 2 * Unsorted::LevelHeight &&
                        map.GetCellAt(LinkedTo->Location)->LandType != LandType::Tunnel) {
                        Stop_Driver();
                        return Abandon_Navigation() ? 1 : 0;
                    }
                    const bool upper = bridge(tile) && absolute(subtract(LinkedTo->Location.Z / Unsorted::LevelHeight,
                                                                         static_cast<signed char>(tile->Level))) > 2;
                    tile->ScatterContent(CoordStruct::Empty, true, true, upper);
                }
            }
            return -1;
        };
        if (!found) {
            if (!LinkedTo->IsInSameZoneAsCoords(DestinationCoord)) {
                LinkedTo->SetDestination(nullptr, true);
                return false;
            }
            if (DestinationCoord == CoordStruct::Empty) return false;
            if (LinkedTo->GetCurrentMission() != Mission::Enter && length() < RulesClass::Instance->CloseEnough &&
                (LinkedTo->GetCurrentMission() == Mission::Move ||
                 LinkedTo->GetCurrentMission() == Mission::Area_Guard)) {
                Stop_Driver();
                if (Abandon_Navigation()) return true;
                if (!LinkedTo->IsAlive) return false;
            } else {
                const auto direction = LinkedTo->PrimaryFacing.Current().GetValue<3>();
                const auto current = cell_of(coords());
                const auto offset = Unsorted::AdjacentCell[direction];
                const int result =
                    inspect_neighbor({short(current.X + offset.X), short(current.Y + offset.Y)}, direction, true);
                if (result != -1) return result != 0;
                if (LinkedTo->PathWaitTimes <= 0) {
                    Stop_Driver();
                    if (Abandon_Navigation()) return true;
                    if (!LinkedTo->IsAlive) return false;
                    scold();
                    LinkedTo->ShouldScanForTarget = false;
                } else
                    --LinkedTo->PathWaitTimes;
            }
            if (!Is_Moving() && LinkedTo->Target && !LinkedTo->IsCloseEnoughToAttack(LinkedTo->Target)) {
                LinkedTo->IsScanLimited = true;
                if (LinkedTo->Team) LinkedTo->Team->ScanLimit();
                LinkedTo->SetTarget(nullptr);
            }
            Stop_Driver();
            TrackNumber = -1;
            IsTurretLockedDown = 0;
            return false;
        }
        if (LinkedTo->PathDirections[0] == 8) return false;
        const auto current = cell_of(coords());
        const int direction = LinkedTo->PathDirections[0];
        const auto offset = Unsorted::AdjacentCell[direction & 7];
        const int result =
            inspect_neighbor({short(current.X + offset.X), short(current.Y + offset.Y)}, direction, false);
        if (result != -1) return result != 0;
        LinkedTo->PathWaitTimes = 10;
        facing = LinkedTo->PathDirections[0];
    }
    if (facing == 8) return false;
    const auto offset = Unsorted::AdjacentCoord[facing & 7];
    CoordStruct destination{add(LinkedTo->Location.X, offset.X), add(LinkedTo->Location.Y, offset.Y),
                            LinkedTo->Location.Z};
    const auto* current = map.GetCellAt(LinkedTo->Location);
    const int level = static_cast<signed char>(current->Level) + (LinkedTo->OnBridge ? 4 : 0);
    const CoordStruct queried{add(LinkedTo->Location.X, offset.X), add(LinkedTo->Location.Y, offset.Y),
                              LinkedTo->Location.Z};
    if (LinkedTo->OnBridge != bridge(map.GetCellAt(queried))) LinkedTo->unknown_bool_68B = true;
    if (!LinkedTo->vt_entry_29C()) return true;
    if (!map.MakeTraversable(LinkedTo, cell_of(destination))) return true;
    const DirStruct direction(static_cast<unsigned short>(unsigned(facing) << 13));
    if (LinkedTo->PrimaryFacing.Current().Raw != direction.Raw) {
        Do_Turn(direction);
        return true;
    }
    auto nextCell = cell_of(destination);
    auto* tile = map.GetCellAt(nextCell);
    LinkedTo->Mark(MarkType::Up);
    auto move = LinkedTo->IsCellOccupied(tile, static_cast<FacingType>(facing), level, nullptr, true);
    LinkedTo->Mark(MarkType::Down);
    if ((move < Move::No && LinkedTo->GetTechnoType()->IsTrain) ||
        ((move == Move::Destroyable || move == Move::FriendlyDestroyable) && LinkedTo->GetTechnoType()->Crusher &&
         tile->OverlayTypeIndex == 0))
        move = Move::OK;
    bool rocking = false;
    if (tile->OverlayTypeIndex != -1 && move == Move::OK) {
        const auto* overlay = OverlayTypeClass::Array[tile->OverlayTypeIndex];
        rocking = overlay->Crushable ||
                  (overlay->Wall && LinkedTo->GetTechnoType()->MovementZone == MovementZone::CrusherAll);
    }
    if (move != Move::OK) {
        if (move == Move::ClosedGate)
            map.MakeTraversable(LinkedTo, nextCell);
        else if (move == Move::Temp) {
            if (!LinkedTo->GetTechnoType()->IsTrain) {
                if (retry) return reset_path();
                if (distance(coords(), DestinationCoord) >= RulesClass::Instance->CloseEnough ||
                    absolute(subtract(DestinationCoord.Z, LinkedTo->Location.Z)) >= 2 * Unsorted::LevelHeight ||
                    map.GetCellAt(LinkedTo->Location)->LandType == LandType::Tunnel) {
                    bool upper = false;
                    if (bridge(map.GetCellAt(nextCell))) {
                        const int z = LinkedTo->Location.Z;
                        upper = absolute(subtract(z / Unsorted::LevelHeight,
                                                  static_cast<signed char>(map.GetCellAt(nextCell)->Level))) > 2;
                    }
                    map.GetCellAt(nextCell)->ScatterContent(CoordStruct::Empty, true, true, upper);
                } else {
                    Stop_Driver();
                    if (Abandon_Navigation()) return true;
                }
            }
        } else if (move == Move::Cloak) {
            map.GetCellAt(nextCell)->RevealCellObjects();
            if (retry) {
                LinkedTo->PathDirections[0] = -1;
                return Start_Of_Move(stopProcessing, false, false);
            }
            Stop_Driver();
            return Abandon_Navigation();
        }
        Stop_Driver();
        if (move == Move::MovingBlock) {
            if (game::wait_for_moving_blocker(*LinkedTo)) return true;
            if (!LinkedTo->IsWaitingBlockagePath) {
                LinkedTo->IsWaitingBlockagePath = true;
                LinkedTo->BlockagePathTimer.Start(RulesClass::Instance->BlockagePathDelay);
            }
            if (!LinkedTo->PathDelayTimer.GetTimeLeft()) {
                const bool around = LinkedTo->IsWaitingBlockagePath && !LinkedTo->BlockagePathTimer.GetTimeLeft();
                const bool found = LinkedTo->UpdatePathfinding(cell_of(DestinationCoord), 0, around ? 2 : 1);
                if (LinkedTo && game::path_request_pending(*LinkedTo)) return true;
                if (!LinkedTo) {
                    stopProcessing = true;
                    return false;
                }
                if (found || LinkedTo->IsInSameZoneAsCoords(DestinationCoord)) {
                    LinkedTo->PathDelayTimer.Start(game::x87_integer(RulesClass::Instance->PathDelay * 900.0));
                    return true;
                }
                LinkedTo->SetDestination(nullptr, true);
                return false;
            }
            scold();
        } else if (move == Move::Destroyable || move == Move::FriendlyDestroyable) {
            if (retry) return reset_path();
            if (map.GetCellAt(nextCell)->GetSomeObject(Point2D{0, 0}, false)) {
                auto* object = map.GetCellAt(nextCell)->GetSomeObject(Point2D{0, 0}, false);
                if (!LinkedTo->Owner->IsAlliedWith(object)) {
                    auto* target = map.GetCellAt(nextCell)->GetSomeObject(Point2D{0, 0}, false);
                    LinkedTo->Override_Mission(Mission::Attack, target, nullptr);
                }
            } else if (map.GetCellAt(nextCell)->OverlayTypeIndex != -1 &&
                       OverlayTypeClass::Array[map.GetCellAt(nextCell)->OverlayTypeIndex]->Wall)
                LinkedTo->Override_Mission(Mission::Attack, map.GetCellAt(nextCell), nullptr);
        } else
            scold();
        if (move == Move::No) {
            if (retry) return reset_path();
            Stop_Driver();
            return Abandon_Navigation();
        }
        LinkedTo->ShouldScanForTarget = false;
        TrackNumber = -1;
        return true;
    }
    int nextLevel;
    LandType land;
    if (absolute(subtract(level, static_cast<signed char>(map.GetCellAt(nextCell)->Level))) >= 2) {
        nextLevel = level;
        land = LandType::Road;
    } else {
        nextLevel = static_cast<signed char>(map.GetCellAt(nextCell)->Level);
        land = map.GetCellAt(nextCell)->LandType;
    }
    double speed = GroundType::Array[int(land)].Cost[int(LinkedTo->GetTechnoType()->SpeedType)];
    if (speed > 1.0) speed = 1.0;
    const auto center = map.GetCellAt(nextCell)->GetCellCoords();
    const int height = map.GetCellFloorHeight(center), oldHeight = map.GetCellFloorHeight(LinkedTo->Location);
    if (height > oldHeight && LinkedTo->WhatAmI() == AbstractType::Unit) {
        speed *= LinkedTo->GetTechnoType()->SpeedType == SpeedType::Track ? RulesClass::Instance->TrackedUphill
                                                                          : RulesClass::Instance->WheeledUphill;
    } else if (height < oldHeight && LinkedTo->WhatAmI() == AbstractType::Unit) {
        speed *= LinkedTo->GetTechnoType()->SpeedType == SpeedType::Track ? RulesClass::Instance->TrackedDownhill
                                                                          : RulesClass::Instance->WheeledDownhill;
    }
    if (speed == 0.0) speed = 0.5;
    if (!(LinkedTo->GetHealthPercentage() > RulesClass::Instance->ConditionYellow)) speed *= 0.75;
    if (TrackNumber >= 64) {
        if (speed != LinkedTo->SpeedPercentage) LinkedTo->SetSpeedPercentage(speed);
    } else
        movementspeed_50 = speed;
    LinkedTo->TryCrushCell(cell_of(destination), true);
    int next = LinkedTo->PathDirections[1];
    if (next == -1) {
        if (distance(coords(), DestinationCoord) <= 512)
            next = facing;
        else {
            const bool train = LinkedTo->GetTechnoType()->IsTrain;
            const bool found = LinkedTo->UpdatePathfinding(cell_of(DestinationCoord), train ? 1 : 0, 0);
            if (!found) {
                if (!LinkedTo) {
                    stopProcessing = true;
                    return false;
                }
                if (!game::path_request_pending(*LinkedTo) && !LinkedTo->IsInSameZoneAsCoords(DestinationCoord))
                    LinkedTo->SetDestination(nullptr, true);
            }
            next = LinkedTo->PathDirections[1];
        }
    }
    if (next == 8 || next == -1 || forceStraight) next = facing;
    bool nextRocking = false;
    if (next != -1)
        if (auto* neighbor = tile->GetNeighbourCell(static_cast<FacingType>(next))) {
            if (neighbor->OverlayTypeIndex != -1) {
                const auto* overlay = OverlayTypeClass::Array[neighbor->OverlayTypeIndex];
                nextRocking = overlay->Crushable ||
                              (overlay->Wall && LinkedTo->GetTechnoType()->MovementZone == MovementZone::CrusherAll);
            }
        }
    IsRocking = nextRocking || rocking;
    if (IsRocking) next = facing;
    IsOnShortTrack = false;
    TrackNumber = next + 8 * facing;
    if (!TurnTrack[TrackNumber].NormalTrackStructIndex) TrackNumber = 9 * facing;
    if (TurnTrack[TrackNumber].Flag & 8) {
        Move second = Move::No;
        if (map.GetCellAt(nextCell)->CollectCrate(LinkedTo) || LinkedTo->InLimbo) {
            if (!LinkedTo->IsAlive) return false;
            const auto offset = Unsorted::AdjacentCoord[next & 7];
            destination.X = add(destination.X, offset.X);
            destination.Y = add(destination.Y, offset.Y);
            nextCell = cell_of(destination);
            second = LinkedTo->IsCellOccupied(map.GetCellAt(nextCell), static_cast<FacingType>(next), nextLevel,
                                              nullptr, true);
            if ((second < Move::No && LinkedTo->GetTechnoType()->IsTrain) ||
                ((second == Move::FriendlyDestroyable || second == Move::Destroyable) &&
                 LinkedTo->GetTechnoType()->Crusher && map.GetCellAt(nextCell)->OverlayTypeIndex == 0))
                second = Move::OK;
        }
        if (!LinkedTo->IsAlive) return false;
        if (second != Move::OK) {
            switch (second) {
                case Move::ClosedGate:
                    map.MakeTraversable(LinkedTo, nextCell);
                    break;
                case Move::MovingBlock:
                    return Start_Of_Move(stopProcessing, retry, true);
                case Move::Temp:
                    if (!LinkedTo->GetTechnoType()->IsTrain) {
                        if (retry) return reset_path();
                        const auto p = coords();
                        const double x = subtract(p.X, DestinationCoord.X), y = subtract(p.Y, DestinationCoord.Y),
                                     z = subtract(p.Z, DestinationCoord.Z);
                        if (game::x87_integer(Math::sqrt(x * x + z * z + y * y)) >= RulesClass::Instance->CloseEnough ||
                            absolute(subtract(DestinationCoord.Z, LinkedTo->Location.Z)) >= 2 * Unsorted::LevelHeight ||
                            map.GetCellAt(LinkedTo->Location)->LandType == LandType::Tunnel) {
                            auto* blocked = map.GetCellAt(nextCell);
                            const bool upper =
                                bridge(blocked) && absolute(subtract(LinkedTo->Location.Z / Unsorted::LevelHeight,
                                                                     static_cast<signed char>(blocked->Level))) > 2;
                            blocked->ScatterContent(CoordStruct::Empty, true, true, upper);
                        } else {
                            Stop_Driver();
                            if (Abandon_Navigation()) return true;
                        }
                    }
                    break;
                case Move::Cloak:
                    map.GetCellAt(nextCell)->RevealCellObjects();
                    if (retry) {
                        LinkedTo->PathDirections[0] = -1;
                        return Start_Of_Move(stopProcessing, false, false);
                    }
                    Stop_Driver();
                    return Abandon_Navigation();
                case Move::No:
                    if (retry) return reset_path();
                    Stop_Driver();
                    return Abandon_Navigation();
                default:
                    break;
            }
            LinkedTo->PathDirections[0] = -1;
            TrackNumber = -1;
            destination = CoordStruct::Empty;
            if (second == Move::Destroyable || second == Move::FriendlyDestroyable)
                return Start_Of_Move(stopProcessing, retry, true);
        } else {
            std::memmove(LinkedTo->PathDirections, LinkedTo->PathDirections + 2, 22 * sizeof(int));
            LinkedTo->PathDirections[22] = -1;
            LinkedTo->unknown_bool_68B = true;
        }
    } else
        std::memmove(LinkedTo->PathDirections, LinkedTo->PathDirections + 1, 23 * sizeof(int));
    LinkedTo->PathDirections[23] = -1;
    LinkedTo->CurrentMapCoords = cell_of(destination);
    LinkedTo->ShouldScanForTarget = false;
    TrackIndex = 0;
    if (!Start_Driver(destination)) {
        TrackNumber = -1;
        LinkedTo->PathDirections[0] = -1;
        LinkedTo->SetSpeedPercentage(0);
    }
    return false;
}
