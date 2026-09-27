// Existing YRpp 9402d7da TacticalClass/AbstractClass object model, calibrated
// against fixed gamemd 7b8a0685 6D1C20/6D1E80 and the existing Matrix3D
// operations at 5AEF60/5AF1A0/5AEA10; compat supplies YR lookup-table trig values.
#include "yrpp/TacticalClass.h"
#include <bit>
#include <cmath>

namespace {
float matrix_float(double value) noexcept {
    // YR's table lookup calls _ftol, leaving x87 in round-toward-zero mode
    // (0xE7F). Reproduce each FSTP float without changing the host's FP state.
    const float nearest = static_cast<float>(value);
    return std::abs(static_cast<double>(nearest)) > std::abs(value)
        ? std::bit_cast<float>(std::bit_cast<std::uint32_t>(nearest) - 1u) : nearest;
}
}

TacticalClass::TacticalClass(const Point2D& point, const RectangleStruct& bounds,
    float sx, float cx, float sz, float cz, float scale)
    : AbstractClass(), ScreenText{}, EndGameGraphicsFrame(-1), LastAIFrame(-1),
      field_AC(false), field_AD(false), TacticalPos{}, LastTacticalPos{},
      ZoomInFactor(1.0), Point_C8(point), Point_D0(point), field_D8(0), field_DC(0),
      VisibleCellCount(0), VisibleCells{}, TacticalCoord1{}, field_D6C(0), field_D70(0),
      TacticalCoord2{}, field_D7C(true), Redrawing(false), ContainingMapCoords(bounds),
      Band{}, MouseFrameIndex(0), StartTime(0), SelectableCount(0), field_E14(0) {
    // The original leaves padding and unused ScreenText elements unspecified;
    // core gives fresh objects deterministic state without copying object bytes.
    gap_AE[0] = gap_AE[1] = 0;
    gap_D7E[0] = gap_D7E[1] = 0;
    Unused_Matrix3D.MakeIdentity();
    for (auto& row : Unused_Matrix3D.row) {
        const double y = row[1], z = row[2];
        row[1] = matrix_float(y * cx + z * sx);
        row[2] = matrix_float(z * cx - y * sx);
    }
    for (auto& row : Unused_Matrix3D.row) {
        const double x = row[0], y = row[1];
        row[0] = matrix_float(x * cz + y * sz);
        row[1] = matrix_float(y * cz - x * sz);
    }
    for (auto& row : Unused_Matrix3D.row)
        for (int column = 0; column < 3; ++column)
            row[column] = matrix_float(static_cast<double>(row[column]) * scale);
    // Preserve YR's calibrated isometric matrix bit patterns (not rounded text).
    const auto x = std::bit_cast<float>(0x408888ceu);
    const auto y = std::bit_cast<float>(0x410888ceu);
    IsoTransformMatrix = Matrix3D(x, y, 0, 0, -x, y, 0, 0, 0, 0, 1, 0);
    Instance = this;
}
TacticalClass::~TacticalClass() { Instance = nullptr; }
AbstractType TacticalClass::WhatAmI() const { return AbstractType::TacticalMap; }
int TacticalClass::Size() const { return sizeof(TacticalClass); }
