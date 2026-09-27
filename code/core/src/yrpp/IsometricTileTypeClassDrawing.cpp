// Resource/variant selection from the existing 547CF0 port. No pixel-format,
// Surface locking, ABuffer/ZBuffer, or Convert implementation dependency here.
#include "yrpp/IsometricTileTypeClass.h"
#include "type_drawing.hpp"

game::DrawingStatus IsometricTileTypeClass::DrawTMP(const game::TypeDrawingContext& context,
        int subTile, int x, int y, RectangleStruct clip, int level, int intensity,
        bool useZ, int alternate, bool flat, bool flag16, bool flag17, int color) noexcept {
    if (subTile < 0 || alternate < 0) return game::DrawingStatus::invalid_argument;
    if (clip.Width <= 0 || clip.Height <= 0) return game::DrawingStatus::skipped;
    try {
        auto* owner = this;
        if (alternate) {
            if (unk_2F0 <= 0) return game::DrawingStatus::invalid_argument;
            alternate %= unk_2F0;
            while (alternate-- && owner) owner = owner->NextVariant;
            if (!owner) return game::DrawingStatus::invalid_argument;
        }
        const auto* tmp = reinterpret_cast<const TMPStruct*>(owner->IsometricTileTypeClass::GetImage());
        if (!tmp) return game::DrawingStatus::skipped;
        if (tmp->Width != 60 || tmp->Height != 30 || tmp->Columns <= 0 || tmp->Rows <= 0 ||
            tmp->Columns > 255 || tmp->Rows > 255) return game::DrawingStatus::invalid_argument;
        game::TileDrawingRequest request;
        request.sub_tile = subTile % (tmp->Columns * tmp->Rows);
        if (!tmp->GetSubTile(request.sub_tile, request.image)) return game::DrawingStatus::skipped;
        request.target = context.target;
        request.palette = context.palette;
        request.resource = tmp;
        request.position = {x, y}; request.clip = clip;
        request.level = level; request.intensity = intensity;
        request.use_depth = useZ;
        request.flat = flat; request.flag16 = flag16; request.flag17 = flag17;
        request.color = color; request.buffer_offset_y = context.buffer_offset_y;
        request.translucent = context.translucent;
        return game::submit_type_tile(context, request);
    } catch (...) { return game::DrawingStatus::backend_failure; }
}
void IsometricTileTypeClass::DrawTMP(ConvertClass* convert, int subTile, Surface* surface,
        int x, int y, RectangleStruct clip, int level, int intensity, bool useZ,
        int alternate, bool flat, bool flag16, bool flag17, int color) {
    const auto* context = game::active_type_drawing();
    if (!context) {
        DrawTMPOriginal(convert, subTile, surface, x, y, clip, level, intensity,
            useZ, alternate, flat, flag16, flag17, color);
        return;
    }
    if (!context->legacy_target || !context->legacy_palette) {
        game::record_type_drawing_result(game::DrawingStatus::unavailable);
        return;
    }
    try {
        auto bound = *context;
        auto result = context->legacy_target(context->backend_context, surface, bound.target);
        if (result == game::DrawingStatus::drawn)
            result = context->legacy_palette(context->backend_context, convert, bound.palette);
        if (result == game::DrawingStatus::drawn)
            result = DrawTMP(bound, subTile, x, y, clip, level, intensity, useZ,
                alternate, flat, flag16, flag17, color);
        game::record_type_drawing_result(result);
    } catch (...) { game::record_type_drawing_result(game::DrawingStatus::backend_failure); }
}
