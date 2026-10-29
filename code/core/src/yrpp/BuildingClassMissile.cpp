// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Do_MISSION_MISSILE; calibrated to YR 0x0044C980.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/VocClass.h"
#include "projectile_ballistics.hpp"
#include <algorithm>
#if !defined(RA2_YRPP_GAME)
namespace {
AnimClass* animation(AnimTypeClass* type, const CoordStruct& where) {
    auto* memory = YRMemory::Allocate(sizeof(AnimClass));
    return memory ? ::new (memory) AnimClass(type, where, 0, 1, 0x600, 0, false) : nullptr;
}
short direction(double angle) {
    return std::bit_cast<short>(
        static_cast<unsigned short>(game::x87_integer((angle - 1.570796326794897) * -10430.06004058427)));
}
double radians(short value) { return (int(value) - 0x3FFF) * -0.00009587672516830327; }
int difference(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
void recoil(RecoilData& value) {
    if (!value.Turret.Travel) return;
    value.State = RecoilData::RecoilState::Compressing;
    value.TravelFramesLeft = std::max(value.Turret.CompressFrames, 1);
    value.TravelPerFrame = float(double(value.Turret.Travel) / value.TravelFramesLeft);
}
}
int BuildingClass::Mission_Missile() {
    enum NukeState : int { Initial = 0, DoorOpening = 1, LaunchUp = 2, LaunchDown = 3, DoneLaunch = 4 };
    enum EMPState : int { Aim = 0, Charge = 1, Firing = 2, Done = 3 };
    const auto finish = [&] {
        BeginMode(BStateType::Idle);
        QueueMission(Mission::Guard, false);
        return 60;
    };
    if (Type->NukeSilo && MissionStatus >= NukeState::Initial && MissionStatus <= NukeState::DoneLaunch) {
        switch (MissionStatus) {
            case NukeState::Initial: {
                IsReadyToCommence = false;
                BeginMode(BStateType::Active);
                MissionStatus = NukeState::DoorOpening;
                auto* cell = MapClass::Instance.GetCellAt(Owner->NukeTarget);
                auto* type = AnimTypeClass::Find("PSIWARN");
                const auto where = cell->GetCoords();
                auto* warning = animation(type, where);
                if (warning) {
                    warning->AttachedBullet = nullptr;
                    warning->Owner = Owner;
                    warning->Invisible = true;
                }
                PsiWarnAnim = warning;
                [[fallthrough]];
            }
            case NukeState::DoorOpening:
                if (IsReadyToCommence) {
                    BeginMode(BStateType::Aux1);
                    MissionStatus = NukeState::LaunchUp;
                }
                [[fallthrough]];
            case NukeState::LaunchUp: {
                GetCoords();
                auto* cell = MapClass::Instance.GetCellAt(Owner->NukeTarget);
                auto* weapon = SuperWeaponTypeClass::Array[FiringSWType]->WeaponType;
                auto* bullet = weapon->Projectile->CreateBullet(cell, this, weapon->Damage, weapon->Warhead, 255, true);
                if (!bullet) return 1;
                bullet->SetWeaponType(weapon);
                if (PsiWarnAnim) {
                    PsiWarnAnim->AttachedBullet = bullet;
                    PsiWarnAnim = nullptr;
                }
                bullet->Limbo();
                CoordStruct launch;
                GetFLH(&launch, 0, CoordStruct::Empty);
                constexpr double angle = 1.570748388432313;
                const double sine = Math::sin(angle);
                const double y = Math::cos(angle) * sine * 10.0;
                const double cosine = Math::cos(angle);
                const double x = Math::cos(angle) * cosine * 10.0;
                const BulletVelocity velocity{x, y, Math::sin(angle) * 10.0};
                if (bullet->MoveTo(launch, velocity)) {
                    if (auto* puff = animation(RulesClass::Instance->NukeTakeOff, launch)) puff->ZAdjust = -100;
                } else
                    delete bullet;
                MissionStatus = NukeState::LaunchDown;
                return 1;
            }
            case NukeState::LaunchDown:
                BeginMode(BStateType::Aux2);
                MissionStatus = NukeState::DoneLaunch;
                return 6;
            case NukeState::DoneLaunch:
                return finish();
        }
    }
    if (FiringSWType != -1) {
        Fire(MapClass::Instance.GetCellAt(Owner->NukeTarget), 0);
        QueueMission(Mission::Guard, false);
    }
    if (Type->EMPulseCannon) {
        switch (MissionStatus) {
            case EMPState::Aim: {
                const auto yaw = FireAngleTo(MapClass::Instance.GetCellAt(Owner->EMPTarget));
                DirStruct pitch;
                vt_entry_2C8(&pitch, MapClass::Instance.GetCellAt(Owner->EMPTarget));
                if (PrimaryFacing.Current().Raw != yaw.Raw)
                    PrimaryFacing.SetDesired(yaw);
                else if (BarrelFacing.Current().Raw != pitch.Raw)
                    BarrelFacing.SetDesired(pitch);
                else
                    MissionStatus = EMPState::Charge;
                return 1;
            }
            case EMPState::Charge: {
                if (auto* memory = YRMemory::Allocate(sizeof(AnimClass))) {
                    auto* type = AnimTypeClass::Find("PULSBALL");
                    CoordStruct launch;
                    GetFLH(&launch, 0, CoordStruct::Empty);
                    ::new (memory) AnimClass(type, launch, 0, 1, 0x600, 0, false);
                }
                MissionStatus = EMPState::Firing;
                return 32;
            }
            case EMPState::Firing: {
                auto* weapon = GetWeapon(0)->WeaponType;
                auto* target = MapClass::Instance.GetCellAt(Owner->EMPTarget);
                auto* target2 = MapClass::Instance.GetCellAt(Owner->EMPTarget);
                if (Unsorted::ArmageddonMode || Owner->EMPTarget == CellStruct::Empty) return finish();
                DirStruct pitch;
                vt_entry_2C8(&pitch, target);
                BarrelFacing.SetDesired(pitch);
                CoordStruct launch;
                GetFLH(&launch, 0, CoordStruct::Empty);
                const CoordStruct at{int(Owner->EMPTarget.X) * 256 + 128, int(Owner->EMPTarget.Y) * 256 + 128, 0};
                MapClass::Instance.GetCellFloorHeight(at);
                const short yaw = direction(Math::atan2(double(launch.Y) - at.Y, double(at.X) - launch.X));
                const int speed =
                    game::x87_integer(Math::sqrt(double(DistanceFrom(target)) * RulesClass::Instance->Gravity * 1.2));
                auto* bullet =
                    weapon->Projectile->CreateBullet(target, this, weapon->Damage, weapon->Warhead,
                                                     std::bit_cast<int>(unsigned(speed) * 3u) / 4, weapon->Bright);
                if (!bullet) return finish();
                const double initial = Math::sqrt(10000.0);
                BulletVelocity velocity{Math::cos(radians(yaw)) * initial, -Math::sin(radians(yaw)) * initial, 0.0};
                if (velocity.X == 0.0 && velocity.Y == 0.0 && velocity.Z == 0.0) velocity.X = 100.0;
                const double scale = double(speed) / Math::sqrt(velocity.X * velocity.X + velocity.Z * velocity.Z +
                                                                velocity.Y * velocity.Y);
                velocity.X *= scale;
                velocity.Y *= scale;
                velocity.Z *= scale;
                vt_entry_300(&launch, 0);
                const auto end = target2->GetCoords();
                const int dx = difference(end.X, launch.X), dy = difference(end.Y, launch.Y),
                          dz = difference(end.Z, launch.Z);
                const int squared = std::bit_cast<int>(unsigned(dx) * unsigned(dx) + unsigned(dy) * unsigned(dy));
                short elevation;
                if (!game::projectile_pitch(ShouldUseHighArc(0), speed, game::x87_integer(Math::sqrt(squared)), dz,
                                            RulesClass::Instance->Gravity, elevation) &&
                    !game::projectile_pitch(ShouldUseHighArc(0), std::bit_cast<int>(unsigned(speed) * 10u) / 8,
                                            game::x87_integer(Math::sqrt(squared)), dz, RulesClass::Instance->Gravity,
                                            elevation))
                    elevation = std::bit_cast<short>(WORD(0xFA00));
                const double old = radians(
                    direction(Math::atan2(velocity.Z, Math::sqrt(velocity.Y * velocity.Y + velocity.X * velocity.X))));
                const double magnitude =
                    Math::sqrt(velocity.Y * velocity.Y + velocity.Z * velocity.Z + velocity.X * velocity.X);
                if (old != 0.0) {
                    velocity.X /= Math::cos(old);
                    velocity.Y /= Math::cos(old);
                }
                velocity.X *= Math::cos(radians(elevation));
                velocity.Y *= Math::cos(radians(elevation));
                velocity.Z = Math::sin(radians(elevation)) * magnitude;
                if (!bullet->MoveTo(launch, velocity)) {
                    bullet->Release();
                    return finish();
                }
                if (HasTurret() && GetTechnoType()->TurretRecoil) {
                    recoil(TurretRecoil);
                    recoil(BarrelRecoil);
                }
                if (!GetTechnoType()->IsGattling && weapon->Report.Count) {
                    const int sound =
                        weapon->Report[unsigned(Randomizer::Global.Random()) % unsigned(weapon->Report.Count)];
                    GetFLH(&launch, 0, CoordStruct::Empty);
                    VocClass::PlayAt(sound, launch);
                }
                MissionStatus = EMPState::Done;
                return 1;
            }
            case EMPState::Done:
                BarrelFacing.SetDesired(DirStruct(short(0x4000)));
                return finish();
        }
    }
    return game::x87_scaled_quotient_to_integer(CurrentMissionControl()->Rate, 900.0, 1);
}
#endif
