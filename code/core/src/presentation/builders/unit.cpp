// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 unit.cpp::Draw_It / Unit_Draw_Shape / Unit_Draw_Voxel;
// YR 0x0073CEC0 / 0x0073C5F0 / 0x0073B470 / 0x0073C860.
#include "presentation/builders/unit.hpp"
#include "presentation/builders/techno_common.hpp"
#include "selectable_registration.hpp"
#include "yrpp/UnitClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/Drawing.h"
#include "x87_integer.hpp"
#include "yrpp/Matrix3DArithmetic.hpp"
#include "techno_drawing.hpp"
#include "locomotor_drawing.hpp"
#include "presentation/visibility.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include <algorithm>
#include <bit>
namespace game {
namespace {
bool ready(const VoxelStruct& v) {
    return v.VXL && !v.VXL->Initialized && v.VXL->CountHeaders && v.VXL->BodyData && v.HVA && !v.HVA->LoadedFailed &&
           v.HVA->Matrixes && v.HVA->FrameCount > 0 && v.HVA->LayerCount >= int(v.VXL->CountHeaders);
}
// The original IndexClass lookup sorts the type and updates Archive through const. Native rendering
// owns raster caches separately; compatibility entries can be inspected without writing the type.
bool cached_voxel(const IndexClass<VoxelIndexKey, VoxelCacheStruct*>& cache, int key, VoxelCacheStruct*& value) {
    if (cache.Archive && cache.Archive->ID == key) {
        value = cache.Archive->Data;
        return true;
    }
    for (int i = 0; i < cache.IndexCount; ++i)
        if (cache.IndexTable[i].ID == key) {
            value = cache.IndexTable[i].Data;
            return true;
        }
    return false;
}

}

void build_unit(PresentationContext& ctx, const UnitClass& self, Point2D* location, RectangleStruct* bounds) {
    auto* d = ctx.techno;
    if (!d) return;
    if (!location || !bounds || !self.Type || !self.Locomotor) {
        record_techno_drawing(*d, DrawingStatus::invalid_argument);
        return;
    }
    const Point2D raw{location->X, location->Y - self.DrawingYOffset};
    Point2D point;
    if (!unit_selectable_anchor(self, *location, point)) return;
    auto* cell = techno_drawing_cell(self);
    auto* ground = MapClass::Instance.TryGetCellAt(self.Location);
    if (!cell || !ground) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    const int height = self.GetHeight();
    const auto flags = unsigned(ground->Flags);
    bool bridge = false;
    if ((!self.OnBridge && height >= d->bridge_height) || (self.OnBridge && height >= 0))
        if (flags & 0x100u) {
            const auto* adjacent = ground->GetNeighbourCell((flags & 0x800u) ? FacingType::North : FacingType::West);
            bridge = adjacent && (unsigned(adjacent->Flags) & 0x100u);
        }
    const int intensity = std::bit_cast<short>(cell->Intensity_Normal) +
                          (bridge                               ? 4 * d->level_light
                           : (unsigned(cell->Flags) & 0x10000u) ? -500
                                                                : 0) +
                          d->extra_unit_light;
    // Keep the effect hidden while movement is pending, even at zero speed.
    if (self.Type->Harvester && self.IsHarvesting && !self.Locomotor->Is_Moving() && !self.Locomotor->Is_Moving_Now() &&
        !self.IsBeingWarpedOut() && !self.IsWarpingIn()) {
        auto* image = FileSystem::LoadSHPFile("OREGATH.SHP");
        if (image) {
            const auto facing = self.PrimaryFacing.Current().GetValue<3>();
            const double angle = (std::bit_cast<short>(std::uint16_t(facing << 13)) - 0x3FFF) * -0.00009587672516830327;
            auto at = self.GetRenderCoords();
            at.Y = int(double(at.Y) - Math::sin(angle) * 30.0);
            at.X = int(Math::cos(angle) * 30.0 + at.X);
            ShapeDrawingRequest r;
            r.image = image;
            r.frame = 15 * (7 - facing) + (d->frame + self.WalkedFramesSoFar) % 15;
            r.position = d->project ? d->project(d->context, at) : point;
            r.clip = *bounds;
            r.flags = 0x2A00;
            r.gradient = 0;
            r.depth_adjustment = drawing_z_adjustment(ctx, self) - 2;
            r.intensity = intensity;
            if (!self.TemporalTargetingMe &&
                !techno_submit_shape(*d, self, TechnoPalette::animation, cell, nullptr, r))
                return;
        }
    }
    auto body_ctx = ctx;
    auto* effective_type = self.Unloading && self.Type->Harvester && self.Type->UnloadingClass
        ? self.Type->UnloadingClass : self.Type;
    body_ctx.unit_type = effective_type;
    ObjectTypeClass* apparent = effective_type;
    if (!self.IsClearlyVisibleTo(HouseClass::CurrentPlayer))
        if (auto* disguise = self.GetDisguise(true)) apparent = disguise;
    if (self.Deployed && effective_type->UnloadingClass) apparent = effective_type->UnloadingClass;
    if (self.TubeIndex == -1) {
        if (apparent->WhatAmI() == AbstractType::TerrainType) {
            const auto* previous_type = d->appearance_type;
            struct RestoreType { TechnoDrawing& drawing; const TechnoTypeClass* previous;
                ~RestoreType() { drawing.appearance_type = previous; } } restore_type{*d, previous_type};
            d->appearance_type = effective_type;
            draw_a_shp(body_ctx, self, apparent->GetImage(), 0, &point, bounds, 0, 256, 0, ZGradient::Deg90, 0, intensity, 0,
                            nullptr, 0, 0, 0, 0);
        } else if (apparent->Voxel) {
            build_unit_vxl(body_ctx, self, point, *bounds, intensity, 0);
        } else
            build_unit_shp(body_ctx, self, point, *bounds, intensity, 0);
    }
    if (!techno_drawing_complete(d->status)) return;
    if (self.FlagHouseIndex != -1) {
        auto* image = FileSystem::LoadSHPFile("FLAGFLY.SHP");
        auto* owner = HouseClass::Array.GetItemOrDefault(self.FlagHouseIndex);
        if (image && owner) {
            ShapeDrawingRequest r;
            r.image = image;
            r.frame = d->frame % 14;
            r.position = raw;
            r.clip = *bounds;
            r.flags = 0xA00;
            r.gradient = 0;
            r.intensity = intensity;
            techno_submit_shape(*d, self, TechnoPalette::house, cell, owner, r);
        }
    }
}

void build_unit_shp(PresentationContext& ctx, const UnitClass& self, Point2D point, RectangleStruct clip, int intensity, int tint) {
    auto* effective_type = ctx.unit_type ? ctx.unit_type : self.Type;
    auto* d = ctx.techno;
    if (!d || !effective_type) return;
    struct AppearanceScope {
        TechnoDrawing& drawing;
        const TechnoTypeClass* previous;
        ~AppearanceScope() { drawing.appearance_type = previous; }
    } appearance{*d, d->appearance_type};
    d->appearance_type = effective_type;
    auto* image = effective_type->GetImage();
    if (!image) return;
    int facing = 0;
    if (effective_type->Facings == 8 && !self.IsDisguised()) facing = (int(self.PrimaryFacing.Current().GetValue<3>()) + 1) & 7;
    int frame = facing;
    if (self.CurrentFiringFrame >= 0)
        frame = effective_type->StartFiringFrame + self.CurrentFiringFrame / 2 + facing * int(effective_type->FiringFrames);
    else if ((self.Locomotor && self.Locomotor->Is_Moving()) || effective_type->IdleRate) {
        if (effective_type->WalkFrames <= 0) {
            record_techno_drawing(*d, DrawingStatus::invalid_argument);
            return;
        }
        frame = effective_type->StartWalkFrame + facing * int(effective_type->WalkFrames) + self.WalkedFramesSoFar % int(effective_type->WalkFrames);
    } else if (self.DeathFrameCounter >= 0) {
        if (effective_type->DeathFrameRate <= 0) {
            record_techno_drawing(*d, DrawingStatus::invalid_argument);
            return;
        }
        frame = effective_type->StartDeathFrame + std::min(self.DeathFrameCounter / effective_type->DeathFrameRate, effective_type->DeathFrames - 1);
    } else if (self.FrozenStill)
        frame = effective_type->StandingFrames ? effective_type->StartStandFrame + facing * effective_type->StandingFrames
                                     : effective_type->StartWalkFrame + facing * int(effective_type->WalkFrames);
    if (!effective_type->Turret) {
        draw_a_shp(ctx, self, image, frame, &point, &clip, 0, 256, 0, self.GetZGradient(), 0, intensity, 0, nullptr, 0, 0, 0, 0);
        return;
    }
    // Native hosts compose the ordered parts at their final position instead
    // of using the EXE's shared 256x256 EightBitSurface. Frame and matrix choices
    // belong here; the host consumes the resulting requests synchronously.
    auto* shape = d->shape_data ? d->shape_data(image) : image;
    if (!shape) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    ShapeDrawingRequest shadow;
    shadow.image = image;
    shadow.frame = frame + shape->Frames / 2;
    shadow.position = point;
    if (self.IsOnCarryall) shadow.position.Y -= 14;
    shadow.clip = clip;
    shadow.flags = 0x2E01;
    shadow.depth_adjustment = drawing_z_adjustment(ctx, self) - 2;
    shadow.intensity = 1000;
    if (!techno_submit_shape(*d, self, TechnoPalette::normal, techno_drawing_cell(self),
                                   nullptr, shadow))
        return;
    const bool shadows = d->draw_shadows;
    struct Restore {
        TechnoDrawing& d;
        bool shadows;
        ~Restore() { d.draw_shadows = shadows; }
    } restore{*d, shadows};
    d->draw_shadows = false;
    auto barrel = Matrix3D::GetIdentity();
    const bool has_barrel = effective_type->BarrelVoxel.VXL && effective_type->BarrelVoxel.HVA;
    bool above = true;
    if (has_barrel) {
        VoxelIndexKey key(-1);
        auto body = (effective_type == self.Type ? self.Locomotor->Draw_Matrix(&key)
            : locomotor_drawing_matrix(self.Locomotor, &key, *effective_type));
        drawing_translate_axis(body, 0, matrix_store_float(effective_type->TurretOffset / 8));
        const double secondary = (int(self.SecondaryFacing.Current().GetValue<5>()) - 8) * -0.1963495408493621;
        const double primary = (int(self.PrimaryFacing.Current().GetValue<5>()) - 8) * -0.1963495408493621;
        body.RotateZ(matrix_store_float(secondary - primary));
        barrel = body;
        const Vector3D<float> pivot{body.row[0][3], body.row[1][3], body.row[2][3]};
        drawing_translate(barrel, {-pivot.X, -pivot.Y, -pivot.Z});
        auto* weapon = self.Veterancy.IsElite() && effective_type->EliteWeapon[0].WeaponType
            ? &effective_type->EliteWeapon[0] : &effective_type->Weapon[0];
        if (!weapon || !weapon->WeaponType || !weapon->WeaponType->Projectile) {
            record_techno_drawing(*d, DrawingStatus::unavailable);
            return;
        }
        const Vector3D<float> flh{float(weapon->FLH.X / -8), 0, float(weapon->FLH.Z / -8)};
        drawing_translate(barrel, {-flh.X, 0, -flh.Z});
        auto facing = self.BarrelFacing.Current();
        if (weapon->WeaponType->Projectile->Inviso) {
            int direction = facing.GetValue<8>();
            direction = direction > 64 && direction <= 128 ? (direction - 64) / 3 + 64 : (64 - direction) / 3 + 64;
            facing.Raw = WORD(direction << 8);
        }
        if (self.BarrelRecoil.State != RecoilData::RecoilState::Inactive)
            drawing_translate_axis(barrel, 0, -self.BarrelRecoil.TravelSoFar);
        barrel.RotateY(matrix_store_float(-(int(facing.GetValue<5>()) - 8) * -0.1963495408493621));
        drawing_translate(barrel, flh);
        drawing_translate(barrel, pivot);
        const auto quadrant = self.SecondaryFacing.Current().GetValue<2>();
        above = quadrant == 1 || quadrant == 2;
    }
    const auto draw_barrel = [&] {
        VoxelMatrixScope scope(*d, barrel);
        auto matrix = drawing_matrix_product(d->camera, barrel);
        draw_a_vxl(ctx, self, &effective_type->BarrelVoxel, 0, -1, &effective_type->VoxelTurretBarrelCache, &clip, &point, &matrix, intensity,
                   BlitterFlags(0), unsigned(tint));
    };
    draw_a_shp(ctx, self, image, frame, &point, &clip, 0, 256, 0, ZGradient::Ground, 0, intensity, unsigned(tint), nullptr, 0, 0,
               0, 0);
    if (has_barrel && !above) draw_barrel();
    const int turret = 8 * int(effective_type->WalkFrames) + ((int(self.SecondaryFacing.Current().GetValue<5>()) + 4) & 31);
    draw_a_shp(ctx, self, image, turret, &point, &clip, 0, 256, 0, ZGradient::Ground, 0, intensity, unsigned(tint), nullptr, 0, 0,
               0, 0);
    if (has_barrel && above) draw_barrel();
}

void build_unit_vxl(PresentationContext& ctx, const UnitClass& self, Point2D point, RectangleStruct clip, int intensity, int tint) {
    auto* effective_type = ctx.unit_type ? ctx.unit_type : self.Type;
    auto* d = ctx.techno;
    if (!d || !effective_type || !self.Locomotor) return;
    struct AppearanceScope {
        TechnoDrawing& drawing;
        const TechnoTypeClass* previous;
        ~AppearanceScope() { drawing.appearance_type = previous; }
    } appearance{*d, d->appearance_type};
    d->appearance_type = effective_type;
    auto* type = effective_type;
    if (!self.IsClearlyVisibleTo(HouseClass::CurrentPlayer)) {
        auto* disguise = self.GetDisguise(true);
        if (disguise && disguise->WhatAmI() == AbstractType::UnitType) type = static_cast<UnitTypeClass*>(disguise);
    }
    // 0x73B4C8: the deployed image takes precedence over disguise.
    if (self.Deployed && effective_type->UnloadingClass) type = effective_type->UnloadingClass;
    if (!type->Voxel) return;
    Surface* unit_surface = d->unit_voxel_surface;
    if (d->begin_unit_voxel) {
        const auto status = d->begin_unit_voxel(d->context, unit_surface);
        if (!techno_drawing_complete(status)) {
            record_techno_drawing(*d, status);
            return;
        }
    }
    if (!unit_surface && !d->inspect_voxel_parts) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    const auto world_point = point;
    const auto world_clip = clip;
    struct CompositeScope {
        PresentationContext& ctx;
        TechnoDrawing& drawing;
        Surface* source;
        Surface* target;
        bool previous;
        bool software;
        bool restored = false;
        bool cleaned = false;
        void restore() noexcept {
            ctx.unit_terrain_palette = previous;
            drawing.voxel_surface = target;
            restored = true;
        }
        void clear() noexcept {
            if (cleaned) return;
            cleaned = true;
            if (source && software) try {
                    auto bounds = drawing.composite_rect;
                    if (!techno_drawing_complete(drawing.status)) source->GetRect(&bounds);
                    source->FillRect(&bounds, 0);
                } catch (...) {
                    record_techno_drawing(drawing, DrawingStatus::backend_failure);
                }
        }
        ~CompositeScope() {
            if (!restored) restore();
            clear();
        }
    } composite{ctx, *d, unit_surface, d->voxel_surface, ctx.unit_terrain_palette, d->unit_voxel_software};
    d->voxel_surface = unit_surface;
    ctx.unit_terrain_palette = true;
    d->composite_rect = {};
    point = {128, 128};
    clip = {0, 0, 256, 256};
    if (unit_surface) unit_surface->GetRect(&clip);
    VoxelIndexKey key(d->voxel_cache_enabled ? 0 : -1);
    auto body = (effective_type == self.Type ? self.Locomotor->Draw_Matrix(&key)
            : locomotor_drawing_matrix(self.Locomotor, &key, *effective_type));
    const unsigned body_frame =
        ready(type->MainVoxel) ? unsigned(self.WalkedFramesSoFar) % unsigned(type->MainVoxel.HVA->FrameCount) : 0;
    if (key.Is_Valid_Key()) key.Value = std::bit_cast<int>((unsigned(key.Value) << 5) | (body_frame & 0x1F));
    const unsigned shadow_ramp = (unsigned(key.Value) >> 5) & 0x3FFu;
    bool cached_parts = false;
    const auto draw = [&](VoxelStruct& voxel, const Matrix3D& local, unsigned frame,
                          IndexClass<VoxelIndexKey, VoxelCacheStruct*>& cache) {
        if (!ready(voxel) || !techno_drawing_complete(d->status)) return;
        auto matrix = cached_parts ? local : drawing_matrix_product(d->camera, local);
        VoxelMatrixScope scope(*d, local);
        draw_a_vxl(ctx, self, &voxel, int(frame % unsigned(voxel.HVA->FrameCount)), key.Value, &cache, &clip, &point, &matrix,
                   intensity, static_cast<BlitterFlags>(0x2800), 0);
    };
    // 0x73B66C..0x73B742 checks the actual type even when drawing a disguise.
    // Idle/reloading nodes retain the loaded body; the other states use WO.
    const bool empty_launcher = effective_type->NoSpawnAlt && !self.SpawnManager->CountDockedSpawns();
    if (empty_launcher) key.Value |= 0x10000;
    draw(empty_launcher ? type->TurretVoxel : type->MainVoxel, body, unsigned(self.WalkedFramesSoFar), type->VoxelMainCache);
    if (type->Turret) {
        const bool multiple = type->HasMultipleTurrets() && !type->IsGattling;
        const int mode = self.CurrentTurretNumber;
        if (multiple && (mode < 0 || mode >= type->TurretCount || mode >= 18)) {
            record_techno_drawing(*d, DrawingStatus::invalid_argument);
            return;
        }
        auto& turret = multiple ? type->ChargerTurrets[mode] : type->TurretVoxel;
        auto& barrel = multiple ? type->ChargerBarrels[mode] : type->BarrelVoxel;
        unsigned turret_frame = body_frame;
        if (!turret_frame && ready(type->TurretVoxel))
            turret_frame = unsigned(self.TurretAnimFrame) % unsigned(type->TurretVoxel.HVA->FrameCount);
        if (key.Is_Valid_Key()) {
            auto value = (unsigned(key.Value) & ~0x1Fu) | unsigned(self.SecondaryFacing.Current().GetValue<5>());
            if (!effective_type->TurretOffset) value &= ~0x3E0u;
            key.Value = std::bit_cast<int>(value | ((turret_frame & 0xFFu) << 16));
            if (multiple) key.Value = std::bit_cast<int>(unsigned(key.Value) | (unsigned(mode) << 24));
        }
        VoxelCacheStruct* cached_turret = nullptr;
        VoxelCacheStruct* cached_barrel = nullptr;
        const bool turret_cached =
            key.Is_Valid_Key() && cached_voxel(type->VoxelTurretWeaponCache, key, cached_turret) && cached_turret;
        const bool barrel_cached =
            !ready(barrel) ||
            (key.Is_Valid_Key() && cached_voxel(type->VoxelTurretBarrelCache, key, cached_barrel) && cached_barrel);
        const bool cached = turret_cached && barrel_cached;
        auto cached_matrix = body;
        drawing_translate_axis(body, 0, matrix_store_float(type->TurretOffset * 0.16572815467691362));
        const double secondary = (int(self.SecondaryFacing.Current().GetValue<5>()) - 8) * -0.1963495408493621;
        const double primary = (int(self.PrimaryFacing.Current().GetValue<5>()) - 8) * -0.1963495408493621;
        auto turret_matrix = body;
        turret_matrix.RotateZ(matrix_store_float(secondary - primary));
        auto barrel_matrix = turret_matrix;
        const Vector3D<float> pivot{turret_matrix.row[0][3], turret_matrix.row[1][3], turret_matrix.row[2][3]};
        drawing_translate(barrel_matrix, {-pivot.X, -pivot.Y, -pivot.Z});
        const auto direction = ((unsigned(self.BarrelFacing.Current().Raw) >> 10) + 1u) / 2u & 31u;
        barrel_matrix.RotateY(-matrix_store_float((8 - int(direction)) * 0.19634954084936207));
        drawing_translate(barrel_matrix, pivot);
        if (cached) {
            turret_matrix = cached_matrix;
            barrel_matrix = cached_matrix;
            cached_parts = true;
        }
        const unsigned quadrant = self.SecondaryFacing.Current().GetValue<2>();
        if (quadrant == 0 || quadrant == 3) draw(barrel, barrel_matrix, 0, type->VoxelTurretBarrelCache);
        draw(turret, turret_matrix, multiple ? body_frame : turret_frame, type->VoxelTurretWeaponCache);
        if (quadrant == 1 || quadrant == 2) draw(barrel, barrel_matrix, 0, type->VoxelTurretBarrelCache);
    }
    auto final_clip = world_clip;
    auto waterline = unit_waterline(self);
    if (std::bit_cast<short>(waterline) > 0 || self.IsSinking) {
        auto* tactical = TacticalClass::Instance;
        if (!tactical) {
            record_techno_drawing(*d, DrawingStatus::unavailable);
            return;
        }
        // 0x73BEBD: retain the waterline in world-screen coordinates.
        if (std::bit_cast<short>(waterline) > 0) {
            const int bottom = std::bit_cast<int>(unsigned(int(std::bit_cast<short>(waterline))) -
                                                  unsigned(tactical->TacticalPos.Y));
            const RectangleStruct water_clip{0, 0, TacticalClass::ViewBounds.Width, bottom};
            Drawing::Intersect(&final_clip, water_clip, world_clip, nullptr, nullptr);
        } else {
            set_unit_waterline(self,
                static_cast<unsigned short>(unsigned(d->composite_rect.Y) + unsigned(d->composite_rect.Height) +
                                            unsigned(world_point.Y) + unsigned(tactical->TacticalPos.Y) - 128u));
        }
    }
    intensity = self.GetFlashingIntensity(intensity);
    if (self.IsIronCurtained()) {
        intensity = self.GetEffectTintIntensity(intensity);
        tint |= int(d->iron_tint);
    }
    if (self.Berzerk) tint |= int(d->berserk_tint);
    if (self.IsDeactivated()) intensity /= 2;
    if (techno_drawing_complete(d->status) && !d->inspect_voxel_parts)
        build_unit_composite(ctx, self, composite.target, world_point, final_clip, intensity, tint);
    composite.restore();
    if (!self.IsSinking && !type->NoShadow && ready(type->MainVoxel) && techno_drawing_complete(d->status)) {
        const bool floating =
            effective_type->BalloonHover || effective_type->SpeedType == SpeedType::Track || effective_type->SpeedType == SpeedType::Wheel;
        const auto shadow_point = self.Locomotor->Shadow_Point();
        VoxelIndexKey shadow_key(std::bit_cast<int>(
            floating ? shadow_ramp : (shadow_ramp << 5) | ((unsigned(shadow_point.Y) - 16u) & 0x1Fu)));
        auto local = self.Locomotor->Shadow_Matrix(&shadow_key);
        auto matrix = drawing_matrix_product(d->camera, local);
        auto at = world_point;
        if (self.IsOnCarryall) at.Y -= 14;
        if (floating && !self.IsInAir()) {
            if (self.GetHeight() > 0)
                shadow_key.Invalidate();
            else
                shadow_key.Value = std::bit_cast<int>(unsigned(shadow_key.Value) | 0x80000000u);
        }
        VoxelMatrixScope scope(*d, local);
        auto shadow_clip = world_clip;
        auto shadow_offset = shadow_point;
        // 0x73C4CD biases the airborne body's exclusion sample, not the shadow position.
        if (self.GetHeight() > 0) shadow_offset.Y = std::bit_cast<int>(unsigned(shadow_offset.Y) + 1u);
        draw_foot_voxel_shadow(ctx, self, &type->MainVoxel, type->ShadowIndex, shadow_key, &type->VoxelShadowCache, &shadow_clip, &at,
                        &matrix, true, self.IsInAir() ? nullptr : unit_surface, shadow_offset);
    }
    composite.clear();
}


}
