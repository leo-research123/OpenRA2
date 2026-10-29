// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 techno.cpp::Techno_Draw_Object; YR 0x00705E00.
// Drawing helpers the infantry, unit, aircraft and building builders share.
#include "presentation/builders/techno_common.hpp"
#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/TacticalClass.h"
#include "techno_drawing.hpp"
#include "techno_visual_queries.hpp"
#include "yrpp/FootClass.h"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include <algorithm>
#include <bit>
namespace game {
TechnoTypeClass* drawing_techno_type(const PresentationContext& ctx, const TechnoClass& self) {
    return self.WhatAmI() == AbstractType::Unit && ctx.unit_type ? ctx.unit_type : self.GetTechnoType();
}
int drawing_z_adjustment(const PresentationContext& ctx, const TechnoClass& self) {
    return ctx.unit_type && self.WhatAmI() == AbstractType::Unit && ctx.unit_type != static_cast<const UnitClass&>(self).Type
        ? foot_z_adjustment(static_cast<const FootClass&>(self), *ctx.unit_type) : self.GetZAdjustment();
}
VisualType drawing_visual_character(const PresentationContext& ctx, const TechnoClass& self) {
    return ctx.unit_type && self.WhatAmI() == AbstractType::Unit && ctx.unit_type != static_cast<const UnitClass&>(self).Type
        ? foot_visual_character(static_cast<const FootClass&>(self), false, nullptr, *ctx.unit_type) : self.VisualCharacter(false, nullptr);
}

void draw_object(PresentationContext& ctx, const TechnoClass& self, SHPStruct* image, int frame, Point2D* location,
                 RectangleStruct* bounds, int, int, int z, ZGradient gradient, int write_depth, int intensity, int tint,
                 SHPStruct* depth_image, int depth_frame, int depth_x, int depth_y, int excluded) {
    auto* d = ctx.techno;
    if (!d || !image) return;
    if (!location || !bounds) {
        record_techno_drawing(*d, DrawingStatus::invalid_argument);
        return;
    }
    const auto visual = drawing_visual_character(ctx, self);
    unsigned flags = 0;
    if (!excluded) {
        switch (visual) {
            case VisualType::Hidden:
                return;
            case VisualType::Indistinct:
                flags = 2;
                break;
            case VisualType::Darken:
            case VisualType::Shadowy:
                flags = 4;
                break;
            case VisualType::Ripple:
                flags = self.CloakProgress.Value ? 4 : 2;
                break;
            default:
                break;
        }
    }
    const auto kind = self.WhatAmI();
    const auto* building = kind == AbstractType::Building ? static_cast<const BuildingClass*>((&self)) : nullptr;
    const auto* unit = kind == AbstractType::Unit ? static_cast<const UnitClass*>((&self)) : nullptr;
    if (self.IsBeingWarpedOut() || self.IsWarpingIn()) flags |= building && building->Type->DoubleThick ? 6u : 4u;
    auto palette = TechnoPalette::house;
    auto* house = self.Owner;
    CellClass* cell = nullptr;
    if (building && building->Type->TerrainPalette) {
        cell = techno_drawing_cell(*(&self));
        if (!cell) {
            record_techno_drawing(*d, DrawingStatus::unavailable);
            return;
        }
        palette = TechnoPalette::cell;
        intensity = std::bit_cast<short>(cell->Intensity_Terrain);
    } else if (unit && (ctx.unit_terrain_palette || unit->TerrainPalette))
        palette = TechnoPalette::eight_bit;
    bool shadow = d->draw_shadows;
    auto point = *location;
    if (kind == AbstractType::Unit || kind == AbstractType::Infantry) {
        if (unit && (static_cast<UnitTypeClass*>(drawing_techno_type(ctx, self))->SmallVisceroid || static_cast<UnitTypeClass*>(drawing_techno_type(ctx, self))->LargeVisceroid)) shadow = false;
        if (self.GetHeight()) {
            z -= TacticalClass::AdjustForZ(self.GetZ());
            shadow = false;
        } else
            z += drawing_z_adjustment(ctx, self);
    } else if (kind == AbstractType::Aircraft) {
        point.Y -= TacticalClass::AdjustForZ(self.GetZ());
        z -= TacticalClass::AdjustForZ(self.GetZ());
    }
    if (drawing_techno_type(ctx, self)->NoShadow) shadow = false;
    if (!self.IsClearlyVisibleTo(HouseClass::CurrentPlayer)) {
        auto* disguise = self.GetDisguise(true);
        if (disguise &&
            (disguise->WhatAmI() == AbstractType::TerrainType || disguise->WhatAmI() == AbstractType::OverlayType)) {
            cell = self.GetCell();
            palette = TechnoPalette::cell;
            if (!cell) {
                record_techno_drawing(*d, DrawingStatus::unavailable);
                return;
            }
            intensity = std::bit_cast<short>(cell->Intensity_Terrain);
        } else {
            house = self.GetDisguiseHouse(true);
            palette = TechnoPalette::house;
        }
        if (!house && palette == TechnoPalette::house) house = HouseClass::CurrentPlayer;
    }
    if (gradient != ZGradient::None) flags |= 0x2000;
    if (write_depth) flags |= 0x4000;
    flags |= 0x800;
    if (unit && (ctx.unit_terrain_palette || unit->TerrainPalette)) {
        flags |= 0x20;
        shadow = false;
    }
    flags &= ~unsigned(excluded);
    auto* shape = d->shape_data ? d->shape_data(image) : image;
    if (!shape) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    if (unit && (ctx.unit_terrain_palette || unit->TerrainPalette)) {
        if (!d->frame_bounds) {
            record_techno_drawing(*d, DrawingStatus::unavailable);
            return;
        }
        const auto frame_bounds = d->frame_bounds(shape, frame);
        const RectangleStruct r{point.X + frame_bounds.X - shape->Width / 2,
                                point.Y + frame_bounds.Y - shape->Height / 2, frame_bounds.Width, frame_bounds.Height};
        auto& out = d->composite_rect;
        if (out.Width <= 0 || out.Height <= 0)
            out = r;
        else if (r.Width > 0 && r.Height > 0) {
            const int right = out.X + out.Width, bottom = out.Y + out.Height;
            out.X = std::min(out.X, r.X);
            out.Y = std::min(out.Y, r.Y);
            out.Width = (right < r.X + r.Width ? r.X + r.Width + 1 : right) - out.X;
            out.Height = (bottom < r.Y + r.Height ? r.Y + r.Height + 1 : bottom) - out.Y;
        }
    }
    if (self.Owner && self.Owner->IsControlledByCurrentPlayer() && self.IsDisguised()) flags = self.GetDisguiseFlags(flags);
    intensity = self.GetFlashingIntensity(intensity);
    if (self.IsIronCurtained() || (building && building->Airstrike && building->Airstrike->Target == building))
        intensity = self.GetEffectTintIntensity(intensity);
    if (auto* at = techno_drawing_cell(*(&self)); at && techno_cell_shrouded(*at)) tint = 0;
    if (visual > VisualType::Ripple) return;
    ShapeDrawingRequest r;
    r.image = image;
    r.frame = frame;
    r.position = point;
    r.clip = *bounds;
    r.flags = flags | 0x600;
    r.depth_adjustment = z - 2;
    r.gradient = int(gradient);
    r.intensity = intensity;
    r.tint = unsigned(tint);
    r.depth_image = depth_image;
    r.depth_frame = depth_frame;
    r.depth_offset = {depth_x, depth_y};
    if (!techno_submit_shape(*d, *(&self), palette, cell, house, r)) return;
    if (shadow && (visual == VisualType::Normal || visual == VisualType::Indistinct)) {
        r.frame = frame + shape->Frames / 2;
        r.flags = visual == VisualType::Normal ? (flags & 0xFFFFF9F8u) | 0x601u : (flags & 0xFFFFFFF9u) | 0x601u;
        r.depth_adjustment = -TacticalClass::AdjustForZ(self.GetZ()) - 4;
        r.gradient = 0;
        r.intensity = 1000;
        r.tint = 0;
        if (self.IsOnCarryall) r.position.Y -= 14;
        r.depth_image = nullptr;
        r.depth_frame = 0;
        r.depth_offset = {};
        techno_submit_shape(*d, *(&self), palette, cell, house, r);
    }
}

void draw_a_shp(PresentationContext& ctx, const FootClass& self, SHPStruct* image, int frame, Point2D* point,
                RectangleStruct* clip, DWORD rotation, DWORD scale, DWORD z, ZGradient gradient, DWORD write_depth,
                int intensity, DWORD tint, SHPStruct* depth_image, DWORD depth_frame, DWORD depth_x, DWORD depth_y,
                DWORD excluded) {
    draw_object(ctx, self, image, frame, point, clip, int(rotation), int(scale), int(z), gradient, int(write_depth),
                intensity, int(tint), depth_image, int(depth_frame), int(depth_x), int(depth_y), int(excluded));
}

void draw_voxel(PresentationContext& ctx, const TechnoClass& self, const VoxelStruct& voxel, DWORD frame, int key,
                const IndexClass<VoxelIndexKey, VoxelCacheStruct*>&, const RectangleStruct& clip, const Point2D& point,
                const Matrix3D& matrix, int intensity, DWORD tint, DWORD excluded) {
    auto* d = ctx.techno;
    if (!d || !techno_drawing_complete(d->status)) return;
    unsigned flags = 0x2000;
    if (!excluded) {
        switch (drawing_visual_character(ctx, self)) {
            case VisualType::Hidden:
                return;
            case VisualType::Indistinct:
                flags |= 2;
                break;
            case VisualType::Darken:
            case VisualType::Shadowy:
                flags |= 4;
                break;
            case VisualType::Ripple:
                flags |= self.CloakProgress.Value ? 0xC : 0xA;
                break;
            default:
                break;
        }
    }
    const auto* unit = self.WhatAmI() == AbstractType::Unit ? static_cast<const UnitClass*>((&self)) : nullptr;
    const auto* building = self.WhatAmI() == AbstractType::Building ? static_cast<const BuildingClass*>((&self)) : nullptr;
    const bool composite = unit && (ctx.unit_terrain_palette || unit->TerrainPalette);
    if (!composite && (self.IsBeingWarpedOut() || self.IsWarpingIn())) flags |= building && building->Type->DoubleThick ? 6 : 4;
    if (!composite && self.Owner && self.Owner->IsControlledByCurrentPlayer() && self.IsDisguised()) flags = self.GetDisguiseFlags(flags);
    flags = (flags | 0x800) & ~excluded;
    intensity = self.GetFlashingIntensity(intensity);
    if (self.IsIronCurtained() || (building && building->Airstrike && building->Airstrike->Target == building))
        intensity = self.GetEffectTintIntensity(intensity);
    if (drawing_techno_type(ctx, self)->DisableVoxelCache) key = -1;
    if (!voxel.VXL || !voxel.HVA) return;
    TechnoVoxelRequest r;
    r.object = (&self);
    r.resource = &voxel;
    r.frame = int(frame);
    r.cache_key = key;
    r.matrix = matrix;
    r.local_matrix = d->local_voxel_matrix;
    r.position = point;
    r.clip = clip;
    r.flags = flags;
    r.intensity = intensity;
    r.tint = tint;
    r.depth = drawing_z_adjustment(ctx, self);
    r.use_buffer = drawing_techno_type(ctx, self)->UseBuffer;
    RectangleStruct raster_bounds{};
    r.raster_bounds = &raster_bounds;
    auto kind = composite ? TechnoPalette::eight_bit : TechnoPalette::house;
    auto* cell = techno_drawing_cell(*(&self));
    if (building && building->Type->TerrainPalette) kind = TechnoPalette::cell;
    if (!techno_drawing_palette(*d, *(&self), kind, cell, self.Owner, r.palette)) return;
    const auto status = d->voxel ? d->voxel(d->context, r) : DrawingStatus::unavailable;
    record_techno_drawing(*d, status);
    if (unit && status == DrawingStatus::drawn)
        accumulate_unit_voxel_bounds(d->composite_rect, {point.X + raster_bounds.X, point.Y + raster_bounds.Y,
                                                               raster_bounds.Width, raster_bounds.Height});
}
void draw_voxel_shadow(PresentationContext& ctx, const TechnoClass& self, VoxelStruct* voxel, int layer, VoxelIndexKey key,
                       IndexClass<ShadowVoxelIndexKey, VoxelCacheStruct*>* cache, RectangleStruct* clip, Point2D* point,
                       Matrix3D* matrix, bool force, Surface* exclusion, Point2D shadowPoint,
                       const ShadowSample* sampled) {
    auto* d = ctx.techno;
    if (!d || !techno_drawing_complete(d->status)) return;
    if (!voxel || !voxel->VXL || !voxel->HVA || !clip || !point || !matrix) {
        record_techno_drawing(*d, DrawingStatus::invalid_argument);
        return;
    }
    if (self.CloakState != ::CloakState::Uncloaked || drawing_techno_type(ctx, self)->NoShadow || voxel->VXL->Initialized) return;
    TechnoVoxelRequest r;
    r.object = (&self);
    r.resource = voxel;
    r.matrix = *matrix;
    r.local_matrix = d->local_voxel_matrix;
    r.position = *point;
    r.clip = *clip;
    r.shadow = true;
    r.shadow_layer = layer;
    r.cache_key = drawing_techno_type(ctx, self)->DisableShadowCache ? -1 : key.Value;
    r.shadow_exclusion = exclusion;
    r.shadow_offset = shadowPoint;
    r.shadow_cache = cache;
    r.shadow_force = force;
    r.flags = 0x2001;
    r.intensity = 1000;
    r.depth = sampled ? sampled->depth : drawing_z_adjustment(ctx, self);
    r.half_shadow = self.WhatAmI() == AbstractType::Aircraft || drawing_techno_type(ctx, self)->ConsideredAircraft;
    if (!techno_drawing_palette(*d, *(&self), TechnoPalette::house, techno_drawing_cell(*(&self)),
                                      self.Owner, r.palette))
        return;
    record_techno_drawing(*d, d->voxel ? d->voxel(d->context, r) : DrawingStatus::unavailable);
}
void draw_a_vxl(PresentationContext& ctx, const FootClass& self, VoxelStruct* voxel, int frame, int key,
                IndexClass<VoxelIndexKey, VoxelCacheStruct*>* cache, RectangleStruct* clip, Point2D* point,
                Matrix3D* matrix, int intensity, BlitterFlags flags, DWORD tint) {
    auto* d = ctx.techno;
    if (!d) return;
    if (!voxel || !cache || !clip || !point || !matrix) {
        record_techno_drawing(*d, DrawingStatus::invalid_argument);
        return;
    }
    auto position = *point;
    if (self.Locomotor) {
        const auto delta = self.Locomotor->Draw_Point();
        position.X += delta.X;
        position.Y += delta.Y;
    }
    draw_voxel(ctx, self, *voxel, DWORD(frame), key, *cache, *clip, position, *matrix, intensity, tint, DWORD(flags));
}
void draw_foot_voxel_shadow(PresentationContext& ctx, const FootClass& self, VoxelStruct* voxel, int layer, VoxelIndexKey key,
                            IndexClass<ShadowVoxelIndexKey, VoxelCacheStruct*>* cache, RectangleStruct* clip,
                            Point2D* point, Matrix3D* matrix, bool force, Surface* surface, Point2D shadowPoint,
                            const ShadowSample* sampled) {
    if (!self.Locomotor || !(sampled ? sampled->has_shadow : self.Locomotor->Is_To_Have_Shadow())) return;
    if (!point) {
        if (auto* d = ctx.techno) record_techno_drawing(*d, DrawingStatus::invalid_argument);
        return;
    }
    auto position = *point;
    const auto delta = sampled ? sampled->delta : self.Locomotor->Shadow_Point();
    position.X += delta.X;
    position.Y += delta.Y;
    draw_voxel_shadow(ctx, self, voxel, layer, key, cache, clip, &position, matrix, force, surface, shadowPoint, sampled);
}
}
