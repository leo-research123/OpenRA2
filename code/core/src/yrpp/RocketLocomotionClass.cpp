// Original rocket launch, climb, curve and impact, YR 0x661EC0..0x6635C0.
#include "yrpp/RocketLocomotionClass.h"
#include "runtime_diagnostics.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include "Matrix3DArithmetic.hpp"
#include <bit>
#include <cmath>

namespace {
RocketStruct& rocket_for(FootClass* foot) {
    auto& rules = *RulesClass::Instance;
    auto* type = static_cast<AircraftClass*>(foot)->Type;
    if (type == rules.V3Rocket.Type) return rules.V3Rocket;
    if (type == rules.CMisl.Type) return rules.CMisl;
    return rules.DMisl;
}
void animation(const char* name, const CoordStruct& at, int delay, int z) {
    if (auto* storage = YRMemory::Allocate(sizeof(AnimClass)))
        ::new (storage) AnimClass(AnimTypeClass::Find(name), at, delay, 1, 0x600, z, false);
}
CoordStruct advance(FootClass* foot, float pitch, int distance) {
    auto at = foot->Location;
    const int horizontal = game::x87_integer(Math::cos(pitch) * distance);
    const double angle = (std::bit_cast<short>(foot->PrimaryFacing.Current().Raw) - 0x3FFF) * -0.00009587672516830327;
    at.Y = game::x87_integer(double(at.Y) - Math::sin(angle) * horizontal);
    at.X = game::x87_integer(double(at.X) + Math::cos(angle) * horizontal);
    at.Z = game::x87_integer(double(at.Z) + Math::sin(pitch) * distance);
    return at;
}
double target_pitch(FootClass* foot, const CoordStruct& destination) {
    const auto& at = foot->Location;
    const int dx = destination.X - at.X, dy = destination.Y - at.Y, dz = destination.Z - at.Z;
    // 0x662AF0/0x662CE2 multiply as 32-bit integers before conversion.
    const int square = std::bit_cast<int>(unsigned(dx) * unsigned(dx) + unsigned(dy) * unsigned(dy));
    const double horizontal = Math::sqrt(double(square));
    return horizontal > 0 ? Math::atan(double(dz) / horizontal) : -Math::HalfPi;
}
int horizontal_distance(FootClass* foot, const CoordStruct& destination) {
    const auto at = foot->GetCoords();
    const int dx = at.X - destination.X, dy = at.Y - destination.Y;
    return game::x87_integer(Math::sqrt(double(dx) * dx + double(dy) * dy));
}
}

RocketLocomotionClass::RocketLocomotionClass() noexcept
    : LocomotionClass(), MovingDestination{-1, -1, -1}, MissionTimer(0), TrailerTimer(0), MissionState(0),
      unknown_44(0), CurrentSpeed(0), unknown_bool_4C(true), SpawnerIsElite(false), CurrentPitch(0), unknown_58(0),
      unknown_5C(0) {}

bool YRPP_STDCALL RocketLocomotionClass::Is_Moving() { return MovingDestination != CoordStruct{-1, -1, -1}; }
bool YRPP_STDCALL RocketLocomotionClass::Is_Moving_Now() { return MissionState >= 3 && MissionState <= 5; }
CoordStruct YRPP_STDCALL RocketLocomotionClass::Destination() { return MovingDestination; }
HRESULT YRPP_STDCALL RocketLocomotionClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    *id = CLSIDs::Rocket;
    return 0;
}
void YRPP_STDCALL RocketLocomotionClass::Move_To(CoordStruct destination) {
    auto& rocket = rocket_for(LinkedTo);
    if (MovingDestination != CoordStruct{-1, -1, -1}) return;
    if (rocket.PauseFrames) {
        MissionState = 1;
        MissionTimer.Start(rocket.PauseFrames);
    } else {
        MissionState = 2;
        MissionTimer.Start(rocket.TiltFrames);
    }
    CurrentPitch = float(rocket.PitchInitial * Math::HalfPi);
    MovingDestination = destination;
}

bool RocketLocomotionClass::ProcessImpact(RocketStruct* rocket) {
    const int nose = game::x87_integer(Math::sin(CurrentPitch) * rocket->BodyLength + LinkedTo->Location.Z);
    if (nose > MovingDestination.Z) {
        auto* cell = LinkedTo->GetCell();
        if (!cell || !cell->ContainsBridge() || cell->GetCoords().Z != MovingDestination.Z ||
            nose > MovingDestination.Z + Unsorted::LevelHeight)
            if (LinkedTo->GetHeight() > 0) return false;
    }
    Explode();
    return true;
}

void RocketLocomotionClass::Explode() {
    auto* foot = LinkedTo;
    AircraftTrackerClass::Instance.Remove(foot);
    auto& rules = *RulesClass::Instance;
    auto& rocket = rocket_for(foot);
    WarheadTypeClass* warhead;
    if (&rocket == &rules.V3Rocket)
        warhead = SpawnerIsElite ? rules.V3EliteWarhead : rules.V3Warhead;
    else if (&rocket == &rules.CMisl)
        warhead = SpawnerIsElite ? rules.CMislEliteWarhead : rules.CMislWarhead;
    else
        warhead = SpawnerIsElite ? rules.DMislEliteWarhead : rules.DMislWarhead;
    const auto at = advance(foot, CurrentPitch, rocket.BodyLength);
    const int damage = SpawnerIsElite ? rocket.EliteDamage : rocket.Damage;
    if (game::runtime_log_enabled())
        game::runtime_log_event("rocket_impact", "id=%u type=\"%.24s\" house=%d damage=%d pos=%d,%d,%d", foot->UniqueID,
                                foot->GetTechnoType()->ID, foot->Owner ? foot->Owner->ArrayIndex : -1, damage, at.X,
                                at.Y, at.Z);
    if (auto* storage = YRMemory::Allocate(sizeof(AnimClass))) {
        auto* cell = MapClass::Instance.GetCellAt(CellStruct{short(at.X / 256), short(at.Y / 256)});
        auto* type = MapClass::SelectDamageAnimation(damage, warhead, cell->LandType, at);
        ::new (storage) AnimClass(type, at, 0, 1, 0x2600, -15, false);
    }
    MapClass::FlashbangWarheadAt(damage, warhead, at, false, SpotlightFlags::None);
    MapClass::DamageArea(at, damage, foot, warhead, true, nullptr);
    foot->UnInit();
}

bool YRPP_STDCALL RocketLocomotionClass::Process() {
    enum RocketState : int { Idle = 0, Pause = 1, Tilt = 2, Climb = 3, Cruise = 4, Dive = 5, Raise = 6 };
    auto& rocket = rocket_for(LinkedTo);
    auto* foot = LinkedTo;
    const auto remember_elite = [&] { SpawnerIsElite = foot->SpawnOwner && foot->SpawnOwner->Veterancy.IsElite(); };
    const auto submit = [&](bool value) {
        foot->Mark(MarkType::Up);
        unknown_bool_4C = value;
        DisplayClass::Submit(foot);
        foot->Mark(MarkType::Down);
    };
    const auto accelerate = [&] {
        CurrentSpeed += rocket.Acceleration;
        if (CurrentSpeed > foot->GetTechnoType()->Speed) CurrentSpeed = foot->GetTechnoType()->Speed;
    };
    switch (MissionState) {
        case RocketState::Pause:
            CurrentSpeed = 0;
            remember_elite();
            if (rocket.Type == RulesClass::Instance->CMisl.Type) {
                if (!TrailerTimer.GetTimeLeft()) {
                    animation("V3TAKOFF", foot->Location, 2, -10);
                    TrailerTimer.Start(24);
                }
                if (unknown_bool_4C) submit(false);
            } else
                unknown_bool_4C = true;
            if (MissionTimer.GetRatePassed() == 1.0) {
                MissionState = rocket.Type == RulesClass::Instance->CMisl.Type ? RocketState::Raise : RocketState::Tilt;
                MissionTimer.Start(rocket.TiltFrames);
            }
            break;
        case RocketState::Tilt:
            CurrentSpeed = 0;
            remember_elite();
            if (MissionTimer.GetRatePassed() != 1.0) {
                const double initial = rocket.PitchInitial * Math::HalfPi, final = rocket.PitchFinal * Math::HalfPi;
                CurrentPitch = float((final - initial) * MissionTimer.GetRatePassed() + initial);
            } else {
                CurrentPitch = float(rocket.PitchFinal * Math::HalfPi);
                MissionState = RocketState::Climb;
                if (foot->GetLastFlightMapCoords() == CellStruct{-1, -1}) AircraftTrackerClass::Instance.Add(foot);
                animation("V3TAKOFF", foot->Location, 2, -10);
                VocClass::PlayAt(foot->GetTechnoType()->AuxSound1, foot->Location);
            }
            break;
        case RocketState::Climb:
            if (!unknown_bool_4C) submit(true);
            accelerate();
            if (foot->GetHeight() >= rocket.Altitude) {
                MissionState = RocketState::Cruise;
                unknown_58 = unsigned(horizontal_distance(foot, MovingDestination));
            }
            break;
        case RocketState::Cruise:
            if (foot->GetHeight() <= 0) {
                Explode();
                return false;
            }
            accelerate();
            if (rocket.LazyCurve && unknown_58) {
                if (ProcessImpact(&rocket)) return false;
                const double ratio =
                    double(horizontal_distance(foot, MovingDestination)) / std::bit_cast<int>(unknown_58);
                CurrentPitch = float(rocket.PitchFinal * ratio * Math::HalfPi +
                                     target_pitch(foot, MovingDestination) * (1.0 - ratio));
            } else {
                if (CurrentPitch > 0) {
                    CurrentPitch -= rocket.TurnRate;
                    if (CurrentPitch < 0) CurrentPitch = 0;
                }
                const auto at = foot->GetCoords();
                const int dx = at.X - MovingDestination.X, dy = at.Y - MovingDestination.Y,
                          dz = at.Z - foot->Location.Z;
                const int distance = game::x87_integer(Math::sqrt(double(dx) * dx + double(dy) * dy + double(dz) * dz));
                if (distance <= foot->Location.Z - MovingDestination.Z) MissionState = RocketState::Dive;
            }
            {
                const auto at = foot->GetCoords();
                const double angle =
                    Math::atan2(double(at.Y) - MovingDestination.Y, double(MovingDestination.X) - at.X);
                foot->PrimaryFacing.SetDesired(
                    DirStruct(game::x87_integer((angle - Math::HalfPi) * -10430.06004058427)));
            }
            break;
        case RocketState::Dive: {
            if (ProcessImpact(&rocket)) return false;
            const double difference = target_pitch(foot, MovingDestination) - CurrentPitch;
            if (std::abs(difference) <= rocket.TurnRate)
                CurrentPitch = float(CurrentPitch + difference);
            else
                CurrentPitch = float(CurrentPitch + (difference >= 0 ? rocket.TurnRate : -rocket.TurnRate));
            break;
        }
        case RocketState::Raise:
            remember_elite();
            if (!TrailerTimer.GetTimeLeft()) {
                animation("V3TAKOFF", foot->Location, 2, -10);
                TrailerTimer.Start(24);
            }
            if (MissionTimer.GetRatePassed() != 1.0) {
                auto at = foot->Location;
                at.Z += rocket.RaiseRate;
                if (MapClass::Instance.CoordinatesLegal(CellStruct{short(at.X / 256), short(at.Y / 256)}))
                    foot->SetLocation(at);
            } else {
                CurrentPitch = float(rocket.PitchFinal * Math::HalfPi);
                if (foot->GetLastFlightMapCoords() == CellStruct{-1, -1}) {
                    VocClass::PlayAt(foot->GetTechnoType()->AuxSound1, foot->Location);
                    AircraftTrackerClass::Instance.Add(foot);
                }
                TrailerTimer.Start(0);
                MissionState = RocketState::Climb;
            }
            break;
        default:
            break;
    }
    if (Is_Moving_Now() && !TrailerTimer.GetTimeLeft()) {
        animation("V3TRAIL", foot->Location, 0, 0);
        TrailerTimer.Start(3);
    }
    if (CurrentSpeed > 0) {
        const auto at = advance(foot, CurrentPitch, game::x87_integer(CurrentSpeed));
        if (MapClass::Instance.CoordinatesLegal(CellStruct{short(at.X / 256), short(at.Y / 256)}))
            foot->SetLocation(at);
        const auto cell = foot->GetMapCoords();
        if (cell != foot->LastFlightMapCoords && foot->IsAlive)
            AircraftTrackerClass::Instance.Update(foot, foot->LastFlightMapCoords, cell);
        if (foot->Health <= 0) Explode();
    }
    return Is_Moving();
}

Matrix3D YRPP_STDCALL RocketLocomotionClass::Draw_Matrix(VoxelIndexKey* key) {
    auto result = Matrix3D::GetIdentity();
    const unsigned facing = LinkedTo->PrimaryFacing.Current().GetValue<5>();
    result.RotateZ(matrix_store_float((int(facing) - 8) * -0.1963495408493621));
    if (CurrentPitch != 0) {
        result.RotateY(-CurrentPitch);
        const auto& rocket = rocket_for(LinkedTo);
        const float final = float(rocket.PitchFinal * Math::HalfPi);
        if (key) {
            if (rocket.PitchInitial * Math::HalfPi == CurrentPitch)
                key->Value |= 0x20;
            else if (CurrentPitch == final)
                key->Value |= 0x40;
            else
                key->Invalidate();
        }
    }
    if (key) key->Value |= int(facing);
    return result;
}
