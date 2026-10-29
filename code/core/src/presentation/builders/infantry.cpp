// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 infantry.cpp::Draw_It; calibrated to YR 0x00518F90. The airborne
// shadow is INFSHDW frame 1 in YR, not another copy of the infantry SHP.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "presentation/builders/infantry.hpp"
#include "presentation/builders/techno_common.hpp"
#include "selectable_registration.hpp"
#include "yrpp/InfantryClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "techno_drawing.hpp"
#include <bit>
namespace game {
void build_infantry(PresentationContext& ctx, const InfantryClass& self, Point2D* location, RectangleStruct* bounds) {
    auto* d = ctx.techno;
    if (!d) return;
    if (!location || !bounds || !self.Type) {
        record_techno_drawing(*d, DrawingStatus::invalid_argument);
        return;
    }
    auto point = *location;
    auto* cell = techno_drawing_cell(self);
    if (!cell) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    unsigned tint = self.Berzerk ? d->berserk_tint : 0;
    if (techno_cell_shrouded(*cell)) tint = 0;
    if (self.TubeIndex != -1) return;
    if (!self.Locomotor) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    bool airborne = false;
    const auto locomotor_status = infantry_drop_pod(self, airborne);
    if (!techno_drawing_complete(locomotor_status)) {
        record_techno_drawing(*d, locomotor_status);
        return;
    }
    if (airborne) {
        auto* image = FileSystem::LoadSHPFile("POD.SHP");
        if (!image) {
            record_techno_drawing(*d, DrawingStatus::unavailable);
            return;
        }
        const auto delta = self.Locomotor->Shadow_Point();
        ShapeDrawingRequest r;
        r.image = image;
        r.frame = self.Locomotor->Drawing_Code();
        r.position = {point.X + delta.X, point.Y + delta.Y};
        r.clip = *bounds;
        r.flags = 0x2E01;
        r.depth_adjustment = self.GetZAdjustment() - 2;
        r.gradient = 0;
        r.intensity = 1000;
        r.tint = tint;
        if (!techno_submit_shape(*d, self, TechnoPalette::animation, cell, nullptr, r)) return;
        draw_a_shp(ctx, self, image, self.Locomotor->Drawing_Code(), location, bounds, 0, 256, 0, ZGradient::Deg90, 0, 1000, 0,
                        nullptr, 0, 0, 0, 0);
        return;
    }
    auto* image = self.GetImage();
    if (!image) return;
    auto* ground = MapClass::Instance.TryGetCellAt(self.Location);
    if (!ground) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    const bool overshadowed = (unsigned(ground->Flags) & 0x10000u) != 0;
    const int floor = MapClass::Instance.GetCellFloorHeight(self.Location);
    int intensity = std::bit_cast<short>(cell->Intensity_Normal);
    if (self.OnBridge || (overshadowed && self.Location.Z > floor + d->bridge_height / 2))
        intensity += d->level_light * (self.GetHeight() / (2 * d->level_height));
    else if (overshadowed)
        intensity -= 500;
    intensity += d->extra_infantry_light;
    int height = self.GetHeight();
    const auto flags = unsigned(ground->Flags);
    if ((flags & 0x100u) && height >= d->bridge_height) {
        const auto* neighbor = ground->GetNeighbourCell((flags & 0x800u) ? FacingType::North : FacingType::West);
        if (neighbor && (unsigned(neighbor->Flags) & 0x100u)) height -= d->bridge_height;
    }
    if (height > 0) {
        auto* shadow = FileSystem::LoadSHPFile("INFSHDW.SHP");
        if (shadow) {
            ShapeDrawingRequest r;
            r.image = shadow;
            r.frame = 1;
            r.position = {point.X, point.Y - self.DrawingYOffset + TacticalClass::AdjustForZ(height)};
            r.clip = *bounds;
            r.flags = 0x2601;
            r.depth_adjustment = -5 - TacticalClass::AdjustForZ(self.Location.Z - height);
            r.gradient = 0;
            r.intensity = 1000;
            if (!techno_submit_shape(*d, self, TechnoPalette::animation, cell, nullptr, r)) return;
        }
    }
    point.Y -= self.DrawingYOffset;
    const int frame = self.GetCurrentFrame();
    if (frame < 0) return;
    draw_a_shp(ctx, self, image, frame, &point, bounds, 0, 256, DWORD(-10), ZGradient::Deg90, 0, intensity, tint, nullptr, 0,
                    0, 0, 0);
}
}
