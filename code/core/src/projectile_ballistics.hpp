// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 combat.cpp, calibrated to YR 0x0048A8D0/0x0048A9D0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#pragma once
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>
namespace game {
inline bool projectile_angle(bool high, int speed, int distance, int height, double gravity, double& angle) {
    if (height > 0) {
        angle = 0.7853822499999998;
        return true;
    }
    const double v = speed, h = height, d = distance;
    const double k = v * v - h * gravity;
    const double discriminant = v * v * v * v - (v * v * h * gravity + v * v * h * gravity) - gravity * gravity * d * d;
    const double divisor = h * h / (d * d) + 1.0 + h * h / (d * d) + 1.0;
    if (discriminant < 0.0 || divisor == 0.0) return false;
    const double square = (high ? k - Math::sqrt(discriminant) : Math::sqrt(discriminant) + k) / divisor;
    if (square < 0.0) return false;
    angle = 1.570796326794897 - Math::asin(Math::sqrt(square) / v);
    return true;
}
inline bool projectile_pitch(bool high, int speed, int distance, int height, double gravity, short& pitch) {
    double angle;
    if (!projectile_angle(high, speed, distance, height, gravity, angle)) return false;
    if (!high) {
        double other = angle;
        projectile_angle(false, speed, std::bit_cast<int>(unsigned(distance) + 1u), height, gravity, other);
        if (other < angle) angle = -angle;
    }
    pitch = std::bit_cast<short>(
        static_cast<unsigned short>(x87_integer((angle - 1.570796326794897) * -10430.06004058427)));
    return true;
}
}
