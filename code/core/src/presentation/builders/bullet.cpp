// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/bullet.cpp Draw_It; YR 0x468090 adds Shadow,
// SpawnNextAnim, FirersPalette and the original 32-facing frame lookup.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/HouseClass.h"
#include "type_drawing.hpp"
#include "yrpp/Matrix3D.h"
#include "yrpp/YRMath.h"
#include "yrpp/RulesClassReaders.hpp"
#include "yrpp/Matrix3DArithmetic.hpp"

#include "presentation/builders/bullet.hpp"
namespace game {
bool build_bullet_voxel(PresentationContext& ctx, const BulletClass& self, const VoxelStruct& voxel, const Matrix3D& matrix, const Point2D& point,
                            const RectangleStruct& clip, unsigned frame, unsigned flags, int intensity) {
    const auto* resources = ctx.resources;
    if (!resources || !resources->voxel) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::unavailable);
        return false;
    }
    game::ObjectVoxelRequest r;
    r.resource = &voxel;
    r.matrix = matrix;
    r.position = point;
    r.clip = clip;
    r.frame = frame;
    r.flags = flags & ~0x10u;
    r.intensity = intensity;
    r.flat = self.Type->Flat;
    // 0x46B1B9 selects the original flat-voxel depth offset at 0x81AF00.
    if (r.flat) {
        r.flags |= 0x2000u;
        r.depth_adjustment = -65;
    }
    if (!game::resolve_drawing_palette(ctx, game::DrawingPaletteKind::color_scheme, self.Type->Color, r.palette)) return false;
    const auto status = resources->voxel(resources->context, r);
    game::record_type_drawing_result(ctx, status);
    return status == game::DrawingStatus::drawn;
}
void build_bullet(PresentationContext& ctx, const BulletClass& self, Point2D* point, RectangleStruct* clip) {
    const auto* context = ctx.type;
    if (!context) return;
    if (!point || !clip) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::invalid_argument);
        return;
    }
    if (!self.Type || self.Type->Inviso || self.SpawnNextAnim) return;
    auto* cell = self.GetCell();
    if (ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar && cell->IsFogged()) return;
    if (self.Type->Voxel) {
        const auto* resources = ctx.resources;
        if (!resources || !resources->voxel_camera) {
            game::record_type_drawing_result(ctx, game::DrawingStatus::unavailable);
            return;
        }
        // 0x468137..0x468205 quantizes both velocity angles to 32 facings.
        const auto angle = [](double radians) {
            const auto direction =
                static_cast<unsigned short>(rule_integer((radians - 1.570796326794897) * -10430.06004058427));
            return double(int((((unsigned(direction) >> 10) + 1) >> 1) & 0x1F) - 8) * -0.1963495408493621;
        };
        auto local = Matrix3D::GetIdentity();
        local.RotateZ(matrix_store_float(angle(Math::atan2(-self.Velocity.Y, self.Velocity.X))));
        local.RotateY(matrix_store_float(
            -angle(Math::atan2(self.Velocity.Z, Math::sqrt(self.Velocity.X * self.Velocity.X + self.Velocity.Y * self.Velocity.Y)))));
        const auto matrix = drawing_matrix_product(*resources->voxel_camera, local);
        build_bullet_voxel(ctx, self, self.Type->MainVoxel, matrix, *point, *clip, 0, 0x800, 1000);
        return;
    }
    auto* image = self.GetImage();
    if (!image) return;
    const int frame = self.GetAnimFrame();
    int height = self.GetHeight(), floor = MapClass::Instance.GetCellFloorHeight(self.Location);
    if (!self.OnBridge && (unsigned(cell->Flags) & 0x100) && height >= CellClass::BridgeHeight) {
        height -= CellClass::BridgeHeight;
        floor += CellClass::BridgeHeight;
    }
    game::ShapeDrawingRequest r;
    r.target = context->target;
    r.palette = context->palette;
    r.image = image;
    r.frame = frame;
    r.position = *point;
    r.clip = *clip;
    r.gradient = 0;
    r.intensity = 1000;
    if (height > 0 && self.Type->Shadow) {
        // 0x00468374 always uses NormalDrawer, independently of the body.
        if (!game::resolve_drawing_palette(ctx, game::DrawingPaletteKind::normal, -1, r.palette)) return;
        r.position.Y += TacticalClass::AdjustForZ(height);
        r.flags = 0x2601;
        r.depth_adjustment = -10 - TacticalClass::AdjustForZ(floor);
        const auto status = game::submit_type_shape(*context, r);
        game::record_type_drawing_result(ctx, status);
        if (!game::drawing_completed(status)) return;
    }
    // 0x0046837F..0x004683D1: AnimPalette takes precedence. InheritedColor
    // indexes ColorScheme, not House; only -1 selects the current player's index.
    auto palette = game::DrawingPaletteKind::normal;
    int color = -1;
    if (self.Type->AnimPalette)
        palette = game::DrawingPaletteKind::animation;
    else if (self.Type->FirersPalette) {
        palette = game::DrawingPaletteKind::color_scheme;
        color = self.InheritedColor;
        if (color == -1) {
            if (!HouseClass::CurrentPlayer) {
                game::record_type_drawing_result(ctx, game::DrawingStatus::unavailable);
                return;
            }
            color = HouseClass::CurrentPlayer->ColorSchemeIndex;
        }
    }
    if (!game::resolve_drawing_palette(ctx, palette, color, r.palette)) return;
    r.position = *point;
    r.flags = 0x2E00;
    r.depth_adjustment = -30 - TacticalClass::AdjustForZ(self.Location.Z);
    game::record_type_drawing_result(ctx, game::submit_type_shape(*context, r));
}
}
