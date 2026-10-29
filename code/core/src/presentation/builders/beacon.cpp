// YR tactical beacon drawing (0x00430250) and radar drawing (0x00430650).
// Whole-radar composition replaces the original dirty rectangle/pixel queue tail.
#include "yrpp/BeaconClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/Unsorted.h"
#include "game_ui_runtime.hpp"
#include "type_drawing.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/BitText.h"
#include "yrpp/Drawing.h"
#include "yrpp/Timer.h"
#include <cwchar>
#include "presentation/builders/beacon.hpp"
#include "presentation/context.hpp"
#include "presentation/ui/bit_text.hpp"
namespace {
game::DrawingStatus beacon_box(const game::TypeDrawingContext& drawing, game::DrawingTargetHandle* target,
                               const RectangleStruct& box, const RectangleStruct& clip, WORD color) noexcept {
    if (box.Width <= 0 || box.Height <= 0) return game::DrawingStatus::skipped;
    game::RasterDrawingRequest r;
    r.target = target;
    r.clip = clip;
    r.position = {box.X, box.Y};
    r.width = box.Width;
    r.height = box.Height;
    auto status = game::submit_type_raster(drawing, r);
    if (!game::drawing_completed(status)) return status;
    // Fill black, then outline the clipped rectangle, as in 0x4304D6/0x4304E4.
    r.color = color;
    for (const auto edge :
         {RectangleStruct{box.X, box.Y, box.Width, 1}, RectangleStruct{box.X, box.Y + box.Height - 1, box.Width, 1},
          RectangleStruct{box.X, box.Y, 1, box.Height}, RectangleStruct{box.X + box.Width - 1, box.Y, 1, box.Height}}) {
        r.position = {edge.X, edge.Y};
        r.width = edge.Width;
        r.height = edge.Height;
        status = game::submit_type_raster(drawing, r);
        if (!game::drawing_completed(status)) return status;
    }
    return status;
}
}
namespace game {
void build_beacon(PresentationContext& ctx, const BeaconClass& self, Surface* surface, RectangleStruct bounds) noexcept {
    using namespace game;
    const auto record = [&ctx](DrawingStatus status) { record_type_drawing_result(ctx, status); };
    try {
        auto* house = HouseClass::Array.GetItemOrDefault(self.HouseID);
        auto* scheme = house ? ColorScheme::Array.GetItemOrDefault(house->ColorSchemeIndex) : nullptr;
        if (!scheme || !TacticalClass::Instance) {
            record(DrawingStatus::unavailable);
            return;
        }
        Point2D point;
        if (!TacticalClass::Instance->CoordsToClient(&self.Coord, &point)) return;
        const auto* drawing = ctx.type;
        const int period = BeaconManagerClass::Instance.BeaconFrameCount / 2;
        if (!drawing || !BeaconManagerClass::BeaconArt || period <= 0) {
            record(DrawingStatus::unavailable);
            return;
        }
        ShapeDrawingRequest shape;
        shape.target = drawing->target;
        if (surface) {
            if (!drawing->legacy_target || !drawing->legacy_palette) {
                record(DrawingStatus::unavailable);
                return;
            }
            auto status = drawing->legacy_target(drawing->backend_context, surface, shape.target);
            if (status == DrawingStatus::drawn)
                status = drawing->legacy_palette(drawing->backend_context, scheme->LightConvert, shape.palette);
            if (status != DrawingStatus::drawn) {
                record(status);
                return;
            }
        } else if (!resolve_drawing_palette(ctx, DrawingPaletteKind::color_scheme, house->ColorSchemeIndex, shape.palette))
            return;
        shape.image = BeaconManagerClass::BeaconArt;
        shape.frame = Unsorted::CurrentFrame % period + ((self.Bitfield & 2) ? period : 0);
        shape.position = point;
        shape.clip = bounds;
        shape.flags = 0x600;
        shape.intensity = 1000;
        auto status = submit_type_shape(*drawing, shape);
        record(status);
        if (!drawing_completed(status) || !self.Text[0]) return;
        wchar_t text[128];
        std::wmemcpy(text, self.Text, 128);
        if ((self.Bitfield & 2) && MessageListClass::Instance.IsEdit && SystemTimer::GetMilliseconds() % 500u < 250u) {
            if (auto* cursor = std::wcsrchr(text, L'_'); cursor && cursor[1] == L'\0') *cursor = L' ';
        }
        if (!(self.Bitfield & 2) || !MessageListClass::Instance.IsEdit) {
            auto* tail = std::wcsrchr(text, L'_');
            if (!tail || tail[1]) tail = std::wcsrchr(text, L' ');
            if (tail && !tail[1]) *tail = L'\0';
        }
        auto* font = BitFont::Instance;
        if (!font || !BitText::Instance) {
            record(DrawingStatus::unavailable);
            return;
        }
        int width = 0, height = 0;
        font->GetTextDimension(text, &width, &height, 200);
        const RectangleStruct box{point.X - width / 2 - 4, point.Y + 32, width + 8, height + 8};
        const auto color = ColorScheme::HSVToRGB(scheme->BaseColor);
        const WORD pixel = WORD(((color.R >> Drawing::RedShiftRight) << Drawing::RedShiftLeft) |
                                ((color.G >> Drawing::GreenShiftRight) << Drawing::GreenShiftLeft) |
                                ((color.B >> Drawing::BlueShiftRight) << Drawing::BlueShiftLeft));
        status = beacon_box(*drawing, shape.target, Drawing::Intersect(box, bounds), bounds, pixel);
        record(status);
        if (!drawing_completed(status)) return;
        font->SetClipMode(true);
        LTRBStruct clip{bounds.X, bounds.Y, bounds.X + bounds.Width - 1, bounds.Y + bounds.Height - 1};
        font->SetRectangle(&clip);
        font->SetColor(pixel);
        // The original BitText algorithm uses the same BitFont glyphs on both
        // legacy surfaces and the currently borrowed tactical drawing target.
        build_bit_text(ctx, font, surface, text, box.X + 4, box.Y + 4, 200, box.Height, 0, 0, 0);
    } catch (...) {
        record(DrawingStatus::backend_failure);
    }
}
void build_radar_beacon(GameUiFrame& context, const BeaconClass& self, Surface* surface, RectangleStruct bounds, bool clear) noexcept {
    using namespace game;
    if (clear) return;  // The next complete composition supplies these background pixels.
    const auto& manager = BeaconManagerClass::Instance;
    const auto record = [](DrawingStatus result) { record_ui_drawing(result); };
    if (manager.RadarBeaconAnimPeriod <= 0) {
        record(DrawingStatus::unavailable);
        return;
    }
    const int frame = Unsorted::CurrentFrame % manager.RadarBeaconAnimPeriod;
    if (frame >= manager.RadarBeaconFrameCount) return;  // Includes the original erase frame.
    auto* image = BeaconManagerClass::RadarBeaconArt;
    if (!image) {
        record(DrawingStatus::unavailable);
        return;
    }
    Point2D point;
    auto coords = self.Coord;
    if (!RadarClass::Instance.GetCrdOnRadar(&point, &coords, true)) {
        record(DrawingStatus::unavailable);
        return;
    }
    auto* house = HouseClass::Array[self.HouseID];
    auto* scheme = ColorScheme::Array.GetItemOrDefault(house->ColorSchemeIndex);
    if (!scheme) {
        record(DrawingStatus::unavailable);
        return;
    }
    ShapeDrawingRequest request;
    request.image = image;
    request.frame = frame;
    request.position = point;
    request.clip = bounds;
    request.flags = 0x600;
    request.intensity = 1000;
    if (surface) {
        const auto* drawing = &context.drawing.types;
        if (!drawing->legacy_target || !drawing->legacy_palette) {
            record(DrawingStatus::unavailable);
            return;
        }
        try {
            auto result = drawing->legacy_target(drawing->backend_context, surface, request.target);
            if (result == DrawingStatus::drawn)
                result = drawing->legacy_palette(drawing->backend_context, scheme->LightConvert, request.palette);
            if (result == DrawingStatus::drawn) result = submit_type_shape(*drawing, request);
            record(result);
        } catch (...) {
            record(DrawingStatus::backend_failure);
        }
    } else {
        auto* ui = &context;
        if (!ui || !ui->drawing.color_scheme_palette) {
            record(DrawingStatus::unavailable);
            return;
        }
        // Shape positions are relative to the supplied clipping rectangle in
        // both original software blits and the GPU packet contract.
        request.target = ui->drawing.types.target;
        try {
            auto result = ui->drawing.color_scheme_palette(ui->drawing.types.backend_context, scheme->Colors,
                                                           scheme->ShadeCount, request.palette);
            if (result == DrawingStatus::drawn) result = submit_type_shape(ui->drawing.types, request);
            record(result);
        } catch (...) {
            record(DrawingStatus::backend_failure);
        }
    }
}
}
namespace game {
void build_beacons(PresentationContext& ctx, const BeaconManagerClass& self, Surface* surface, RectangleStruct bounds) noexcept {
    if (!self.AllocatedCount) return;
    for (const auto& house : self.Beacons)
        for (auto* beacon : house)
            if (beacon && beacon->VisibleToPlayer()) {
                build_beacon(ctx, *beacon, surface, bounds);
                if (!drawing_completed(type_drawing_status(ctx))) return;
            }
}
void build_radar_beacons(GameUiFrame& context, const BeaconManagerClass& self, Surface* surface, RectangleStruct bounds) noexcept {
    if (!self.AllocatedCount) return;
    if (self.RadarBeaconAnimPeriod <= 0) {
        record_ui_drawing(DrawingStatus::unavailable);
        return;
    }
    if (Unsorted::CurrentFrame % self.RadarBeaconAnimPeriod >= BeaconManagerClass::Instance.RadarBeaconFrameCount + 1) return;
    for (const auto& house : self.Beacons)
        for (auto* beacon : house)
            if (beacon && beacon->VisibleToPlayer()) build_radar_beacon(context, *beacon, surface, bounds, false);
}
}
