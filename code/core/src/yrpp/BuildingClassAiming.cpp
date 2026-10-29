// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Barrel_Pitch; YR 0x0043E940.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "projectile_ballistics.hpp"
#if !defined(RA2_YRPP_GAME)
namespace {
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
int sub(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
int square_sum(int x, int y) { return std::bit_cast<int>(unsigned(x) * unsigned(x) + unsigned(y) * unsigned(y)); }
}
DirStruct* BuildingClass::vt_entry_2C8(DirStruct* output, AbstractClass* target) const {
    if (GetWeapon(0)->WeaponType->IsLaser) {
        GetTechnoType();
        if (!target) {
            *output = BarrelFacing.Current();
            return output;
        }
        CoordStruct predicted;
        PredictTargetCoords(&predicted);
        const int muzzle_z = GetWeapon(0)->FLH.Z;
        auto origin = GetRenderCoords();
        origin.Z = add(origin.Z, muzzle_z);
        const auto pixel = TacticalClass::Instance->ApplyMatrix_Pixel(Type->BuildingAnim[9].Position);
        const int x = sub(predicted.X, add(origin.X, pixel.X));
        const int y = sub(predicted.Y, add(origin.Y, pixel.Y));
        const int z = sub(predicted.Z, origin.Z);
        if (!z) {
            output->Raw = 0x4000;
            return output;
        }
        const double planar = Math::sqrt(double(square_sum(x, y)));
        const int distance = game::x87_integer(Math::sqrt(double(x) * x + double(z) * z + double(y) * y));
        const double ratio = planar / distance;
        const double radians = Math::asin(ratio > 1.0 ? 1.0 : ratio);
        const auto pitch = static_cast<WORD>(game::x87_integer(radians * 16384.0 * 0.6366197723675814));
        output->Raw = z < 0 ? WORD(0x8000u - pitch) : pitch;
        return output;
    }
    if (Type->EMPulseCannon) {
        MapClass::Instance.GetCellFloorHeight(target->GetCoords());
        const auto at = target->GetCoords();
        CoordStruct fire;
        GetFLH(&fire, 0, CoordStruct::Empty);
        Math::atan2(double(fire.Y) - at.Y, double(at.X) - fire.X);
        const int speed =
            game::x87_integer(Math::sqrt(double(DistanceFrom(target)) * RulesClass::Instance->Gravity * 1.2));
        CoordStruct origin;
        vt_entry_300(&origin, 0);
        const auto end = target->GetCoords();
        const int x = sub(end.X, origin.X), y = sub(end.Y, origin.Y), z = sub(end.Z, origin.Z);
        const double planar = Math::sqrt(double(square_sum(x, y)));
        short pitch = 0;
        if (!game::projectile_pitch(ShouldUseHighArc(0), speed, game::x87_integer(planar), z,
                                    RulesClass::Instance->Gravity, pitch))
            game::projectile_pitch(ShouldUseHighArc(0), std::bit_cast<int>(unsigned(speed) * 10u) / 8,
                                   game::x87_integer(planar), z, RulesClass::Instance->Gravity, pitch);
        output->Raw = static_cast<WORD>(pitch);
        return output;
    }
    return TechnoClass::vt_entry_2C8(output, target);
}
#endif
