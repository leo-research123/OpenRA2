// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 aircraft.cpp::Draw_It; YR 0x004144B0.
#include "presentation/builders/aircraft.hpp"
#include "presentation/builders/techno_common.hpp"
#include "presentation/object_draw.hpp"
#include "selectable_registration.hpp"
#include "aircraft_shadow_cycle.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "techno_drawing.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/Matrix3DArithmetic.hpp"
#include <bit>
namespace game {
void build_aircraft(PresentationContext& ctx, const AircraftClass& self, Point2D* location, RectangleStruct* bounds) {
    auto* d = ctx.techno;
    if (!d) return;
    if (!location || !bounds || !self.Type || !self.Locomotor) {
        record_techno_drawing(*d, DrawingStatus::invalid_argument);
        return;
    }
    Point2D point;
    int frame = 0;
    const auto prepared = aircraft_draw_anchor(self, *d, *location, point, frame);
    if (prepared != DrawingStatus::drawn) {
        record_techno_drawing(*d, prepared);
        return;
    }
    if (self.Passengers.NumPassengers && self.Type->Carryall && self.Passengers.FirstPassenger)
        draw_it(ctx, *self.Passengers.FirstPassenger, &point, bounds);
    CellStruct map_coords;
    self.GetMapCoords(&map_coords);
    if (!techno_drawing_complete(d->status) || !self.Type->Voxel || !self.Type->MainVoxel.VXL) return;
    auto ground = self.Location;
    ground.Z = MapClass::Instance.GetCellFloorHeight(self.Location);
    auto shadow =
        d->project ? d->project(d->context, ground) : Point2D{location->X, location->Y + TacticalClass::AdjustForZ(self.GetHeight())};
    const auto shadow_offset = self.Locomotor->Shadow_Point();
    shadow.X += shadow_offset.X;
    shadow.Y += shadow_offset.Y;
    // The shadow inputs were sampled by the render-phase step while the aircraft was at the ground height (the
    // original lowers it around the shadow draw; see aircraft_shadow_cycle.hpp). The height is not touched here.
    auto* cell = MapClass::Instance.TryGetCellAt(self.Location);
    if (!cell) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    VoxelIndexKey key(frame & 0x1F);
    if (const auto* sample = aircraft_shadow_sample(&self)) {
        if (sample->raised) shadow.Y -= TacticalClass::AdjustForZ(d->bridge_height);
        key = sample->key;
        const auto& local = sample->local;
        if (!self.IsSinking) {
            VoxelMatrixScope scope(*d, local);
            auto matrix = drawing_matrix_product(d->camera, local);
            const ShadowSample at_ground{sample->has_shadow, sample->shadow_delta, sample->depth};
            draw_foot_voxel_shadow(ctx, self, &self.Type->MainVoxel, self.Type->ShadowIndex, key,
                                   &self.Type->VoxelShadowCache, bounds, &shadow, &matrix, true, nullptr, {}, &at_ground);
        }
    }
    if (!techno_drawing_complete(d->status)) return;
    const int height = self.GetHeight();
    auto* light_cell = MapClass::Instance.TryGetCellAt(ground);
    if (!light_cell) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    const int intensity = std::bit_cast<short>(light_cell->Intensity_Normal) +
                          d->level_light * (height / (2 * d->level_height)) + d->extra_aircraft_light;
    key.Value = frame & 0x1F;
    auto local = self.Locomotor->Draw_Matrix(&key);
    auto matrix = drawing_matrix_product(d->camera, local);
    VoxelMatrixScope scope(*d, local);
    draw_a_vxl(ctx, self, &self.Type->MainVoxel, int(frame), key.Value, &self.Type->VoxelMainCache, bounds, &point, &matrix, intensity,
                    static_cast<BlitterFlags>(0), 0);
}
}
