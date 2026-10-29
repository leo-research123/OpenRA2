// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 bounce.cpp / voxel.cpp; YR 0x004397E0..0x0043A010.
// Copyright 2026 OpenTS contributors. See third_party/opents/LICENSE.md.
#include "yrpp/BounceClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/YRMath.h"
#include "Matrix3DArithmetic.hpp"
#include <bit>
#include <cmath>
#include <cstdint>

namespace {
int integer_component(double value) {
    // 0x7C5F00 returns an int64 conversion; coordinate callers keep EAX.
    if (!(value >= -0x1p63 && value < 0x1p63)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
CoordStruct integer_coords(const Vector3D<float>& p) {
    return {integer_component(p.X), integer_component(p.Y), integer_component(p.Z)};
}
bool bridge(const CellClass* cell) { return cell && (cell->Flags & CellFlags::Bridge) != CellFlags{}; }
int level(const CellClass* cell) { return cell ? static_cast<signed char>(cell->Level) : 0; }
Quaternion multiply(const Quaternion& a, const Quaternion& b) {
    // 0x645ED0 stores the intermediate cross products, then divides by
    // the squared magnitude (not its square root).
    const float yw = matrix_store_float(double(a.Y) * b.W), zw = matrix_store_float(double(a.Z) * b.W);
    const float wx = matrix_store_float(double(a.W) * b.X), wy = matrix_store_float(double(a.W) * b.Y),
                wz = matrix_store_float(double(a.W) * b.Z);
    const double cx = matrix_add(double(a.Y) * b.Z, -double(a.Z) * b.Y);
    const float cy = matrix_store_float(matrix_add(double(a.Z) * b.X, -double(b.Z) * a.X));
    const float cz = matrix_store_float(matrix_add(double(b.Y) * a.X, -double(a.Y) * b.X));
    Quaternion result{
        matrix_store_float(matrix_add(matrix_add(cx, wx), double(a.X) * b.W)),
        matrix_store_float(matrix_add(matrix_add(yw, wy), cy)), matrix_store_float(matrix_add(matrix_add(zw, wz), cz)),
        matrix_store_float(matrix_add(
            double(a.W) * b.W, -matrix_add(matrix_add(double(a.Z) * b.Z, double(a.Y) * b.Y), double(a.X) * b.X)))};
    const double norm = matrix_add(
        matrix_add(matrix_add(double(result.Z) * result.Z, double(result.Y) * result.Y), double(result.W) * result.W),
        double(result.X) * result.X);
    if (norm != 0.0) {
        result.X = matrix_store_float(matrix_divide(result.X, norm));
        result.Y = matrix_store_float(matrix_divide(result.Y, norm));
        result.Z = matrix_store_float(matrix_divide(result.Z, norm));
        result.W = matrix_store_float(matrix_divide(result.W, norm));
    }
    return result;
}
int cliff_direction(const CellClass& cell, const CellClass& previous) {
    // OpenTS CellClass::Bounce_Direction; YR table 0x0081CD4C.
    const int dx = cell.MapCoords.X - previous.MapCoords.X, dy = cell.MapCoords.Y - previous.MapCoords.Y;
    if (dx < -1 || dx > 1 || dy < -1 || dy > 1) return -1;
    constexpr int directions[] = {7, 0, 1, 6, -1, 2, 5, 4, 3};
    int direction = directions[4 + dx + 3 * dy];
    if (dx && dy) {
        auto& map = MapClass::Instance;
        const bool left = level(map.TryGetCellAt(CellStruct{short(previous.MapCoords.X + (dx == dy ? dx : dy)),
                                                            previous.MapCoords.Y})) >= level(&previous) + 2;
        const bool right = level(map.TryGetCellAt(
                               CellStruct{previous.MapCoords.X, short(previous.MapCoords.Y + (dx == dy ? dy : dx))})) >=
                           level(&previous) + 2;
        if (left && !right)
            direction = (direction - 1) & 7;
        else if (!left && right)
            ++direction;
    }
    return direction;
}
}

void BounceClass::Initialize(const CoordStruct& coords, double elasticity, double gravity, double maxVelocity,
                             const Vector3D<float>& velocity, double angularVelocity) {
    Elasticity = elasticity;
    Gravity = gravity;
    MaxVelocity = maxVelocity;
    Coords = {float(coords.X), float(coords.Y), float(coords.Z)};
    Velocity = velocity;
    auto& random = ScenarioClass::Instance->Random;
    const int z = random.RandomRanged(-65535, 65535), y = random.RandomRanged(-65535, 65535),
              x = random.RandomRanged(-65535, 65535);
    constexpr float scale = 1.0f / 65535.0f;
    Vector3D<float> axis{matrix_store_float(double(x) * scale), matrix_store_float(double(y) * scale),
                         matrix_store_float(double(z) * scale)};
    const double length =
        Math::sqrt(matrix_add(matrix_add(double(axis.Y) * axis.Y, double(axis.Z) * axis.Z), double(axis.X) * axis.X));
    if (length != 0.0) {
        axis.X = matrix_store_float(matrix_divide(axis.X, length));
        axis.Y = matrix_store_float(matrix_divide(axis.Y, length));
        axis.Z = matrix_store_float(matrix_divide(axis.Z, length));
    }
    Quaternion::FromAxis(&AngularVelocity, axis, matrix_store_float(angularVelocity));
    CurrentAngle.X = CurrentAngle.Y = CurrentAngle.Z = 0;
    CurrentAngle.W = 1;
}
CoordStruct* BounceClass::GetCoords(CoordStruct* output) const {
    *output = integer_coords(Coords);
    return output;
}
Matrix3D* BounceClass::GetDrawingMatrix(Matrix3D* output) const {
    return Matrix3D::FromQuaternion(output, &CurrentAngle);
}
BounceClass::Status BounceClass::Update() {
    auto& map = MapClass::Instance;
    const auto oldPosition = Coords, oldVelocity = Velocity;
    Velocity.Z = float(Velocity.Z - Gravity);
    // YR 0x00439B9A and OpenTS both scale by speed/speed when over
    // MaxVelocity. Preserve that no-op rather than inventing a speed clamp.
    const auto previous = integer_coords(Coords);
    Coords += Velocity;
    auto current = integer_coords(Coords);
    auto* cell = map.TryGetCellAt(current);
    auto* previousCell = map.TryGetCellAt(previous);
    if (!cell) return Status::Impact;
    const int floor = map.GetCellFloorHeight(current), deck = floor + CellClass::BridgeHeight;
    const bool crossedBridge = bridge(cell) || bridge(previousCell);
    const bool top = crossedBridge && current.Z < deck && previous.Z >= deck;
    const bool bottom = crossedBridge && current.Z >= deck && previous.Z < deck;
    bool obstacle = false;
    if (!top && !bottom && floor <= Coords.Z && Coords.Z - 150.0 < floor) {
        auto* building = cell->GetBuilding();
        obstacle = building || cell->ConnectsToOverlay();
        if (building && ((building->Type->LaserFence && building->LaserFenceFrame >= 8) || building->IsStrange()))
            obstacle = false;
    }
    auto result = Status::None;
    if (Coords.Z < floor || top || bottom || obstacle) {
        if (top)
            Coords.Z = float(deck);
        else if (bottom)
            Coords.Z = float(deck - 20);
        else if (floor - 100 < Coords.Z)
            Coords.Z = float(floor);
        const auto& slope = Matrix3D::VoxelRampMatrix[cell->SlopeIndex];
        const auto inverse = Matrix3D::TransposeMatrix(slope);
        auto velocity = inverse.RotateVector({Velocity.X, -Velocity.Y, Velocity.Z});
        const float elasticity = matrix_store_float(Elasticity);
        velocity.X = matrix_store_float(double(velocity.X) * elasticity);
        velocity.Y = matrix_store_float(double(velocity.Y) * elasticity);
        velocity.Z = -matrix_store_float(double(velocity.Z) * elasticity);
        velocity = slope.RotateVector(velocity);
        Velocity = {velocity.X, -velocity.Y, velocity.Z};
        AngularVelocity.X = -AngularVelocity.X;
        AngularVelocity.Y = -AngularVelocity.Y;
        AngularVelocity.Z = -AngularVelocity.Z;
        if (previousCell && level(cell) - level(previousCell) >= 2 &&
            ((oldVelocity.Z < -0.0002 && Coords.Z > oldPosition.Z) ||
             (oldVelocity.Z >= -0.0003 && oldVelocity.Z + oldPosition.Z + 1.0 < Coords.Z))) {
            Coords = oldPosition;
            Velocity = oldVelocity;
            const int facing = cliff_direction(*cell, *previousCell);
            if (facing != -1) {
                const float x = Velocity.X, y = Velocity.Y;
                if (facing == 8) {
                    // 0x755C60 subtracts four only once. Direction eight reads the
                    // adjacent bridge height, camera scale and first ramp matrix.
                    // Preserve the initialized bytes without an out-of-bounds C++ read.
                    constexpr float height = std::bit_cast<float>(0x000001A0u);
                    constexpr float scaleLow = std::bit_cast<float>(0x11111111u);
                    constexpr float scaleHigh = std::bit_cast<float>(0x401822CBu);
                    Velocity.X = matrix_store_float(matrix_add(
                        matrix_add(matrix_add(double(Velocity.Z) * scaleLow, double(y) * 0.0), double(x) * height),
                        scaleHigh));
                    Velocity.Y = x;
                    Velocity.Z = y;
                } else
                    switch (facing & 3) {
                        case 0:
                            Velocity.Y = -y;
                            break;
                        case 1:
                            Velocity.X = y;
                            Velocity.Y = x;
                            break;
                        case 2:
                            Velocity.X = -x;
                            break;
                        case 3:
                            Velocity.X = -y;
                            Velocity.Y = -x;
                            break;
                    }
                Velocity.X = float(Velocity.X * Elasticity);
                Velocity.Y = float(Velocity.Y * Elasticity);
                Velocity.Z = float(Velocity.Z * Elasticity);
            }
        }
        result = Status::Bounce;
    }
    auto angle = multiply(CurrentAngle, AngularVelocity);
    CurrentAngle = angle;
    current = integer_coords(Coords);
    cell = map.TryGetCellAt(current);
    int height = int(Coords.Z - map.GetCellFloorHeight(current));
    if (bridge(cell) && height >= CellClass::BridgeHeight) height -= CellClass::BridgeHeight;
    // 0x00439A10 converts the residual vector to integer leptons first.
    const double x = int(Velocity.X), y = int(Velocity.Y), z = int(height * Gravity + Velocity.Z);
    if (int(std::sqrt(x * x + y * y + z * z)) < 2.5) result = Status::Impact;
    return result;
}
