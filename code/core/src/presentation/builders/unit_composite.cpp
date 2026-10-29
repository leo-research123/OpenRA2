// YR 0x0073C860: final unit voxel composition.
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
void build_unit_composite(PresentationContext& ctx, const UnitClass& self, Surface* target, Point2D point, RectangleStruct clip, int intensity, int tint) {
    auto* d = ctx.techno;
    if (!d || !techno_drawing_complete(d->status)) return;
    if (!d->submit_unit_voxel || !d->voxel_surface) {
        record_techno_drawing(*d, DrawingStatus::unavailable);
        return;
    }
    auto* cell = techno_drawing_cell(self);
    if (cell && techno_cell_shrouded(*cell)) tint = 0;
    unsigned flags = 0x2800;
    int ripple = 0;
    switch (drawing_visual_character(ctx, self)) {
        case VisualType::Indistinct:
            flags |= 2;
            break;
        case VisualType::Darken:
        case VisualType::Shadowy:
            flags |= 4;
            break;
        case VisualType::Ripple:
            flags |= self.CloakProgress.Value ? 0xC : 0xA;
            ripple = (self.Fetch_ID() + x87_integer(self.WarpFactor)) % 400;
            break;
        default:
            break;
    }
    if (self.IsBeingWarpedOut() || self.IsWarpingIn()) flags |= 4;
    if (self.Owner && self.Owner->IsControlledByCurrentPlayer() && self.IsDisguised()) flags = self.GetDisguiseFlags(flags);
    UnitVoxelCompositeRequest r;
    r.object = (&self);
    r.source = d->voxel_surface;
    r.target = target;
    r.source_rect = d->composite_rect;
    r.clip = clip;
    r.position = {point.X + r.source_rect.X - 128, point.Y + r.source_rect.Y - 128};
    r.flags = flags;
    r.intensity = intensity;
    r.tint = unsigned(tint);
    r.ripple = ripple;
    r.gradient = int(self.GetZGradient());
    r.depth = drawing_z_adjustment(ctx, self);
    if (!techno_drawing_palette(*d, self, TechnoPalette::house, cell, self.Owner, r.palette)) return;
    record_techno_drawing(*d, d->submit_unit_voxel(d->context, r));
}
}
