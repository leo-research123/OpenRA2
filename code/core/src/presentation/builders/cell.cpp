// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 additional terms: code/third_party/opents/LICENSE.md.
// Cell drawing: OpenTS cell.cpp at 44fac744f70235e0d5ddca107364a68f95132ce9. YR 0x00480350 (terrain and smudge
// submission, CellClass::DrawIt), 0x0047F6A0 / 0x0047F510 / 0x00480110 (overlay and overlay shadow: the theater
// converter for ore/veins, crates/rubble, no tiberium frame checks on the shadow path), 0x004802A0 (shadow caster),
// 0x00547230 (IsometricTileTypeClass::DrawShadowCaster), 0x006D2DE0 / 0x006D3040 (the tile and tile-shadow passes).
#include "presentation/builders/cell.hpp"
#include "yrpp/CellClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Surface.h"
#include "yrpp/LightConvertClassHelpers.hpp"
#include "tactical_cells.hpp"
#include "tactical_drawing.hpp"
#include "map_runtime.hpp"
#include "map_world_internal.hpp"
#include "overlay_drawing.hpp"
#include "verify/draw_coverage.hpp"
#include <algorithm>
#include <bit>

namespace game {
namespace {
// Borrow the actual cell and the same resolved LightConvert palette as its TMP.
// This scope lasts only through SmudgeTypeClass::DrawIt's synchronous submit.
struct SmudgeCellDrawing {
    const CellClass& cell;
    const game::DrawingPaletteHandle* palette;
};
game::DrawingStatus smudge_cell(void* pointer, const CellStruct& coordinates,
                                const game::DrawingPaletteHandle*& palette, int& intensity) noexcept {
    const auto& state = *static_cast<const SmudgeCellDrawing*>(pointer);
    if (coordinates != state.cell.MapCoords) return game::DrawingStatus::unavailable;
    palette = state.palette;
    intensity = std::bit_cast<std::int16_t>(state.cell.Intensity_Terrain);
    return game::DrawingStatus::drawn;
}
game::DrawingStatus smudge_height(void*, int height, int& pixels) noexcept {
    pixels = TacticalClass::AdjustForZ(height);
    return game::DrawingStatus::drawn;
}
}


void build_cell_tile(PresentationContext& ctx, CellClass& self, const Point2D& unraised, const RectangleStruct& clip, bool skip) {
    auto* frame = ctx.frame;
    if (!frame || !frame->drawing || skip) return;
    // 0x00480384: a cell drawn before any light pass borrows its converter now.
    if (!self.LightConvert) self.InitLightConvert(nullptr, 0x10000, 0, 1000, 1000, 1000);
    const auto& context = *frame->drawing;
    const auto draw = [&]() noexcept -> game::DrawingStatus {
        using game::DrawingStatus;
        if (clip.Width <= 0 || clip.Height <= 0) {
            game::cull_background_item(ctx);
            return DrawingStatus::skipped;
        }
        if (!context.terrain_palette) return DrawingStatus::unavailable;
        try {
            IsometricTileTypeClass* base = nullptr;
            int variant = 0;
            if (!self.GetTerrainTile(base, variant)) return DrawingStatus::unavailable;
            auto* owner = base;
            for (int i = 0; i < variant && owner; ++i)
                owner = owner->NextVariant;
            if (!owner) return DrawingStatus::unavailable;
            const auto* tmp = reinterpret_cast<const TMPStruct*>(owner->GetImage());
            if (!tmp || tmp->Columns <= 0 || tmp->Rows <= 0) return DrawingStatus::unavailable;
            const int sub = self.IsoTileTypeIndex == 0xffff ? 0 : static_cast<unsigned char>(self.Height);
            const TMPImage* image = nullptr;
            if (!tmp->GetSubTile(sub % (tmp->Columns * tmp->Rows), image)) return DrawingStatus::skipped;
            const int level = static_cast<signed char>(self.Level);
            const std::int64_t x = unraised.X,
                               y = std::int64_t(unraised.Y) - 15 * level + context.types.overlay_offset.Y;
            auto left = x, top = y, right = x + 60, bottom = y + 30;
            if (image->Flags & 1) {
                const auto ex = x + image->ExtraX - image->X, ey = y + image->ExtraY - image->Y;
                left = std::min(left, ex);
                top = std::min(top, ey);
                right = std::max(right, ex + image->ExtraWidth);
                bottom = std::max(bottom, ey + image->ExtraHeight);
            }
            const bool tile_visible = right > clip.X && bottom > clip.Y && left < std::int64_t(clip.X) + clip.Width &&
                                      top < std::int64_t(clip.Y) + clip.Height;
            auto* smudge = SmudgeTypeClass::Array.GetItemOrDefault(self.SmudgeTypeIndex);
            // The clip decides what this cell submits: a recorded scene item must not
            // stand for the cell at a view where the TMP becomes visible.
            if (!tile_visible) game::cull_background_item(ctx);
            // A multi-cell smudge can remain visible when its owning TMP is offscreen.
            // Its original data offset and SHP bounds are handled by DrawIt/backend.
            if (!tile_visible && !smudge) return DrawingStatus::skipped;
            if (y < INT32_MIN || y > INT32_MAX) return DrawingStatus::invalid_argument;
            // DrawIt (0x00480350) passes the cell's converter to the TMP draw; its table
            // follows from the converter's tint and shade count.
            int red, green, blue, shades;
            if (!game::cell_converter_tint(self, red, green, blue, shades)) {
                red = self.Color2_Red;
                green = self.Color2_Green;
                blue = self.Color2_Blue;
                shades = red == 1000 && green == 1000 && blue == 1000
                             ? 53
                             : LightConvertClass::PrepareCellTint(red, green, blue, context.lighting_quality);
            }
            auto types = context.types;
            const auto palette = context.terrain_palette(types.backend_context, FileSystem::ISOx_PAL, red, green, blue,
                                                         shades, types.palette);
            if (palette != DrawingStatus::drawn) return palette;
            auto result = DrawingStatus::skipped;
            if (tile_visible) {
                result = game::draw_isometric_tile(*base, types, sub, static_cast<int>(x), static_cast<int>(y), clip,
                                                   level, std::bit_cast<std::int16_t>(self.Intensity_Terrain), true, variant,
                                                   false, false, false, 0);
                if (result != DrawingStatus::drawn && result != DrawingStatus::skipped) return result;
            }
            if (smudge) {
                // 0x004804A2: immediately after this cell's TMP, at its top vertex.
                // SHP takes clip-relative coordinates; TMP takes absolute coordinates.
                const auto sx = x + 30 - clip.X, sy = y - clip.Y;
                if (sx < INT32_MIN || sx > INT32_MAX || sy < INT32_MIN || sy > INT32_MAX)
                    return DrawingStatus::invalid_argument;
                SmudgeCellDrawing state{self, types.palette};
                types.world_context = &state;
                types.smudge_cell = smudge_cell;
                types.height_to_pixels = smudge_height;
                const auto drawn = game::draw_smudge_type(*smudge, types, {static_cast<int>(sx), static_cast<int>(sy)},
                                                          clip, self.SmudgeData, level * Unsorted::LevelHeight, self.MapCoords);
                if (drawn != DrawingStatus::skipped) return drawn;
            }
            return result;
        } catch (...) {
            return DrawingStatus::backend_failure;
        }
    };
    game::record_tactical_drawing(ctx, draw());
}

namespace {
int add(int a, int b) noexcept { return std::bit_cast<int>(static_cast<unsigned>(a) + static_cast<unsigned>(b)); }
int subtract(int a, int b) noexcept { return std::bit_cast<int>(static_cast<unsigned>(a) - static_cast<unsigned>(b)); }
bool initialize_light(CellClass& cell, game::OverlayDrawing& drawing) {
    if (cell.LightConvert) return true;
    if (!drawing.initialize_light) {
        drawing.status = game::DrawingStatus::unavailable;
        return false;
    }
    drawing.status = drawing.initialize_light(drawing.context, cell);
    return drawing.status == game::DrawingStatus::drawn;
}
void submit(CellClass& cell, game::OverlayDrawing& drawing, game::OverlayPalette palette, SHPStruct* image, int frame,
            const Point2D& point, const RectangleStruct& clip, unsigned flags, int depth, int gradient, int intensity,
            SHPStruct* depth_image = nullptr, Point2D depth_offset = {}) {
    game::ShapeDrawingRequest request;
    request.target = drawing.types.target;
    if (!drawing.palette) {
        drawing.status = game::DrawingStatus::unavailable;
        return;
    }
    drawing.status = drawing.palette(drawing.context, cell, palette, request.palette);
    if (drawing.status != game::DrawingStatus::drawn) return;
    request.image = image;
    request.frame = frame;
    request.position = point;
    request.clip = clip;
    request.flags = flags;
    request.depth_adjustment = depth;
    request.gradient = gradient;
    request.intensity = intensity;
    request.depth_image = depth_image;
    request.depth_offset = depth_offset;
    drawing.status = game::submit_type_shape(drawing.types, request);
}
bool slope_image(game::OverlayDrawing& drawing, unsigned slope, SHPStruct*& image) {
    // The original table contains exactly four slope resources. Malformed
    // host cells must fail explicitly instead of reading unrelated globals.
    if (slope > 4) {
        drawing.status = game::DrawingStatus::invalid_argument;
        return false;
    }
    image = drawing.slope_depth[slope];
    if (slope && !image) {
        drawing.status = game::DrawingStatus::unavailable;
        return false;
    }
    return true;
}
}

void build_cell_overlay(PresentationContext& ctx, CellClass& self, const Point2D& location, const RectangleStruct& clip) {
    auto* drawing = ctx.overlay;
    if (!drawing) return;  // Hosts bind the internal synchronous drawing contract.
    if (self.OverlayTypeIndex == 167 || self.OverlayTypeIndex == 178) return;
    auto* type = OverlayTypeClass::Array.GetItemOrDefault(self.OverlayTypeIndex);
    if (!type) {
        drawing->status = game::DrawingStatus::invalid_argument;
        return;
    }
    game::draw_coverage_instance(game::DrawCategory::overlay);
    Point2D point;
    point = cell_overlay_offset(self, ctx.overlay->tactical_rect.Y);
    point.X = subtract(add(point.X, location.X), clip.X);
    point.Y = subtract(add(point.Y, location.Y), clip.Y);
    const bool bridge = (static_cast<unsigned>(self.Flags) & 0x80u) != 0;
    const int height = 15 * (static_cast<signed char>(self.Level) + (bridge ? 4 : 0));
    if (!initialize_light(self, *drawing)) return;
    auto* image = type->GetImage();
    if (bridge) {
        const auto& rect = self.InViewportRect;
        if (self.RedrawFrame == static_cast<unsigned>(drawing->frame) &&
            static_cast<unsigned char>(self.unknown_118) == drawing->redraws && rect.X == clip.X && rect.Y == clip.Y &&
            rect.Width == clip.Width && rect.Height == clip.Height) {
            drawing->status = game::DrawingStatus::skipped;
            return;
        }
        constexpr int variations[16] = {0, 1, 2, 3, 3, 2, 1, 0, 2, 3, 0, 1, 1, 0, 3, 2};
        int frame = self.OverlayData;
        if (frame == 0 || frame == 9) frame += variations[(self.MapCoords.X & 3) | ((self.MapCoords.Y & 3) << 2)];
        submit(self, *drawing, game::OverlayPalette::cell, image, frame, point, clip, 0x4E00, -2 - height, 0,
               std::bit_cast<short>(self.Color1_Blue));
        // Original writes TacticalRect, not the potentially smaller clip.
        // A host failure is retryable and must not mark a lost request as drawn.
        if (drawing->status == game::DrawingStatus::drawn || drawing->status == game::DrawingStatus::skipped) {
            self.RedrawFrame = static_cast<unsigned>(drawing->frame);
            self.InViewportRect = drawing->tactical_rect;
            self.unknown_118 = static_cast<char>(drawing->redraws);
        }
        return;
    }
    if (type->Tiberium) {
        const int resource_index = TiberiumClass::FindIndex(self.OverlayTypeIndex);
        if (resource_index == -1) {
            drawing->status = game::DrawingStatus::skipped;
            return;
        }
        auto* resource = TiberiumClass::Array.GetItemOrDefault(resource_index);
        if (!resource) {
            drawing->status = game::DrawingStatus::invalid_argument;
            return;
        }
        const int variants = self.SlopeIndex ? resource->NumSlopes / 4 : resource->NumImages;
        if (!resource->Image || variants <= 0) {
            drawing->status = game::DrawingStatus::invalid_argument;
            return;
        }
        int index = resource->Image->ArrayIndex + (self.MapCoords.X * self.MapCoords.Y) % variants;
        if (self.SlopeIndex) index += resource->NumImages + variants * (self.SlopeIndex - 1);
        auto* variant = OverlayTypeClass::Array.GetItemOrDefault(index);
        if (!variant) {
            drawing->status = game::DrawingStatus::invalid_argument;
            return;
        }
        image = variant->GetImage();
        SHPStruct* depth = nullptr;
        if (!slope_image(*drawing, self.SlopeIndex, depth)) return;
        submit(self, *drawing, game::OverlayPalette::theater, image, self.OverlayData, point, clip, 0x4E00, -2 - height, 0,
               1000, depth);
    } else if (type->Wall) {
        submit(self, *drawing, game::OverlayPalette::wall, image, self.OverlayData, point, clip, 0x4E00, -2 - height, 2,
               std::bit_cast<short>(self.Intensity_Normal));
    } else if (type->ArrayIndex == 126) {
        SHPStruct* depth = nullptr;
        if (!slope_image(*drawing, self.SlopeIndex, depth)) return;
        submit(self, *drawing, game::OverlayPalette::theater, image, self.OverlayData, point, clip, 0x4E00, -2 - height, 0,
               std::bit_cast<short>(self.Intensity_Terrain), depth, self.SlopeIndex ? Point2D{30, -2} : Point2D{});
    } else {
        const int bias = type->DrawFlat || type->IsARock ? 0 : -15;
        int frame = self.OverlayData;
        if (type->Crate)
            frame = 0;
        else if (type->IsRubble && (!self.Rubble || !self.Rubble->GetRubbleShape(&image, &frame))) {
            drawing->status = game::DrawingStatus::skipped;
            return;
        }
        submit(self, *drawing, game::OverlayPalette::cell, image, frame, point, clip, 0x4E00, bias - height - 2,
               type->DrawFlat ? 0 : 2, std::bit_cast<short>(self.Intensity_Terrain));
    }
}

void build_cell_overlay_shadow(PresentationContext& ctx, CellClass& self, const Point2D& location, const RectangleStruct& clip) {
    auto* drawing = ctx.overlay;
    if (!drawing) return;
    auto* type = OverlayTypeClass::Array.GetItemOrDefault(self.OverlayTypeIndex);
    if (!type) {
        drawing->status = game::DrawingStatus::invalid_argument;
        return;
    }
    const int height = 15 * static_cast<signed char>(self.Level);
    auto* image = type->GetImage();
    Point2D point;
    point = cell_overlay_offset(self, ctx.overlay->tactical_rect.Y);
    point.X = subtract(add(point.X, location.X), clip.X);
    point.Y = subtract(add(point.Y, location.Y), clip.Y);
    if ((static_cast<unsigned>(self.Flags) & 0x80u) && self.OverlayData >= 9 && self.OverlayData <= 17) {
        point.X = add(point.X, -15);
        point.Y = add(point.Y, 7);
    }
    if (!initialize_light(self, *drawing)) return;
    int frame = type->Crate ? 0 : self.OverlayData;
    if (!type->Crate && type->IsRubble) {
        if (!self.Rubble || !self.Rubble->GetRubbleShadowShape(&image, &frame)) {
            drawing->status = game::DrawingStatus::skipped;
            return;
        }
    } else {
        if (!image) {
            drawing->status = game::DrawingStatus::unavailable;
            return;
        }
        // GetImage resolves OverlayType's image. The YR entry reads the SHP
        // header directly; it does not pick the ore/body coordinate variant.
        frame += image->Frames / 2;
    }
    submit(self, *drawing, game::OverlayPalette::cell, image, frame, point, clip, 0x4601, -2 - height, 0, 1000);
}

namespace {
struct Caster {
    int frame, index, x, y;
};
constexpr Caster cliffs[40] = {
    {0, 0, 0, 0},     // 0
    {0, 0, 0, 0},     // 1
    {0, 0, 0, 0},     // 2
    {0, 0, 0, 0},     // 3
    {0, 0, 0, 0},     // 4
    {0, 0, 0, 0},     // 5
    {0, 0, 0, 0},     // 6
    {0, 0, 0, 0},     // 7
    {0, 0, 0, 0},     // 8
    {0, 0, 0, 0},     // 9
    {0, 0, 0, 0},     // 10
    {0, 0, 0, 0},     // 11
    {0, 0, 0, 0},     // 12
    {0, 0, 0, 0},     // 13
    {0, 0, 0, 0},     // 14
    {0, 0, 0, 0},     // 15
    {0, 0, 0, 0},     // 16
    {0, 0, 30, 30},   // 17
    {0, 0, 30, 30},   // 18
    {0, 0, 30, 30},   // 19
    {1, 0, 30, 30},   // 20
    {1, 0, 30, 30},   // 21
    {2, 1, 60, 15},   // 22
    {3, 1, 60, 15},   // 23
    {4, 1, 60, 15},   // 24
    {5, 0, 90, 30},   // 25
    {6, 0, 60, 15},   // 26
    {7, 1, 30, 0},    // 27
    {8, 0, 60, 15},   // 28
    {9, 0, 60, 15},   // 29
    {10, 1, 0, -15},  // 30
    {11, 1, 0, -15},  // 31
    {12, 0, 60, 15},  // 32
    {0, 0, 0, 0},     // 33
    {0, 0, 0, 0},     // 34
    {0, 0, 0, 0},     // 35
    {0, 0, 0, 0},     // 36
    {0, 0, 0, 0},     // 37
    {0, 0, 0, 0},     // 38
    {0, 0, 0, 0},     // 39
};
constexpr Caster slopes[10] = {{}, {}, {}, {}, {13, 6, 48, 12}, {}, {14, 1, 48, 12}, {}, {}, {}};
}
void build_tile_shadow_caster(PresentationContext& ctx, IsometricTileTypeClass& self, int sub, Point2D point, RectangleStruct clip, int depth) {
    auto* frame = ctx.frame;
    if (!frame || !frame->world) return;
    const Caster* caster = nullptr;
    if (self.ArrayIndex >= self.SlopeSetPieces && self.ArrayIndex < self.SlopeSetPieces + 10)
        caster = &slopes[self.ArrayIndex - self.SlopeSetPieces];
    else if (self.ArrayIndex >= self.SlopeSetPieces2 && self.ArrayIndex < self.SlopeSetPieces2 + 10)
        caster = &slopes[self.ArrayIndex - self.SlopeSetPieces2];
    else if (self.ShadowCaster)
        for (int first : self.ShadowTileSets)
            if (unsigned(self.ArrayIndex - first) < 40u) {
                caster = &cliffs[self.ArrayIndex - first];
                break;
            }
    if (!caster || !caster->frame || caster->index != sub) return;
    try {
        auto* image = static_cast<SHPStruct*>(FileSystem::LoadFile("C_SHADOW.SHP", true));
        const auto bounds = frame->bounds.Width > 0 ? frame->bounds : TacticalClass::ViewBounds;
        point.X += bounds.X + 30 + caster->x - clip.X;
        point.Y += bounds.Y + 15 + caster->y - clip.Y;
        if (!image) { record_tactical_drawing(ctx, DrawingStatus::unavailable); return; }
        ShapeDrawingRequest request;
        request.image = image;
        request.frame = caster->frame - 1;
        request.position = point;
        request.palette = reinterpret_cast<const DrawingPaletteHandle*>(&FileSystem::ISOx_PAL);
        request.flags = 0x4601;
        request.gradient = 0;
        request.depth_adjustment = depth;
        request.intensity = 1000;
        record_tactical_drawing(ctx, submit_world_shape(*frame, request, nullptr, nullptr, nullptr, false, false));
    } catch (...) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}

void build_cell_shadow_cast(PresentationContext& ctx, CellClass& self, const Point2D& point, const RectangleStruct& clip) {
    auto* type = IsometricTileTypeClass::Array.GetItemOrDefault(
        self.IsoTileTypeIndex == 0xFFFF ? IsometricTileTypeClass::ClearTile : self.IsoTileTypeIndex);
    if (!type) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::unavailable);
        return;
    }
    const int sub = self.IsoTileTypeIndex == 0xFFFF ? 0 : static_cast<unsigned char>(self.Height);
    if (type->ShadowCaster) {
        const int level = static_cast<signed char>(self.Level);
        build_tile_shadow_caster(ctx, *type, sub, {point.X, point.Y - 15 * level}, clip, 58 - 15 * level);
    }
}

void build_tiles(PresentationContext& ctx, TacticalClass& tactical, const RectangleStruct& area, const RectangleStruct& clip) {
    auto* frame = ctx.frame;
    if (!frame || !frame->drawing) return;
    try {
        tactical_walk_cells(ctx, tactical, area, 17, 4, 0, false, [&](CellClass& cell, const Point2D& at) {
            // Cell/TMP perform artwork and multi-cell-smudge clipping; do not discard a
            // smudge merely because the owning TMP's diamond is outside the rectangle.
            const auto status = game::draw_cell_terrain(cell, *frame->drawing, at, clip);
            game::record_tactical_drawing(ctx, status);
            if (frame->statistics) {
                ++frame->statistics->visited;
                if (status == game::DrawingStatus::drawn)
                    ++frame->statistics->drawn;
                else if (status == game::DrawingStatus::skipped)
                    ++frame->statistics->skipped;
            }
        });
    } catch (...) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}

void build_tile_shadows(PresentationContext& ctx, TacticalClass& tactical, const RectangleStruct& area, const RectangleStruct& clip) {
    try {
        tactical_walk_cells(ctx, tactical, area, 20, 7, 3, false,
                            [&](CellClass& cell, const Point2D& at) { build_cell_shadow_cast(ctx, cell, at, clip); });
    } catch (...) {
        game::record_tactical_drawing(ctx, game::DrawingStatus::backend_failure);
    }
}
}
