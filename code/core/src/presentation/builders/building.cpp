// Copyright 2025 Electronic Arts Inc.
// Copyright 2026 OpenTS contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 additional terms and warranty disclaimers apply;
// see code/third_party/opents/LICENSE.md. Modified by OpenRA2, 2026.
// Adapted from OpenTS building.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR calibration: 0x0043D290 body/lower factory pass; 0x0043DA80
// factory exiting object, upper door and SHP/VXL turret/barrel ordering; 0x0043D030 placement preview split.
#include "presentation/builders/building.hpp"
#include "presentation/builders/techno_common.hpp"
#include "presentation/object_draw.hpp"
#include "presentation/visibility.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/TacticalClass.h"
#include "building_drawing.hpp"
#include "techno_drawing.hpp"
#include "debug/debug_state.hpp"
#include "building_render_rect.hpp"
#include "presentation/state.hpp"
#include "building_barrel_matrix.hpp"
#include "tactical_cells.hpp"
#include "map_runtime.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/AnimClass.h"
#include "yrpp/Matrix3DArithmetic.hpp"
#include <algorithm>
#include <cmath>
namespace game {
namespace {
SHPStruct* data(SHPStruct* image) {
    if (!image) return nullptr;
    if (auto* reference = image->AsReference()) {
        reference->Load();
        return reference->Data;
    }
    return image;
}
bool complete(game::DrawingStatus status) {
    return status == game::DrawingStatus::drawn || status == game::DrawingStatus::skipped;
}
int progress_frame(TransitionTimer timer, int stages) {
    const double value = timer.PercentageDone(), nearest = value * stages;
    const double residual = std::fma(value, double(stages), -nearest);
    const double product =
        (nearest > 0 && residual < 0) || (nearest < 0 && residual > 0) ? std::nextafter(nearest, 0.0) : nearest;
    if (!std::isfinite(product) || product < -2147483648.0 || product >= 2147483648.0) return 0;
    return static_cast<int>(product);
}
bool roof_exit(const BuildingClass& building) {
    const auto* link = building.RadioLinks.Capacity > 0 ? building.RadioLinks[0] : nullptr;
    if (!link) return false;
    const auto* type = static_cast<const TechnoTypeClass*>(link->GetType());
    return type && (type->JumpJet || type->BalloonHover);
}
bool part(game::PresentationContext& ctx, const BuildingClass& building, SHPStruct* image, int index, Point2D point, const RectangleStruct& clip, int z,
          int gradient, int intensity, bool write_depth, SHPStruct* depth = nullptr, Point2D depth_offset = {}) {
    auto* scope = ctx.building;
    if (!image || !complete(scope->status)) return complete(scope->status);
    if (auto* drawing = ctx.techno) {
        auto bounds = clip;
        unsigned tint = building.Airstrike ? drawing->laser_tint : 0;
        if (building.IsIronCurtained() && building.ForceShielded == 1) tint |= drawing->shield_tint;
        game::draw_object(ctx, building, image, index, &point, &bounds, 0, 256, z, ZGradient(gradient), write_depth,
                          intensity, int(tint), depth, 0, depth_offset.X, depth_offset.Y, 0);
        scope->status = drawing->status;
        return complete(scope->status);
    }
    game::ShapeDrawingRequest r;
    r.target = scope->context.target;
    r.palette = scope->context.palette;
    r.image = image;
    r.frame = index;
    r.position = point;
    r.clip = clip;
    r.flags = write_depth ? 0x6E00 : 0x2E00;
    r.gradient = gradient;
    r.depth_adjustment = z - 2;
    // DrawObject (0x705E00) owns the flashing adjustment; this direct path stands in for it.
    r.intensity = building.GetFlashingIntensity(intensity);
    r.depth_image = depth;
    r.depth_offset = depth_offset;
    auto status = game::submit_type_shape(scope->context, r);
    if (status != game::DrawingStatus::skipped) scope->status = status;
    if (!complete(status) || building.Type->NoShadow) return complete(status);
    auto* shape = data(image);
    if (!shape) {
        scope->status = game::DrawingStatus::unavailable;
        return false;
    }
    r.frame += shape->Frames / 2;
    r.flags |= 1;
    r.gradient = 0;
    r.depth_adjustment = -TacticalClass::AdjustForZ(building.Location.Z) - 4;
    r.intensity = 1000;
    r.depth_image = nullptr;
    r.depth_offset = {};
    status = game::submit_type_shape(scope->context, r);
    if (status != game::DrawingStatus::skipped) scope->status = status;
    return complete(status);
}
}
void build_building(PresentationContext& ctx, const BuildingClass& self, Point2D* point, RectangleStruct* clip) {
    auto* scope = ctx.building;
    if (!scope) return;
    if (!point || !clip) {
        scope->status = game::DrawingStatus::invalid_argument;
        return;
    }
    if (!self.Type) return;
    auto* image = self.GetImage();
    if (!image) return;
    // The original sets AnimClass::Invisible on every slot animation here and never clears it again; the
    // presentation keeps the same sticky record without writing the object.
    if (!self.BState && self.GetCurrentMission() == Mission::Selling)
        for (auto* anim : self.Anims)
            if (anim) hidden_anims().hide(anim);
    if (self.Type->InvisibleInGame) return;
    const auto mission = self.GetCurrentMission();
    const int height = TacticalClass::AdjustForZ(self.Location.Z);
    // The state queries are not const in the declarations; a copy of the timer answers them.
    auto timer = self.UnloadTimer;
    if (mission == Mission::Open && !timer.AreStates00()) {
        int index = progress_frame(timer, self.Type->GateStages);
        if (timer.AreStates10()) index = self.Type->GateStages - index;
        if (timer.AreStates00()) index = 0;
        if (timer.AreStates01()) index = self.Type->GateStages - 1;
        index = std::max(0, std::min(index, self.Type->GateStages - 1));
        if (!self.IsGreenHP()) index += self.Type->GateStages + 1;
        part(ctx, self, image, index, *point, *clip, self.Type->NormalZAdjust - height, 2, scope->intensity, true);
        return;
    }
    const bool roof = mission == Mission::Unload && roof_exit(self);
    int z = self.Type->NormalZAdjust;
    if (mission == Mission::Unload) {
        auto* replacement = roof ? self.Type->RoofDeployingAnim : self.Type->DeployingAnim;
        if (replacement) {
            image = replacement;
            z = roof ? -40 : -20;
        }
    }
    auto* shape = data(image);
    if (!shape) {
        scope->status = game::DrawingStatus::unavailable;
        return;
    }
    const int index = std::min(self.GetShapeNumber(), int(shape->Frames) / 2);
    const auto extent = TacticalClass::AdjustForZShapeMove(self.Type->GetFoundationWidth() * 256 - 256,
                                                           self.Type->GetFoundationHeight(false) * 256 - 256);
    const bool constructing = mission == Mission::Construction || mission == Mission::Selling;
    Point2D offset{198 - extent.X, 446 - extent.Y};
    if (!constructing) {
        offset.X += self.Type->ZShapePointMove.X;
        offset.Y += self.Type->ZShapePointMove.Y;
    }
    const int intensity = scope->intensity + static_cast<short>(self.Type->ExtraLight);
    if (clip->Height > 0 && !part(ctx, self, image, index, *point, *clip, z - height, 2, intensity, true,
                                  self.Type->GetFoundationWidth() < 8 ? scope->depth_image : nullptr, offset))
        return;
    if (self.BState != static_cast<int>(BStateType::Construction))
        if (!part(ctx, self, self.Type->BibShape, self.GetShapeNumber(), *point, *clip,
                  -height - 1, 0, intensity, true))
            return;
    if (mission == Mission::Unload)
        part(ctx, self, roof ? self.Type->UnderRoofDoorAnim : self.Type->UnderDoorAnim, self.IsGreenHP() ? 0 : 1, *point, *clip, -height,
             0, intensity, true);
}

void build_building_upper(PresentationContext& ctx, const BuildingClass& self, const Point2D& point, const RectangleStruct& clip) {
    auto* scope = ctx.building;
    if (!self.Type || (self.IsFogged && !game::debug_fog_hidden()) || self.Type->InvisibleInGame) return;
    auto* d = ctx.techno;
    if (d && self.CurrentMission == Mission::Unload && self.IsTether)
        for (int i = 0; i < self.RadioLinks.Capacity; ++i) {
            // 0x43DA80 visits every radio slot, including holes left by earlier exits.
            auto* object = self.RadioLinks[i];
            if (object && !object->InLimbo && object->WhatAmI() != AbstractType::Building) {
                CoordStruct destination;
                object->GetDestination(&destination, nullptr);
                destination.Z = object->Location.Z;
                if (!d->window_active || d->debug_map || !d->fog_of_war ||
                    (!game::techno_location_fogged(object->Location) && !game::techno_location_fogged(destination))) {
                    auto at = d->project ? d->project(d->context, object->GetRenderCoords()) : point;
                    auto bounds = clip;
                    game::draw_it(ctx, *object, &at, &bounds);
                }
            }
        }
    if (scope && self.GetCurrentMission() == Mission::Unload && self.Type->DoorAnim && self.Type->DoorStages > 0) {
        auto door_timer = self.UnloadTimer;
        int index = progress_frame(door_timer, self.Type->DoorStages);
        if (door_timer.AreStates10()) index = self.Type->DoorStages - index;
        if (door_timer.AreStates00()) index = 0;
        index = std::max(0, std::min(index, self.Type->DoorStages - 1));
        if (!self.IsGreenHP() && self.Type->DamagedDoor) index += self.Type->DoorStages;
        part(ctx, self, self.Type->DoorAnim, index, point, clip, -5 - TacticalClass::AdjustForZ(self.Location.Z), 0, scope->intensity,
             false);
        // The original writes NeedsRedraw = true here; the request is the presentation's own now.
        if (door_timer.AreStates10() && !index) redraw_requests().request(&self);
    }
    if (!d || !game::techno_drawing_complete(d->status)) return;
    if (!self.Type->TurretAnimIsVoxel && !self.Type->BarrelAnimIsVoxel) return;
    if (!self.Type->HasSpotlight || !self.Type->TurretAnimIsVoxel) {
        if ((self.CurrentMission == Mission::Construction || self.QueuedMission == Mission::Construction) &&
            self.Animation.Value < self.Type->BuildingAnimFrame[0].dwUnknown + self.Type->BuildingAnimFrame[0].FrameCount - 1)
            return;
        if (self.CurrentMission == Mission::Selling && self.Animation.Value > 0) return;
    }
    const auto ready = [](const VoxelStruct& v) {
        return v.VXL && !v.VXL->Initialized && v.VXL->CountHeaders && v.VXL->BodyData && v.HVA &&
               !v.HVA->LoadedFailed && v.HVA->Matrixes && v.HVA->FrameCount > 0;
    };
    const auto rotation = [](unsigned axis, float angle) {
        Matrix3D m;
        m.MakeIdentity();
        const float cosine = float(Math::cos(angle)), sine = float(Math::sin(angle));
        const int x = (axis + 1) % 3, y = (axis + 2) % 3;
        m.row[x][x] = cosine;
        m.row[x][y] = -sine;
        m.row[y][x] = sine;
        m.row[y][y] = cosine;
        return m;
    };
    const auto yaw = [](unsigned raw) {
        const auto direction = ((raw >> 10) + 1u) / 2u & 31u;
        return matrix_store_float((8 - int(direction)) * 0.19634954084936207);
    };
    const auto& cfg = self.Type->BuildingAnim[9];
    const Point2D at{point.X + cfg.Position.X, point.Y + cfg.Position.Y};
    const int light = (scope ? scope->intensity : 1000) + static_cast<short>(self.Type->ExtraLight);
    const auto draw = [&](VoxelStruct& v, const Matrix3D& local, unsigned frame,
                          IndexClass<VoxelIndexKey, VoxelCacheStruct*>& cache) {
        if (!ready(v) || !game::techno_drawing_complete(d->status)) return;
        const auto matrix = drawing_matrix_product(d->camera, local);
        game::VoxelMatrixScope scope(*d, local);
        game::draw_voxel(ctx, self, v, frame % unsigned(v.HVA->FrameCount), -1, cache, clip, at, matrix, light, 0, 0);
    };
    auto z = rotation(2, yaw(self.PrimaryFacing.Current().Raw));
    const auto y = rotation(1, -yaw(self.BarrelFacing.Current().Raw));
    if (self.Type->TurretAnimIsVoxel) {
        if (ready(self.Type->TurretVoxel)) {
            drawing_translate_axis(z, 0, matrix_store_float(self.Type->TurretOffset / 8));
            auto barrel = z;
            const Vector3D<float> pivot{z.row[0][3], z.row[1][3], z.row[2][3]};
            drawing_translate(barrel, {-pivot.X, -pivot.Y, -pivot.Z});
            if (self.TurretRecoil.State != RecoilData::RecoilState::Inactive)
                drawing_translate_axis(z, 0, -self.TurretRecoil.TravelSoFar);
            if (self.BarrelRecoil.State != RecoilData::RecoilState::Inactive)
                drawing_translate_axis(barrel, 0, -self.BarrelRecoil.TravelSoFar);
            barrel = drawing_matrix_product(barrel, y);
            drawing_translate(barrel, pivot);
            const unsigned quadrant = (((unsigned(self.PrimaryFacing.Current().Raw) >> 13) + 1) / 2) & 3;
            if (quadrant == 0 || quadrant == 3) draw(self.Type->BarrelVoxel, barrel, 0, self.Type->VoxelTurretBarrelCache);
            draw(self.Type->TurretVoxel, z, unsigned(self.TurretAnimFrame), self.Type->VoxelTurretWeaponCache);
            if (quadrant == 1 || quadrant == 2) draw(self.Type->BarrelVoxel, barrel, 0, self.Type->VoxelTurretBarrelCache);
        } else
            draw(self.Type->BarrelVoxel, drawing_matrix_product(z, y), unsigned(self.TurretAnimFrame),
                 self.Type->VoxelTurretBarrelCache);
    } else {
        const unsigned facing = ((unsigned(self.PrimaryFacing.Current().Raw) >> 10) + 1u) / 2u & 31u;
        const bool after = ((facing + 28) % 32) <= 16;
        const auto animation = [&] {
            if (auto* anim = self.Anims[9]; anim && d->animation) {
                // The original clears Invisible for this draw and sets it again afterwards: the animation is
                // drawn on request here, and stays out of the ordinary pass afterwards.
                const auto status = d->animation(d->context, self, *anim, point, clip, true);
                hidden_anims().hide(anim);
                game::record_techno_drawing(*d, status);
            }
        };
        Matrix3D matrix;
        building_barrel_offset_matrix(self, matrix);
        if (after) animation();
        draw(self.Type->BarrelVoxel, matrix, 0, self.Type->VoxelTurretBarrelCache);
        if (!after) animation();
    }
}


void build_building_again(PresentationContext& ctx, const BuildingClass& self, const Point2D& point, const RectangleStruct& bounds) {
    // The state queries are not const in the declarations; a copy of the timer answers them.
    auto door = self.UnloadTimer;
    const auto cell = self.GetMapCoords();
    auto* image = self.GetImage();
    if (!image || self.Type->InvisibleInGame || (self.GetCurrentMission() == Mission::Open && !door.AreStates00()))
        return;
    if (self.GetCurrentMission() == Mission::Unload && self.Type->DeployingAnim) {
        auto* object = self.Factory->Object;
        image = object && object->GetTechnoType()->JumpJet ? self.Type->RoofDeployingAnim : self.Type->DeployingAnim;
    }
    const int height = static_cast<short>(image->Height);
    const int split = point.Y - height / 2;
    auto clip = bounds;
    auto position = point;
    clip.Height = std::min(clip.Height, split);
    if (clip.Height > 0) {
        const int light = static_cast<short>(MapClass::Instance.GetCellAt(cell)->Intensity_Normal);
        const int z = -2 - TacticalClass::AdjustForZ(self.GetZ());
        game::draw_object(ctx, self, image, self.GetShapeNumber(), &position, &clip, 0, 256, z, ZGradient(2), 0, light, 0,
                          nullptr, 0, 0, 0, 0);
    }
    position = {point.X, point.Y - split};
    clip = bounds;
    clip.Y += split;
    clip.Height = height;
    if (clip.Y + clip.Height > bounds.Y + bounds.Height) clip.Height = bounds.Y + bounds.Height - clip.Y;
    if (clip.Y < bounds.Y) {
        clip.Y = bounds.Y;
        clip.Height += split;
        position.Y += split;
    }
    if (clip.Height > 0) {
        const int light = static_cast<short>(MapClass::Instance.GetCellAt(cell)->Intensity_Normal);
        const int z = -TacticalClass::AdjustForZ(self.GetZ());
        game::draw_object(ctx, self, image, self.GetShapeNumber(), &position, &clip, 0, 256, z, ZGradient(0), 0, light, 0,
                          nullptr, 0, 0, 0, 0);
    }
}

void build_buildings(PresentationContext& ctx, bool forced, const RectangleStruct& area, const RectangleStruct& clip) {
    try {
        const auto intersection = Drawing::Intersect(area, clip), bounds = tactical_pass_viewport(ctx);
        const auto& runtime = game::map_runtime();
        const bool debug = game::debug_fog_hidden() || (runtime.debug_map && *runtime.debug_map);
        auto& layer = MapClass::ObjectsInLayers[int(Layer::Ground)];
        for (int i = 0; i < layer.Count && tactical_pass_complete(ctx); ++i) {
            auto* object = layer[i];
            if (object->WhatAmI() != AbstractType::Building) continue;
            auto* building = static_cast<BuildingClass*>(object);
            if (!building->Type || (building->IsFogged && !debug)) continue;
            // The memo-free value of GetRenderDimensions: the drawing does not write the building's cache.
            auto dimensions = game::building_render_rect(*building, ctx.frame ? ctx.frame->state : nullptr);
            dimensions.X += bounds.X;
            dimensions.Y += bounds.Y;
            if (!tactical_rects_overlap(dimensions, intersection)) continue;
            game::record_tactical_drawing(ctx, game::draw_tactical_object(ctx, *object, forced, clip, false));
        }
    } catch (...) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}
}
