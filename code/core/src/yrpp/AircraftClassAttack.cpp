// Original aircraft attack mission and firing-position search, YR 0x417FE0/0x4197C0.
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include "com_error.hpp"
#include <bit>
#include <cstdlib>

bool AircraftClass::IsCellClearToLand(const CellStruct* cell, bool includeSelf) {
    auto& map = MapClass::Instance;
    if (!map.IsWithinUsableArea(*cell, true)) return true;
    if (Type->Spawned) {
        auto* occupant = map.GetCellAt(*cell)->FindTechnoNearestTo(Point2D::Empty, false, nullptr);
        if (occupant && (occupant->SpawnManager || occupant->GetTechnoType()->Spawned)) return true;
    }
    if (Type->AirportBound) return true;
    const bool carry = Type->Carryall && Destination && Destination->WhatAmI() == AbstractType::Unit;
    auto* destination = map.GetCellAt(*cell);
    for (int i = 0; i < FootClass::Array.Count; ++i) {
        auto* foot = FootClass::Array[i];
        if (!foot || (carry && Destination == foot) || (!includeSelf && foot == this) || foot->InLimbo ||
            !foot->IsOnMap)
            continue;
        if (foot->GetMapCoords() == *cell || foot->Destination == destination) return false;
    }
    return true;
}

AbstractClass* AircraftClass::FindFireLocation(AbstractClass* target) {
    if (!target) return nullptr;
    if (Is_Strafe()) return target;
    const int range = GetWeaponRange(0);
    const auto at = target->GetCoords();
    CellStruct best{-1, -1}, second{-1, -1};
    int best_distance = -1, second_distance = -1;
    CoordStruct heading{-1, -1, -1};
    if ((target->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None)
        if (auto* destination = static_cast<FootClass*>(target)->Destination) heading = destination->GetCoords();
    for (int radius = range - 256; radius > 256; radius -= 256) {
        for (int facing = 0; facing < 255; facing += 16) {
            const double angle =
                (std::bit_cast<short>(static_cast<unsigned short>(facing << 8)) - 0x3FFF) * -0.00009587672516830327;
            const int y = game::x87_integer(double(at.Y) - Math::sin(angle) * radius);
            const int x = game::x87_integer(double(at.X) + Math::cos(angle) * radius);
            CellStruct cell{short(x / 256), short(y / 256)};
            if (!MapClass::Instance.IsWithinUsableArea(cell, true)) continue;
            if (!Unsorted::ScenarioInit && !IsALoaner &&
                (MapClass::Instance.GetCellAt(cell)->AltFlags & AltCellFlags::NoFog) == AltCellFlags{})
                continue;
            if (!IsCellClearToLand(&cell, true)) continue;
            const auto origin = heading == CoordStruct{-1, -1, -1} ? GetCoords() : heading;
            const int dx = origin.X - x, dy = origin.Y - y;
            const int distance = game::x87_integer(Math::sqrt(double(dx) * dx + double(dy) * dy));
            if (best_distance == -1 || distance < best_distance) {
                second_distance = best_distance;
                second = best;
                best_distance = distance;
                best = cell;
            }
        }
        if (best_distance != -1) break;
    }
    if (second_distance == -1) second = best;
    if (best_distance == -1) return nullptr;
    return MapClass::Instance.GetCellAt(ScenarioClass::Instance->Random.RandomRanged(0, 99) < 50 ? best : second);
}

int AircraftClass::Mission_Attack() {
    const auto rate = [&] {
        return game::x87_integer(CurrentMissionControl()->Rate * 900.0) +
               ScenarioClass::Instance->Random.RandomRanged(0, 2);
    };
    const auto direction = [&] {
        DirStruct dir;
        GetDirectionTo(&dir, Target);
        return dir;
    };
    const auto face = [&] {
        SecondaryFacing.SetDesired(direction());
        PrimaryFacing.SetDesired(direction());
    };
    const auto scatter = [&] {
        const auto at = Target->GetCoords();
        MapClass::Instance.GetCellAt(at)->ScatterContent(Location, true, false, false);
    };
    switch (static_cast<AirAttackStatus>(MissionStatus)) {
        case AirAttackStatus::ValidateAZ:
            IsLocked = false;
            MissionStatus =
                static_cast<int>(Target ? AirAttackStatus::PickAttackLocation : AirAttackStatus::ReturnToBase);
            return 1;
        case AirAttackStatus::PickAttackLocation:
            IsLocked = false;
            if (ShouldLoseAmmo) {
                ShouldLoseAmmo = false;
                --Ammo;
            }
            if (Target && Ammo) {
                SetDestination(FindFireLocation(Target), true);
                MissionStatus =
                    static_cast<int>(Destination ? AirAttackStatus::FlyToPosition : AirAttackStatus::ReturnToBase);
            } else
                MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
            return rate();
        case AirAttackStatus::FlyToPosition:
            if (ShouldLoseAmmo) {
                ShouldLoseAmmo = false;
                --Ammo;
            }
            IsLocked = false;
            if (!Target || !Ammo) {
                MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                return 1;
            }
            if (Is_Strafe()) {
                if (DistanceFrom(Target) < GetWeapon(0)->WeaponType->Range) {
                    MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget);
                    return 1;
                }
                SetDestination(Target, true);
            } else if (Is_Fighter()) {
                MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget);
                return 1;
            } else {
                if (!Locomotor) game::raise_com_error(static_cast<std::int32_t>(0x80004003u));
                if (!Locomotor->Is_Moving_Now()) {
                    MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget);
                    return 1;
                }
            }
            if (!Destination) {
                MissionStatus = static_cast<int>(AirAttackStatus::PickAttackLocation);
                return 1;
            }
            if (DistanceFrom(Destination) < 512) {
                SecondaryFacing.SetDesired(direction());
                if (DistanceFrom(Destination) < 16) {
                    MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget);
                    SetDestination(nullptr, true);
                }
            } else {
                const auto to = Destination->GetCoords();
                CoordStruct at;
                GetFLH(&at, 0, CoordStruct::Empty);
                const double angle = Math::atan2(double(at.Y) - to.Y, double(to.X) - at.X);
                SecondaryFacing.SetDesired(DirStruct(game::x87_integer((angle - Math::HalfPi) * -10430.06004058427)));
            }
            return 1;
        case AirAttackStatus::FireAtTarget: {
            if (!Target || !Ammo) {
                MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                return 1;
            }
            if (!Is_Strafe()) face();
            switch (GetFireError(Target, SelectWeapon(Target), true)) {
                case FireError::OK:
                    ShouldLoseAmmo = true;
                    for (int i = 0; i < GetWeapon(SelectWeapon(Target))->WeaponType->Burst; ++i)
                        Fire(Target, SelectWeapon(Target));
                    scatter();
                    if (Is_Strafe()) {
                        MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget2_Strafe);
                        IsLocked = true;
                        return GetWeapon(0)->WeaponType->ROF;
                    }
                    if (Is_Fighter()) {
                        IsLocked = true;
                        MissionStatus = static_cast<int>(Ammo > 0 ? AirAttackStatus::PickAttackLocation
                                                                  : AirAttackStatus::ReturnToBase);
                        return GetWeapon(0)->WeaponType->ROF;
                    }
                    MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget2);
                    return 1;
                case FireError::FACING:
                    if (!Ammo) {
                        MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                        return 1;
                    }
                    if (!IsCloseEnoughToAttack(Target) || Is_Strafe())
                        MissionStatus = static_cast<int>(AirAttackStatus::PickAttackLocation);
                    else if (Is_Fighter())
                        MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget);
                    else
                        MissionStatus =
                            static_cast<int>(RulesClass::Instance->CurleyShuffle ? AirAttackStatus::PickAttackLocation
                                                                                 : AirAttackStatus::FireAtTarget);
                    return Is_Strafe() ? 45 : 1;
                case FireError::REARM:
                    return 1;
                case FireError::CLOAKED:
                    Uncloak(false);
                    return 1;
                default:
                    if (!Ammo)
                        MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                    else if (!Is_Strafe())
                        MissionStatus = static_cast<int>(AirAttackStatus::FireAtTarget2);
                    return 1;
            }
        }
        case AirAttackStatus::FireAtTarget2:
            if (!Target) {
                MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                return 1;
            }
            face();
            switch (GetFireError(Target, SelectWeapon(Target), true)) {
                case FireError::OK:
                    Fire(Target, SelectWeapon(Target));
                    scatter();
                    MissionStatus = static_cast<int>(Ammo ? (RulesClass::Instance->CurleyShuffle
                                                                 ? AirAttackStatus::PickAttackLocation
                                                                 : AirAttackStatus::FireAtTarget)
                                                          : AirAttackStatus::ReturnToBase);
                    break;
                case FireError::FACING:
                    if (!Ammo)
                        MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                    else {
                        MissionStatus = static_cast<int>(!IsCloseEnoughToAttack(Target) || Is_Strafe()
                                                             ? AirAttackStatus::PickAttackLocation
                                                             : (RulesClass::Instance->CurleyShuffle
                                                                    ? AirAttackStatus::PickAttackLocation
                                                                    : AirAttackStatus::FireAtTarget));
                        if (Is_Strafe()) return 45;
                    }
                    break;
                case FireError::REARM:
                    break;
                case FireError::CLOAKED:
                    Uncloak(false);
                    break;
                default:
                    MissionStatus = static_cast<int>(
                        Ammo ? (IsCloseEnoughToAttack(Target)
                                    ? (RulesClass::Instance->CurleyShuffle ? AirAttackStatus::PickAttackLocation
                                                                           : AirAttackStatus::FireAtTarget)
                                    : AirAttackStatus::PickAttackLocation)
                             : AirAttackStatus::ReturnToBase);
                    break;
            }
            return rate();
        case AirAttackStatus::FireAtTarget2_Strafe:
        case AirAttackStatus::FireAtTarget3_Strafe:
        case AirAttackStatus::FireAtTarget4_Strafe:
        case AirAttackStatus::FireAtTarget5_Strafe: {
            if (!Target) {
                MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                return 1;
            }
            const auto error = GetFireError(Target, SelectWeapon(Target), true);
            if (error != FireError::OK && error != FireError::FACING && error != FireError::CLOAKED &&
                error != FireError::RANGE) {
                if (!Ammo) {
                    MissionStatus = static_cast<int>(AirAttackStatus::ReturnToBase);
                    IsLocked = false;
                }
                return 1;
            }
            if (error == FireError::RANGE && MissionStatus != static_cast<int>(AirAttackStatus::FireAtTarget5_Strafe))
                SetDestination(Target, true);
            Fire(Target, SelectWeapon(Target));
            scatter();
            if (MissionStatus == static_cast<int>(AirAttackStatus::FireAtTarget5_Strafe)) {
                MissionStatus = static_cast<int>(AirAttackStatus::FlyToPosition);
                return (GetWeapon(0)->WeaponType->Range + 1024) / Type->Speed;
            }
            SetDestination(Target, true);
            ++MissionStatus;
            return GetWeapon(0)->WeaponType->ROF;
        }
        case AirAttackStatus::ReturnToBase: {
            IsLocked = false;
            if (ShouldLoseAmmo) {
                ShouldLoseAmmo = false;
                if (Ammo > 0) --Ammo;
            }
            if (Ammo && Target) {
                MissionStatus = static_cast<int>(AirAttackStatus::PickAttackLocation);
                return 1;
            }
            if (!Ammo && (IsALoaner || Owner->IsControlledByHuman())) SetTarget(nullptr);
            auto edge = static_cast<unsigned>(Owner->Edge) < 4 ? Owner->Edge : static_cast<::Edge>(0);
            const auto cell =
                MapClass::Instance.PickCellOnEdge(edge, CellStruct{-1, -1}, CellStruct{-1, -1},
                                                  static_cast<SpeedType>(4), true, static_cast<MovementZone>(0));
            SetDestination(MapClass::Instance.GetCellAt(cell), true);
            IsReturningFromAttackRun = false;
            if (Airstrike && Ammo > 0)
                QueueMission(Mission::Guard, false);
            else
                EnterIdleMode(false, true);
            IsReturningFromAttackRun = true;
            return 1;
        }
        default:
            return rate();
    }
}

// Original 0x414A80. Airstrike loaners keep their explicitly supplied target.
int AircraftClass::Mission_Hunt() {
    if (!Ammo) {
        if (Team) Team->LiberateMember(this, -1, 0);
        EnterIdleMode(false, true);
        return game::x87_integer(CurrentMissionControl()->Rate * 900.0) +
               ScenarioClass::Instance->Random.RandomRanged(0, 2);
    }
    if (!Target && SessionClass::Instance.GameMode != GameMode::Campaign) {
        auto at = Location;
        SetTarget(GreatestThreat(static_cast<ThreatType>(0x40), &at, false));
    }
    if (!Target) {
        auto at = Location;
        SetTarget(GreatestThreat(static_cast<ThreatType>(0), &at, false));
    }
    if (Target)
        QueueMission(Mission::Attack, false);
    else
        EnterIdleMode(false, true);
    return 1;
}
