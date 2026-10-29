// YR 0x760F50 / 0x762AF0 / 0x7610F0: propagation, expiration and occupied cells.
#include "yrpp/WaveClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include "x87_integer.hpp"
#include <cstdlib>

namespace {
constexpr double step = double(0.05f);
int interpolate(int end, int start, double scale) {
    return game::x87_integer(double(end) * scale + double(start) * (1 - scale));
}
Point2D interpolate(const Point2D& end, const Point2D& start, double scale) {
    return {interpolate(end.X, start.X, scale), interpolate(end.Y, start.Y, scale)};
}
CoordStruct interpolate(const CoordStruct& end, const CoordStruct& start, double scale) {
    // The EXE evaluates Z, Y, X, each via its x87 conversion helper.
    const int z = interpolate(end.Z, start.Z, scale), y = interpolate(end.Y, start.Y, scale);
    return {interpolate(end.X, start.X, scale), y, z};
}
CellStruct cell(const CoordStruct& at) { return {short(at.X / 256), short(at.Y / 256)}; }
}
void WaveClass::Update() {
    switch (Type) {
        case WaveType::Sonic:
            Update_Wave();
            for (int i = 0; i < Cells.Count; ++i) {
                const auto at = Cells[i]->GetCoords();
                DamageArea(at);
            }
            if (--WaveEC < 0)
                UnInit();
            else
                ObjectClass::Update();
            break;
        case WaveType::Magnetron:
            Update_Wave();
            if (--WaveEC < 0)
                UnInit();
            else
                ObjectClass::Update();
            break;
        case WaveType::BigLaser:
        case WaveType::Laser:
            LaserEC -= 6;
            if (LaserEC < 32) UnInit();
            break;
    }
}
void WaveClass::Update_Wave() {
    if (MatrixScale1 - MatrixScale2 > 0.1 && Type == WaveType::Sonic) ShouldEnd = true;
    const int threshold = game::x87_integer(1.0 / step);
    if (WaveEC == threshold && Type == WaveType::Magnetron) {
        WaveEC = 64;
        ++WaveCount;
    }
    bool end = !Target || !Owner || WaveEC == threshold || Owner->Target != Target;
    if (!end && Type != WaveType::Magnetron) {
        const auto at = Target->GetCenterCoords();
        const double x = double(Owner->Location.X) - at.X, y = double(Owner->Location.Y) - at.Y,
                     z = double(Owner->Location.Z) - at.Z;
        end = Math::sqrt(131072.0) * 6 < game::x87_integer(Math::sqrt(x * x + z * z + y * y));
    }
    if (end) {
        IsTraveling = false;
        ShouldEnd = true;
    }
    if (IsTraveling) {
        CoordStruct first, last;
        if (Type == WaveType::Magnetron && Target->WhatAmI() == AbstractType::Unit) {
            Owner->GetFLH(&last, 0, CoordStruct(0, 0, 0));
            Target->GetCenterCoords(&first);
        } else {
            Target->GetCenterCoords(&last);
            Owner->GetFLH(&first, 0, CoordStruct(0, 0, 0));
        }
        if (Type == WaveType::Magnetron)
            Draw_Magnetic(first, last);
        else
            Draw_NonMagnetic(first, last);
    }
    const bool magnetic = Type == WaveType::Magnetron;
    if (MatrixScale1 < 1 && IsTraveling) {
        MatrixScale1 += step;
        if (MatrixScale1 > 0.98) MatrixScale1 = 1;
        PointData_Pointer[0] = interpolate(WaveEndSide1, WaveStartSide1, MatrixScale1);
        if (magnetic)
            PointData_Pointer[1] = interpolate(WaveEndSide2, WaveStartSide2, MatrixScale1);
        else {
            PointData_Pointer[1] = interpolate(WaveEndMiddle, WaveStartMiddle, MatrixScale1);
            PointData_Pointer[2] = interpolate(WaveEndSide2, WaveStartSide2, MatrixScale1);
        }
    }
    if (ShouldEnd && MatrixScale1 + step * 0.5 >= MatrixScale2) {
        MatrixScale2 += step;
        if (MatrixScale2 >= MatrixScale1) {
            UnInit();
            return;
        }
        if (magnetic) {
            PointData_Pointer[2] = interpolate(WaveEndSide2, WaveStartSide2, MatrixScale2);
            PointData_Pointer[3] = interpolate(WaveEndSide1, WaveStartSide1, MatrixScale2);
        } else {
            PointData_Pointer[3] = interpolate(WaveEndSide2, WaveStartSide2, MatrixScale2);
            PointData_Pointer[4] = interpolate(WaveEndMiddle, WaveStartMiddle, MatrixScale2);
            PointData_Pointer[5] = interpolate(WaveEndSide1, WaveStartSide1, MatrixScale2);
        }
    }
    UpdateCells();
}
void WaveClass::UpdateCells() {
    try {
        // Clear frees the backing array in the original vector, not just Count.
        Cells.Clear();
        const auto add = [&](const CellStruct& at) {
            auto* item = MapClass::Instance.GetCellAt(at);
            if (Cells.FindItemIndex(item) < 0) Cells.AddItem(item);
        };
        auto center = cell(IsTraveling ? LimboCoords : Pos0);
        auto side1 = cell(WaveEndSide1Coord), side2 = cell(WaveEndSide2Coord);
        for (double scale = IsTraveling ? step : MatrixScale2; scale <= MatrixScale1 + step * 0.5; scale += step) {
            if (!IsTraveling && FacingIndex < 4) {
                const auto at = cell(interpolate(WaveEndSide1Coord, WaveStartSide1Coord, scale));
                if (at != side1) {
                    side1 = at;
                    add(at);
                }
            }
            const auto at = cell(interpolate(Pos0, LimboCoords, scale));
            if (at != center) {
                center = at;
                add(at);
            }
            if (!IsTraveling && FacingIndex >= 4) {
                const auto at2 = cell(interpolate(WaveEndSide2Coord, WaveStartSide2Coord, float(scale)));
                if (at2 != side2) {
                    side2 = at2;
                    add(at2);
                }
            }
        }
    } catch (...) {
        std::abort();
    }
}
