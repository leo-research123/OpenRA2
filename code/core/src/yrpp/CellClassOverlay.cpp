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
int add(int a, int b) noexcept {
    return std::bit_cast<int>(static_cast<unsigned>(a) + static_cast<unsigned>(b));
}
int subtract(int a, int b) noexcept {
    return std::bit_cast<int>(static_cast<unsigned>(a) - static_cast<unsigned>(b));
}
bool initialize_light(CellClass& cell, game::OverlayDrawing& drawing) {
    if (cell.LightConvert) return true;
    if (!drawing.initialize_light) {
        drawing.status = game::DrawingStatus::unavailable;
        return false;
    }
    drawing.status = drawing.initialize_light(drawing.context, cell);
    return drawing.status == game::DrawingStatus::drawn;
}
void submit(CellClass& cell, game::OverlayDrawing& drawing,
        game::OverlayPalette palette, SHPStruct* image, int frame,
        const Point2D& point, const RectangleStruct& clip, unsigned flags,
        int depth, int gradient, int intensity, SHPStruct* depth_image = nullptr,
        Point2D depth_offset = {}) {
    game::ShapeDrawingRequest request;
    request.target = drawing.types.target;
    if (!drawing.palette) { drawing.status = game::DrawingStatus::unavailable; return; }
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
    if (slope > 4) { drawing.status = game::DrawingStatus::invalid_argument; return false; }
    image = drawing.slope_depth[slope];
    if (slope && !image) { drawing.status = game::DrawingStatus::unavailable; return false; }
    return true;
}
}

Point2D* CellClass::GetOverlayDrawOffset(Point2D* output) const {
    OverlayTypeClass::GetDrawOffset(output, OverlayTypeIndex);
    const bool bridge = (static_cast<unsigned>(Flags) & 0x80u) != 0;
    if (bridge) {
        output->Y -= 16;
        if (OverlayData >= 9 && OverlayData <= 17) output->Y -= 15;
    } else if (OverlayTypeIndex == 239) output->Y -= 15;
    const auto* drawing = game::overlay_drawing();
    const auto* bounds=game::map_runtime().drawing_bounds;
    const int top = drawing ? drawing->tactical_rect.Y : bounds ? bounds->Y : DSurface::ViewBounds.Y;
    output->X = add(output->X, 30);
    output->Y = add(add(output->Y, top), 15 - 15 * static_cast<signed char>(Level));
    return output;
}

void CellClass::DrawOverlay(const Point2D& location, const RectangleStruct& clip) {
    auto* drawing = game::overlay_drawing();
    if (!drawing) return; // Hosts bind the internal synchronous drawing contract.
    if (OverlayTypeIndex == 167 || OverlayTypeIndex == 178) return;
    auto* type = OverlayTypeClass::Array.GetItemOrDefault(OverlayTypeIndex);
    if (!type) { drawing->status = game::DrawingStatus::invalid_argument; return; }
    Point2D point;
    GetOverlayDrawOffset(&point);
    point.X = subtract(add(point.X, location.X), clip.X);
    point.Y = subtract(add(point.Y, location.Y), clip.Y);
    const bool bridge = (static_cast<unsigned>(Flags) & 0x80u) != 0;
    const int height = 15 * (static_cast<signed char>(Level) + (bridge ? 4 : 0));
    if (!initialize_light(*this, *drawing)) return;
    auto* image = type->GetImage();
    if (bridge) {
        const auto& rect = InViewportRect;
        if (RedrawFrame == static_cast<unsigned>(drawing->frame) &&
            static_cast<unsigned char>(unknown_118) == drawing->redraws &&
            rect.X == clip.X && rect.Y == clip.Y &&
            rect.Width == clip.Width && rect.Height == clip.Height) {
            drawing->status = game::DrawingStatus::skipped;
            return;
        }
        constexpr int variations[16] = {0,1,2,3,3,2,1,0,2,3,0,1,1,0,3,2};
        int frame = OverlayData;
        if (frame == 0 || frame == 9)
            frame += variations[(MapCoords.X & 3) | ((MapCoords.Y & 3) << 2)];
        submit(*this, *drawing, game::OverlayPalette::cell, image, frame, point, clip,
            0x4E00, -2-height, 0, std::bit_cast<short>(Color1_Blue));
        // Original writes TacticalRect, not the potentially smaller clip.
        // A host failure is retryable and must not mark a lost request as drawn.
        if (drawing->status == game::DrawingStatus::drawn || drawing->status == game::DrawingStatus::skipped) {
            RedrawFrame = static_cast<unsigned>(drawing->frame);
            InViewportRect = drawing->tactical_rect;
            unknown_118 = static_cast<char>(drawing->redraws);
        }
        return;
    }
    if (type->Tiberium) {
        const int resource_index = TiberiumClass::FindIndex(OverlayTypeIndex);
        if (resource_index == -1) { drawing->status = game::DrawingStatus::skipped; return; }
        auto* resource = TiberiumClass::Array.GetItemOrDefault(resource_index);
        if (!resource) { drawing->status = game::DrawingStatus::invalid_argument; return; }
        const int variants = SlopeIndex ? resource->NumSlopes / 4 : resource->NumImages;
        if (!resource->Image || variants <= 0) { drawing->status = game::DrawingStatus::invalid_argument; return; }
        int index = resource->Image->ArrayIndex + (MapCoords.X * MapCoords.Y) % variants;
        if (SlopeIndex) index += resource->NumImages + variants * (SlopeIndex-1);
        auto* variant = OverlayTypeClass::Array.GetItemOrDefault(index);
        if (!variant) { drawing->status = game::DrawingStatus::invalid_argument; return; }
        image = variant->GetImage();
        SHPStruct* depth = nullptr;
        if (!slope_image(*drawing, SlopeIndex, depth)) return;
        submit(*this, *drawing, game::OverlayPalette::theater, image, OverlayData,
            point, clip, 0x4E00, -2-height, 0, 1000, depth);
    } else if (type->Wall) {
        submit(*this, *drawing, game::OverlayPalette::wall, image, OverlayData,
            point, clip, 0x4E00, -2-height, 2, std::bit_cast<short>(Intensity_Normal));
    } else if (type->ArrayIndex == 126) {
        SHPStruct* depth = nullptr;
        if (!slope_image(*drawing, SlopeIndex, depth)) return;
        submit(*this, *drawing, game::OverlayPalette::theater, image, OverlayData,
            point, clip, 0x4E00, -2-height, 0, std::bit_cast<short>(Intensity_Terrain),
            depth, SlopeIndex ? Point2D{30,-2} : Point2D{});
    } else {
        const int bias = type->DrawFlat || type->IsARock ? 0 : -15;
        int frame = OverlayData;
        if (type->Crate) frame = 0;
        else if (type->IsRubble && (!Rubble || !Rubble->GetRubbleShape(&image, &frame))) {
            drawing->status = game::DrawingStatus::skipped;
            return;
        }
        submit(*this, *drawing, game::OverlayPalette::cell, image, frame, point, clip,
            0x4E00, bias-height-2, type->DrawFlat ? 0 : 2,
            std::bit_cast<short>(Intensity_Terrain));
    }
}

void CellClass::DrawOverlayShadow(const Point2D& location, const RectangleStruct& clip) {
    auto* drawing = game::overlay_drawing();
    if (!drawing) return;
    auto* type = OverlayTypeClass::Array.GetItemOrDefault(OverlayTypeIndex);
    if (!type) { drawing->status = game::DrawingStatus::invalid_argument; return; }
    const int height = 15 * static_cast<signed char>(Level);
    auto* image = type->GetImage();
    Point2D point;
    GetOverlayDrawOffset(&point);
    point.X = subtract(add(point.X, location.X), clip.X);
    point.Y = subtract(add(point.Y, location.Y), clip.Y);
    if ((static_cast<unsigned>(Flags) & 0x80u) && OverlayData >= 9 && OverlayData <= 17) {
        point.X = add(point.X, -15);
        point.Y = add(point.Y, 7);
    }
    if (!initialize_light(*this, *drawing)) return;
    int frame = type->Crate ? 0 : OverlayData;
    if (!type->Crate && type->IsRubble) {
        if (!Rubble || !Rubble->GetRubbleShadowShape(&image, &frame)) {
            drawing->status = game::DrawingStatus::skipped;
            return;
        }
    } else {
        if (!image) { drawing->status = game::DrawingStatus::unavailable; return; }
        // GetImage resolves OverlayType's image. The YR entry reads the SHP
        // header directly; it does not pick the ore/body coordinate variant.
        frame += image->Frames / 2;
    }
    submit(*this, *drawing, game::OverlayPalette::cell, image, frame, point, clip,
        0x4601, -2-height, 0, 1000);
}

// OpenTS Overlay_Render_Rect / Overlay_Shadow_Render_Rect, with YR crate and
// rubble resolution. YR 0x0047FB90 / 0x0047FDE0 use distinct body/shadow images.
namespace {
SHPStruct* shape_data(SHPStruct* image){
    if(image)if(auto* ref=image->AsReference()){ref->Load();return ref->Data;}
    return image;
}
RectangleStruct overlay_rect(const CellClass& cell,SHPStruct* image,int frame,bool shadow){
    image=shape_data(image);
    if(!image||!TacticalClass::Instance||frame<0||frame>=image->Frames)return {};
    Point2D offset;cell.GetOverlayDrawOffset(&offset);
    const auto point=TacticalClass::CoordsToScreen({cell.MapCoords.X*256,cell.MapCoords.Y*256,0});
    auto rect=image->GetFrameBounds(frame);
    rect.X+=point.X-TacticalClass::Instance->TacticalPos.X+offset.X-30-image->Width/2;
    rect.Y+=point.Y-TacticalClass::Instance->TacticalPos.Y+offset.Y-image->Height/2;
    if(shadow&&(unsigned(cell.Flags)&0x80u)&&cell.OverlayData>=9&&cell.OverlayData<=17){rect.X-=15;rect.Y+=7;}
    return rect;
}
}
RectangleStruct* CellClass::GetContainingRect(RectangleStruct* out) const {
    *out={};auto* type=OverlayTypeClass::Array.GetItemOrDefault(OverlayTypeIndex);if(!type)return out;
    SHPStruct* image=nullptr;int frame=OverlayData;
    if(type->Tiberium){
        auto* resource=TiberiumClass::Array.GetItemOrDefault(TiberiumClass::FindIndex(OverlayTypeIndex));
        if(!resource||!resource->Image)return out;
        const int variants=SlopeIndex?resource->NumSlopes/4:resource->NumImages;if(variants<=0)return out;
        int index=resource->Image->ArrayIndex+(MapCoords.X*MapCoords.Y)%variants;
        if(SlopeIndex)index+=resource->NumImages+variants*(SlopeIndex-1);
        auto* variant=OverlayTypeClass::Array.GetItemOrDefault(index);if(variant)image=variant->GetImage();
    }else if(type->IsRubble){if(Rubble)Rubble->GetRubbleShape(&image,&frame);}
    else image=type->GetImage();
    if(type->Crate)frame=0;
    *out=overlay_rect(*this,image,frame,false);return out;
}
RectangleStruct* CellClass::ShapeRect(RectangleStruct* out){
    *out={};auto* type=OverlayTypeClass::Array.GetItemOrDefault(OverlayTypeIndex);if(!type)return out;
    auto* image=shape_data(type->GetImage());if(!image)return out;
    *out=overlay_rect(*this,image,(type->Crate?0:OverlayData)+image->Frames/2,true);return out;
}
