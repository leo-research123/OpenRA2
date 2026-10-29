// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Rock / Rocking_AI; YR 0x70B280 / 0x70B570.
// YR adds direct impulse, sinking direction,
// balloon-crash limits, reciprocal rocker acceleration and overturn damage.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/FileFormats/VXL.h"
#include "Matrix3DArithmetic.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

void TechnoClass::vt_entry_3D8(const CoordStruct* impact, float amplitude, bool direct) {
    if (!GetTechnoType()->MainVoxel.VXL || GetTechnoType()->MainVoxel.VXL->Initialized) return;
    const auto difference = [](int a, int b) {
        return matrix_store_float(std::bit_cast<int>(unsigned(a) - unsigned(b)));
    };
    const float x = difference(Location.X, impact->X);
    const float y = difference(impact->Y, Location.Y);
    const float z = difference(Location.Z, impact->Z);
    const double heading = matrix_multiply(int(short(PrimaryFacing.Current().Raw)) - 0x3FFF, -0.00009587672516830327);
    const float sine = matrix_store_float(Math::sin(heading));
    const float cosine = matrix_store_float(Math::cos(heading));
    const double distance = Math::sqrt(matrix_add(matrix_add(double(x) * x, double(y) * y), double(z) * z));
    const float savedDistance = matrix_store_float(distance);
    const double force =
        matrix_multiply(matrix_add(double(0.04f), -matrix_multiply(distance, double(0.000025f))), amplitude);
    float scale = matrix_store_float(matrix_divide(force, GetTechnoType()->Weight));
    if (std::abs(double(savedDistance)) < 0.00002 || scale < 0.01f) return;
    if (scale > 0.05f) scale = 0.05f;

    const double planar = Math::sqrt(matrix_add(double(y) * y, double(x) * x));
    const float nx = planar == 0.0 ? x : matrix_store_float(matrix_divide(x, planar));
    const float ny = planar == 0.0 ? y : matrix_store_float(matrix_divide(y, planar));
    const float nz = planar == 0.0 ? 0.0f : matrix_store_float(matrix_divide(0.0, planar));
    const float forward = matrix_store_float(matrix_add(double(nx) * cosine, double(ny) * sine));
    const float crossX = matrix_store_float(-(double(nz) * sine));
    const float crossY = matrix_store_float(double(nz) * cosine);
    const double crossZ = matrix_add(double(nx) * sine, -double(ny) * cosine);
    double sideways = Math::sqrt(
        matrix_add(matrix_add(double(crossX) * crossX, double(crossY) * crossY), matrix_multiply(crossZ, crossZ)));
    // The reconstruction check selects the original sign, including lookup error.
    if (std::abs(matrix_add(matrix_add(double(cosine) * forward, -matrix_multiply(sine, sideways)), -nx)) > 0.0002 ||
        std::abs(matrix_add(matrix_add(matrix_multiply(cosine, sideways), double(sine) * forward), -ny)) > 0.0002)
        sideways = -sideways;
    RockingForwardsPerFrame = matrix_store_float(double(forward) * scale * (direct ? 1.0 : 0.5));
    RockingSidewaysPerFrame = matrix_store_float(-matrix_multiply(sideways, scale));
}

void TechnoClass::vt_entry_41C() {
    if (IsSinking) {
        if (std::abs(AngleRotatedForwards) < double(0.78539819f)) {
            const auto face = PrimaryFacing.Current().GetValue<3>();
            AngleRotatedForwards =
                matrix_store_float(matrix_add(AngleRotatedForwards, face && face <= 5 ? 0.01 : -0.01));
        }
        return;
    }
    if (IsCrashing) {
        AngleRotatedForwards = matrix_store_float(double(AngleRotatedForwards) + RockingForwardsPerFrame);
        AngleRotatedSideways = matrix_store_float(double(AngleRotatedSideways) + RockingSidewaysPerFrame);
        // 0x70B677 reads TechnoType + 0xD6A (BalloonHover).
        if (GetTechnoType()->BalloonHover) {
            AngleRotatedForwards = std::max(AngleRotatedForwards, -0.78539819f);
            AngleRotatedSideways = std::clamp(AngleRotatedSideways, -0.78539819f, 0.78539819f);
        }
        return;
    }
    const auto rock = [&](float& angle, float& velocity, float limit, bool sideways) {
        const bool positive = angle > 0.00002, negative = angle < -0.00002;
        const bool overturned = angle > 1.5707964f || angle < -1.5707964f;
        if (velocity == 0)
            angle = 0;
        else {
            const float old = angle;
            angle = matrix_store_float(double(angle) + velocity);
            if (!DirectRockerLinkedUnit && !overturned) {
                if (angle > limit && old < limit) {
                    angle = limit;
                    velocity = 0;
                }
                if (angle < -limit && old > -limit) {
                    angle = -limit;
                    velocity = 0;
                }
            }
            if (overturned)
                velocity = matrix_store_float(double(velocity) + (positive ? double(0.002f) : -double(0.002f)));
            else if (DirectRockerLinkedUnit) {
                // 0x70B800 reads Rules + 0x18B8 (FallBackCoefficient).
                const double amount = double(RulesClass::Instance->FallBackCoefficient) * double(0.002f);
                velocity = matrix_store_float(matrix_add(velocity, positive ? -amount : amount));
            } else {
                // 0x7F4E68 stores 2.5 * double(0.002f), not double(0.005f).
                const double amount =
                    double(0.002f) *
                    (sideways && ((positive && velocity > 0) || (!positive && velocity < 0)) ? 1.0 : 2.5);
                velocity = matrix_store_float(double(velocity) + (positive ? -amount : amount));
            }
        }
        if ((positive && angle < 0.00002) || (negative && angle > -0.00002)) {
            velocity = 0;
            angle = 0;
        }
    };
    rock(AngleRotatedSideways, RockingSidewaysPerFrame, 0.78539819f, true);
    const bool crush = (AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None &&
                       static_cast<FootClass*>(this)->IsCrushingSomething;
    rock(AngleRotatedForwards, RockingForwardsPerFrame, crush ? 0.31415927f : 0.78539819f, false);
    if (std::abs(AngleRotatedSideways) > double(3.1415927f) || std::abs(AngleRotatedForwards) > double(3.1415927f)) {
        int damage = GetTechnoType()->Strength;
        ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false, nullptr);
    }
}
