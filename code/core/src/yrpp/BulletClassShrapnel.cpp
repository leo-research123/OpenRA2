// YR 0x46A310: ordered fragment targets, fallback cells and fragment effects.
#include "yrpp/BulletClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>

namespace {
int difference(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
double horizontal_length(const BulletVelocity& velocity) {
    return Math::sqrt(velocity.X * velocity.X + velocity.Y * velocity.Y);
}
double length(const BulletVelocity& velocity) {
    return Math::sqrt(velocity.X * velocity.X + velocity.Y * velocity.Y + velocity.Z * velocity.Z);
}
double direction_angle(double angle) {
    const auto raw = static_cast<unsigned short>(game::x87_integer((angle - Math::HalfPi) * -10430.06004058427));
    return (int(std::bit_cast<short>(raw)) - 0x3FFF) * -0.00009587672516830327;
}
void pitch_fragment(BulletVelocity& velocity) {
    const double pitch = direction_angle(Math::atan2(velocity.Z, horizontal_length(velocity)));
    const double magnitude = length(velocity);
    if (pitch != 0.0) {
        velocity.X /= Math::cos(pitch);
        velocity.Y /= Math::cos(pitch);
    }
    constexpr double elevation = 0.7853262558535721;
    velocity.X *= Math::cos(elevation);
    velocity.Y *= Math::cos(elevation);
    velocity.Z = Math::sin(elevation) * magnitude;
}
void scale_fragment(BulletVelocity& velocity, int speed) {
    if (velocity.X == 0.0 && velocity.Y == 0.0 && velocity.Z == 0.0) velocity.X = 100.0;
    const double scale = double(speed) / length(velocity);
    velocity.X *= scale;
    velocity.Y *= scale;
    velocity.Z *= scale;
}
void launch_fragment(BulletClass& source, AbstractClass* target, WeaponTypeClass* weapon, BulletTypeClass* projectile,
                     const CellStruct* randomCell) {
    auto* bullet =
        projectile->CreateBullet(target, source.Owner, weapon->Damage, weapon->Warhead, weapon->Speed, weapon->Bright);
    if (!bullet) return;
    // The target and fallback arms deliberately use different operation order.
    const auto origin = source.Location;
    const auto targetCoords = (randomCell ? MapClass::Instance.GetCellAt(*randomCell) : target)->GetCoords();
    const double yaw =
        direction_angle(Math::atan2(double(origin.Y) - targetCoords.Y, double(targetCoords.X) - origin.X));
    BulletVelocity velocity{100.0, 0.0, 0.0};
    const double magnitude = horizontal_length(velocity);
    velocity.X = Math::cos(yaw) * magnitude;
    velocity.Y = -(Math::sin(yaw) * magnitude);
    if (randomCell) scale_fragment(velocity, weapon->Speed);
    pitch_fragment(velocity);
    if (!randomCell) scale_fragment(velocity, weapon->Speed);
    bullet->MoveTo(source.Location, velocity);
    // MoveTo's result does not gate the original fragment's visual effects.
    const auto at = source.Location;
    if (weapon->IsLaser && source.Owner)
        source.Owner->CreateLaser(
            reinterpret_cast<ObjectClass*>(randomCell ? MapClass::Instance.GetCellAt(*randomCell) : target), 0, weapon,
            at);
    if (weapon->IsElectricBolt && source.Owner)
        source.Owner->CreateElectricBolt(randomCell ? MapClass::Instance.GetCellAt(*randomCell) : target, weapon, at);
}
}

void BulletClass::Shrapnel() {
    int count = Type->ShrapnelCount;
    if (count < 0) {
        if (Owner) {
            const auto origin = Location;
            const auto ownerCoords = Owner->GetCoords();
            const int x = difference(ownerCoords.X, origin.X), y = difference(ownerCoords.Y, origin.Y),
                      z = difference(ownerCoords.Z, origin.Z);
            const int cells = game::x87_integer(Math::sqrt(double(x) * x + double(y) * y + double(z) * z)) / 256;
            count = std::bit_cast<int>(0u - unsigned(count) - unsigned(cells));
            if (count <= 0) return;
        } else
            count = 3;
    }
    if (!GetCell()->FirstObject || GetCell()->FirstObject->WhatAmI() == AbstractType::Building) return;
    auto* weapon = Type->ShrapnelWeapon;
    auto* projectile = weapon->Projectile;
    const auto center = GetMapCoords();
    const int range = weapon->Range / 256;
    int emitted = 0;
    for (int radius = 1; radius <= range; ++radius) {
        const auto end = CellSpread::NumCells(radius);
        for (auto index = CellSpread::NumCells(radius - 1); index < end; ++index) {
            const auto offset = CellSpread::GetCell(index);
            auto* target =
                MapClass::Instance.GetCellAt(CellStruct{short(center.X + offset.X), short(center.Y + offset.Y)})
                    ->FirstObject;
            if (!target || target == Owner || !Owner || Owner->Owner->IsAlliedWith(target)) continue;
            launch_fragment(*this, target, weapon, projectile, nullptr);
            // An unsuccessful construction still consumes the target quota.
            if (++emitted == count) return;
        }
    }
    auto& random = ScenarioClass::Instance->Random;
    for (int remaining = count - emitted; remaining > 0; --remaining) {
        auto cell = GetMapCoords();
        cell.X = short(cell.X + random.RandomRanged(-2, 2));
        cell.Y = short(cell.Y + random.RandomRanged(-2, 2));
        auto* target = MapClass::Instance.GetCellAt(cell);
        launch_fragment(*this, target, weapon, projectile, &cell);
    }
}
