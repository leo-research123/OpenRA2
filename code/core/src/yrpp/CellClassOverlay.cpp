// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 additional terms: code/third_party/opents/LICENSE.md.
// Adapted from OpenTS cell.cpp at 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR calibration: 0x0047F6A0, 0x0047F510, 0x00480110. In particular YR
// uses the theater converter for ore/veins, adds crates/rubble, and does not
// apply OpenTS's later tiberium frame checks to the shadow path.
#include "yrpp/CellClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/Surface.h"
#include "yrpp/TacticalClass.h"
#include "map_runtime.hpp"
#include "overlay_drawing.hpp"
#include <bit>

namespace {
int add(int a, int b) noexcept { return std::bit_cast<int>(static_cast<unsigned>(a) + static_cast<unsigned>(b)); }
int subtract(int a, int b) noexcept { return std::bit_cast<int>(static_cast<unsigned>(a) - static_cast<unsigned>(b)); }
}

Point2D game::cell_overlay_offset(const CellClass& cell, int top) {
    Point2D result;
    OverlayTypeClass::GetDrawOffset(&result, cell.OverlayTypeIndex);
    const bool bridge = (static_cast<unsigned>(cell.Flags) & 0x80u) != 0;
    if (bridge) {
        result.Y -= 16;
        if (cell.OverlayData >= 9 && cell.OverlayData <= 17) result.Y -= 15;
    } else if (cell.OverlayTypeIndex == 239) result.Y -= 15;
    result.X = add(result.X, 30);
    result.Y = add(add(result.Y, top), 15 - 15 * static_cast<signed char>(cell.Level));
    return result;
}
Point2D* CellClass::GetOverlayDrawOffset(Point2D* output) const {
    const auto* bounds = game::map_runtime().drawing_bounds;
    *output = game::cell_overlay_offset(*this, bounds ? bounds->Y : DSurface::ViewBounds.Y);
    return output;
}

// OpenTS Overlay_Render_Rect / Overlay_Shadow_Render_Rect, with YR crate and
// rubble resolution. YR 0x0047FB90 / 0x0047FDE0 use distinct body/shadow images.
namespace {
SHPStruct* shape_data(SHPStruct* image) {
    if (image)
        if (auto* ref = image->AsReference()) {
            ref->Load();
            return ref->Data;
        }
    return image;
}
RectangleStruct overlay_rect(const CellClass& cell, SHPStruct* image, int frame, bool shadow, int top) {
    image = shape_data(image);
    if (!image || !TacticalClass::Instance || frame < 0 || frame >= image->Frames) return {};
    const auto offset = game::cell_overlay_offset(cell, top);
    const auto point = TacticalClass::CoordsToScreen({cell.MapCoords.X * 256, cell.MapCoords.Y * 256, 0});
    auto rect = image->GetFrameBounds(frame);
    rect.X += point.X - TacticalClass::Instance->TacticalPos.X + offset.X - 30 - image->Width / 2;
    rect.Y += point.Y - TacticalClass::Instance->TacticalPos.Y + offset.Y - image->Height / 2;
    if (shadow && (unsigned(cell.Flags) & 0x80u) && cell.OverlayData >= 9 && cell.OverlayData <= 17) {
        rect.X -= 15;
        rect.Y += 7;
    }
    return rect;
}
}
RectangleStruct game::cell_overlay_bounds(const CellClass& cell, int top, bool shadow) {
    if (shadow) {
        auto* type = OverlayTypeClass::Array.GetItemOrDefault(cell.OverlayTypeIndex);
        auto* image = type ? shape_data(type->GetImage()) : nullptr;
        if (!image) return {};
        return overlay_rect(cell, image, (type->Crate ? 0 : cell.OverlayData) + image->Frames / 2, true, top);
    }
    RectangleStruct result;
    auto* out = &result;
    *out = {};
    auto* type = OverlayTypeClass::Array.GetItemOrDefault(cell.OverlayTypeIndex);
    if (!type) return *out;
    SHPStruct* image = nullptr;
    int frame = cell.OverlayData;
    if (type->Tiberium) {
        auto* resource = TiberiumClass::Array.GetItemOrDefault(TiberiumClass::FindIndex(cell.OverlayTypeIndex));
        if (!resource || !resource->Image) return *out;
        const int variants = cell.SlopeIndex ? resource->NumSlopes / 4 : resource->NumImages;
        if (variants <= 0) return *out;
        int index = resource->Image->ArrayIndex + (cell.MapCoords.X * cell.MapCoords.Y) % variants;
        if (cell.SlopeIndex) index += resource->NumImages + variants * (cell.SlopeIndex - 1);
        auto* variant = OverlayTypeClass::Array.GetItemOrDefault(index);
        if (variant) image = variant->GetImage();
    } else if (type->IsRubble) {
        if (cell.Rubble) cell.Rubble->GetRubbleShape(&image, &frame);
    } else
        image = type->GetImage();
    if (type->Crate) frame = 0;
    *out = overlay_rect(cell, image, frame, false, top);
    return *out;
}
RectangleStruct* CellClass::GetContainingRect(RectangleStruct* out) const {
    const auto* bounds = game::map_runtime().drawing_bounds;
    *out = game::cell_overlay_bounds(*this, bounds ? bounds->Y : DSurface::ViewBounds.Y, false);
    return out;
}
RectangleStruct* CellClass::ShapeRect(RectangleStruct* out) {
    const auto* bounds = game::map_runtime().drawing_bounds;
    *out = game::cell_overlay_bounds(*this, bounds ? bounds->Y : DSurface::ViewBounds.Y, true);
    return out;
}
