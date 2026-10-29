// YR 0x761640 / 0x762070: original world-space quadrilateral and projected points.
#include "yrpp/WaveClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Matrix3D.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

void WaveClass::Draw_Magnetic(const CoordStruct& from, const CoordStruct& to) { PrepareGeometry(from, to, true); }
void WaveClass::Draw_NonMagnetic(const CoordStruct& from, const CoordStruct& to) { PrepareGeometry(from, to, false); }
void WaveClass::PrepareGeometry(const CoordStruct& from, const CoordStruct& to, bool magnetic) {
    constexpr float scales[]{1.0f, 1.05f, 1.05f, 1.0f};
    const double scale = scales[static_cast<int>(Type)];
    const auto blend = [&](int a, int b) { return game::x87_integer(double(a) * scale + double(b) * (1 - scale)); };
    const CoordStruct end{blend(to.X, from.X), blend(to.Y, from.Y),
                          blend(to.Z, from.Z) + ((Type == WaveType::Sonic || Type == WaveType::Magnetron) ? 50 : 0)};
    const CoordStruct start{blend(from.X, to.X), blend(from.Y, to.Y), blend(from.Z, to.Z)};
    const double dx = double(start.X) - end.X, dy = double(start.Y) - end.Y, dz = double(start.Z) - end.Z;
    const double horizontal = Math::sqrt(dy * dy + dx * dx);
    const int length =
        magnetic ? game::x87_integer(Math::sqrt(dy * dy + dz * dz + dx * dx)) : game::x87_integer(horizontal);
    LimboCoords = start;
    SetLocation(LimboCoords);
    Pos0 = end;
    PointData_Counter = magnetic ? 4 : 6;
    PointData_Pointer = magnetic ? MagPoints : SonicPoints;
    TacticalClass::Instance->CoordsToClient(&start, &WaveStartMiddle);
    TacticalClass::Instance->CoordsToClient(&end, &WaveEndMiddle);
    if (!magnetic) {
        SonicPoints[1] = WaveEndMiddle;
        SonicPoints[4] = WaveStartMiddle;
    }
    constexpr float widths[]{100, 44, 34, 50}, extensions[]{30, 34, 27, 30};
    const int type = static_cast<int>(Type);
    const float extension = magnetic && Type == WaveType::Magnetron ? 0 : extensions[type];
    const int flat = magnetic ? game::x87_integer(float(horizontal)) : length;
    Vector3D<float> points[4]{{float(flat) - extension, -widths[type], 0},
                              {float(flat) - extension, widths[type], 0},
                              {extension, -widths[type], 0},
                              {extension, widths[type], 0}};
    for (auto& point : points)
        point.Z = float(double(point.X) * (end.Z - start.Z) / length + start.Z);
    const float x = float(end.X - start.X);
    const double distance = Math::sqrt(double(end.Y - start.Y) * (end.Y - start.Y) + double(x) * x);
    double angle = Math::acos(std::clamp(double(end.X - start.X), -distance, distance) / distance);
    if (start.Y > end.Y) angle = -angle;
    Matrix3D rotation;
    rotation.MakeIdentity();
    rotation.RotateZ(float(angle));
    CoordStruct* world[]{&WaveEndSide1Coord, &WaveEndSide2Coord, &WaveStartSide1Coord, &WaveStartSide2Coord};
    Point2D* screen[]{&WaveEndSide1, &WaveEndSide2, &WaveStartSide1, &WaveStartSide2};
    for (int i = 0; i < 4; ++i) {
        const auto point = rotation * points[i];
        *world[i] = {game::x87_integer(double(start.X) + point.X), game::x87_integer(double(float(start.Y)) + point.Y),
                     game::x87_integer(point.Z)};
        TacticalClass::Instance->CoordsToClient(world[i], screen[i]);
    }
    if (magnetic) {
        MagPoints[0] = WaveEndSide1;
        MagPoints[1] = WaveEndSide2;
        MagPoints[2] = WaveStartSide2;
        MagPoints[3] = WaveStartSide1;
    } else {
        SonicPoints[0] = WaveEndSide1;
        SonicPoints[2] = WaveEndSide2;
        SonicPoints[3] = WaveStartSide2;
        SonicPoints[5] = WaveStartSide1;
    }
    std::memcpy(&Facing, &Owner->SecondaryFacing, sizeof(Facing));
}
