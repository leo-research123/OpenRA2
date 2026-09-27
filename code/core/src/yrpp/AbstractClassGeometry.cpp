// gamemd.exe 0x5F3DB0, 0x5F6360, 0x5F6440; see first-map-p1 evidence.
#include "yrpp/AbstractClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
int ftol(double value) {
    // 0x7C5F00 is the x87 64-bit conversion helper; callers consume low EAX.
    if (!std::isfinite(value) || value < -0x1p63 || value >= 0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
double delta(int left, int right) {
    return std::bit_cast<std::int32_t>(std::uint32_t(left) - std::uint32_t(right));
}
int distance(const AbstractClass& self, AbstractClass* target, bool spatial) {
    if (!target) return 0;
    const auto there = target->GetCoords();
    const auto here = self.GetCoords();
    const double x = delta(here.X, there.X), y = delta(here.Y, there.Y);
    const double z = spatial ? delta(here.Z, there.Z) : 0.0;
    int result = ftol(Math::sqrt(x*x + y*y + z*z));
    if (target->WhatAmI() == AbstractType::Building) {
        const auto* type = static_cast<BuildingClass*>(target)->Type;
        const int height = type->GetFoundationHeight(false);
        const int width = type->GetFoundationWidth();
        result = std::bit_cast<std::int32_t>(
            std::uint32_t(result) - 64u * std::uint32_t(width + height));
        if (result < 0) result = 0;
    }
    return result;
}
}
DirStruct* AbstractClass::GetTargetDirection(DirStruct* output, AbstractClass* target) const {
    const auto there = target->GetCoords();
    const auto here = GetCoords();
    const double angle = Math::atan2(double(here.Y) - double(there.Y), double(there.X) - double(here.X));
    const auto raw = std::uint32_t(ftol((angle - 1.5707963267948966) * -10430.060040584269));
    // The original copies target X's upper word into the direction padding.
    const std::uint32_t packed = (std::uint32_t(there.X) & 0xFFFF0000u) | (raw & 0xFFFFu);
    std::memcpy(output, &packed, sizeof(packed));
    return output;
}
int AbstractClass::DistanceFrom(AbstractClass* target) const { return distance(*this, target, false); }
int AbstractClass::DistanceFrom3D(AbstractClass* target) const { return distance(*this, target, true); }
