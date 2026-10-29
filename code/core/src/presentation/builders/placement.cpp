// YR 0x6D5030 ordinary foundation traversal, called in both original draw passes.
#include "yrpp/TacticalClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "tactical_drawing.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"

#include "presentation/context.hpp"
#include "presentation/builders/placement.hpp"
namespace game {
void build_placement(PresentationContext& ctx, const TacticalClass& self, bool foreground) {
    auto* frame = ctx.frame;
    if (!frame || !frame->world) return;
    auto& world = *frame->world->impl;
    auto& display = DisplayClass::Instance;
    const auto bounds = frame->bounds;
    if (display.CurrentBuildingType) {
        const auto point = world.view.pointer;
        if (!world.view.pointer_inside || point.X < bounds.X || point.Y < bounds.Y ||
            point.X >= bounds.X + bounds.Width || point.Y >= bounds.Y + bounds.Height)
            return;
    }
    const auto draw = [&](CellStruct center, CellStruct offset, CellStruct* foundation) {
        if (!foundation) return false;
        bool valid = true;
        const auto size = display.FoundationBoundsSize(foundation);
        for (int x = 0; x < size.X; ++x)
            for (int y = 0; y < size.Y; ++y) {
                const CellStruct at{short(center.X + offset.X + x), short(center.Y + offset.Y + y)};
                if (!display.CoordinatesLegal(at)) continue;
                auto* cell = display.TryGetCellAt(at);
                if (!cell) continue;
                const auto flat = TacticalClass::CoordsToScreen({at.X * 256, at.Y * 256, 0});
                if (!build_cell_placement(ctx, *cell, {flat.X - self.TacticalPos.X - 30, flat.Y - self.TacticalPos.Y}, bounds, foreground))
                    valid = false;
            }
        return valid;
    };
    if (display.CurrentBuildingType) {
        const bool valid = draw(display.CurrentFoundation_CenterCell, display.CurrentFoundation_TopLeftOffset,
                                display.CurrentFoundation_Data);
        // 0x006D527C..0x006D53CB: editor placement previews use DrawAgain.
        if (Unsorted::ArmageddonMode && display.CurrentBuilding &&
            display.CurrentBuilding->WhatAmI() == AbstractType::Building) {
            const auto center = display.CurrentFoundation_CenterCell, offset = display.CurrentFoundation_TopLeftOffset;
            const int gateOffset = static_cast<BuildingClass*>(display.CurrentBuilding)->Type->Gate ? -1 : 0;
            CoordStruct at{int(short(center.X + offset.X + gateOffset)) * 256,
                           int(short(center.Y + offset.Y + gateOffset)) * 256, 0};
            at.Z = display.GetCellFloorHeight(at);

            const auto screen = TacticalClass::CoordsToScreen(at);
            game::record_tactical_drawing(ctx, 
                game::draw_tactical_building_preview(ctx, *static_cast<BuildingClass*>(display.CurrentBuilding),
                                                     {screen.X - self.TacticalPos.X, screen.Y - self.TacticalPos.Y}, bounds));
        }
        // 0x6D5330..0x6D547B: only a valid active foundation previews wall links.
        if (valid && display.CurrentBuilding && display.CurrentBuilding->WhatAmI() == AbstractType::Building) {
            const auto* type = static_cast<BuildingClass*>(display.CurrentBuilding)->Type;
            if (!type->LaserFencePost && !type->FirestormWall && type->ToOverlay && type->ToOverlay->Wall) {
                const auto center = display.CurrentFoundation_CenterCell;
                const auto offset = display.CurrentFoundation_TopLeftOffset;
                build_wall_placement(ctx, self, foreground, {short(center.X + offset.X), short(center.Y + offset.Y)});
            }
        }
    }
    if (!Unsorted::ArmageddonMode && display.CurrentBuildingTypeCopy)
        draw(display.CurrentFoundationCopy_CenterCell, display.CurrentFoundationCopy_TopLeftOffset,
             display.CurrentFoundationCopy_Data);
}
void build_wall_placement(PresentationContext& ctx, const TacticalClass& self, bool foreground, CellStruct center) {
    auto* type = static_cast<BuildingClass*>(DisplayClass::Instance.CurrentBuilding)->Type;
    const auto* overlay = type->ToOverlay;
    if (!overlay) return;
    auto* frame = ctx.frame;
    if (!frame || !frame->world || !frame->drawing) return;
    auto& world = *frame->world->impl;
    if (!DisplayClass::PlaceShape) return;
    const auto& context = *frame->drawing;
    auto& map = MapClass::Instance;
    auto* owner = HouseClass::CurrentPlayer;
    for (int direction = 0; direction < 8; direction += 2) {
        const auto step = Unsorted::AdjacentCell[direction];
        auto at = center;
        int count = 0;
        bool connected = false;
        while (count < (type->GuardRange >> 8)) {
            at = {short(at.X + step.X), short(at.Y + step.Y)};
            const auto* cell = map.GetCellAt(at);
            if (cell->OverlayTypeIndex != -1 && cell->OverlayTypeIndex == overlay->ArrayIndex &&
                cell->WallOwnerIndex == owner->ArrayIndex) {
                connected = true;
                break;
            }
            if (!cell->CanThisExistHere(type->SpeedType, type, owner)) break;
            ++count;
        }
        if (!connected) continue;
        at = center;
        for (int i = 0; i < count; ++i) {
            at = {short(at.X + step.X), short(at.Y + step.Y)};
            const int level = static_cast<signed char>(map.GetCellAt(at)->Level);
            const auto point = TacticalClass::CoordsToScreen({at.X * 256, at.Y * 256, 0});
            if (!context.plain_palette) {
                game::record_tactical_drawing(ctx, game::DrawingStatus::unavailable);
                return;
            }
            game::ShapeDrawingRequest request;
            const auto status =
                context.plain_palette(context.types.backend_context, world.presentation.selection_palette, request.palette);
            if (status != game::DrawingStatus::drawn) {
                game::record_tactical_drawing(ctx, status);
                return;
            }
            request.target = context.types.target;
            request.image = DisplayClass::PlaceShape;
            request.frame = 0;
            request.position = {point.X - self.TacticalPos.X, point.Y - self.TacticalPos.Y - 1 - 15 * level};
            request.clip = frame->bounds;
            request.flags = foreground ? 0x10606 : 0x20606;
            request.depth_adjustment = -2 - 15 * level;
            game::record_tactical_drawing(ctx, game::submit_type_shape(context.types, request));
        }
    }
}
bool build_cell_placement(PresentationContext& ctx, const CellClass& self, const Point2D& point, const RectangleStruct& clip, bool foreground) {
    const unsigned flags = unsigned(self.AltFlags);
    if (!(flags & 6u)) return false;
    auto& display = DisplayClass::Instance;
    auto* type = (flags & 2u) ? display.CurrentBuildingType : display.CurrentBuildingTypeCopy;
    bool valid = false;
    if ((flags & 2u) && type && type->WhatAmI() == AbstractType::BuildingType) {
        auto* building = static_cast<BuildingTypeClass*>(type);
        const auto at = self.GetCoords();
        valid = !display.IsLocationShrouded(at) && display.unknown_1180 &&
                self.CanThisExistHere(building->SpeedType, building, HouseClass::CurrentPlayer);
    }
    auto* frame = ctx.frame;
    if (!frame || !frame->world || !frame->drawing) return valid;
    auto& world = *frame->world->impl;
    if (!DisplayClass::PlaceShape) return valid;
    const auto& context = *frame->drawing;
    if (!context.plain_palette) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::unavailable);
        return valid;
    }
    game::ShapeDrawingRequest request;
    auto status = context.plain_palette(context.types.backend_context, world.presentation.selection_palette, request.palette);
    if (status != game::DrawingStatus::drawn) {
        game::record_tactical_drawing(ctx, status);
        return valid;
    }
    const int level = static_cast<signed char>(self.Level);
    request.target = context.types.target;
    request.image = DisplayClass::PlaceShape;
    request.frame = (flags & 4u) ? 1 : valid ? 0 : int(self.SlopeIndex) + 2;
    request.position = {point.X + 30, point.Y - 1 - 15 * level};
    request.clip = clip;
    request.flags = foreground ? 0x10600 : 0x20600;
    request.depth_adjustment = -2 - 15 * level - (self.SlopeIndex ? 10 : 0);
    game::record_tactical_drawing(ctx, game::submit_type_shape(context.types, request));
    return valid;
}
}
