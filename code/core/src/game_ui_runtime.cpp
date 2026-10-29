#include "game_ui_runtime.hpp"
#include "bitfont_drawing.hpp"
#include "type_drawing.hpp"
#include "api/drawing_diagnostics.hpp"
#include <cstdio>
#include "yrpp/Surface.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Drawing.h"

namespace game {
namespace {
thread_local GameUiFrame* active = nullptr;
}
GameUiFrame* game_ui_frame() noexcept { return active; }
void record_ui_drawing(DrawingStatus result) noexcept {
    if (!active || (active->status != DrawingStatus::drawn && active->status != DrawingStatus::skipped)) return;
    if (result != DrawingStatus::skipped) active->status = result;
    if (!drawing_completed(result)) record_drawing_failure(result, "ui_draw");
}
void with_game_ui_state(GameUiFrame& frame, void (*operation)(void*), void* argument) noexcept {
    const auto previous = active;
    auto* previous_font = BitFont::Instance;
    if (frame.resources.font()) BitFont::Instance = frame.resources.font();
    active = &frame;
    try { if (operation) operation(argument); } catch (...) { frame.status = DrawingStatus::backend_failure; }
    active = previous;
    BitFont::Instance = previous_font;
}
DrawingStatus with_game_ui_frame(GameUiFrame& frame, void (*operation)()) noexcept {
    if (!operation || !frame.drawing.plain_palette) {
        char detail[128];
        std::snprintf(detail, sizeof(detail), "operation_present=%d plain_palette_present=%d", operation != nullptr,
                      frame.drawing.plain_palette != nullptr);
        return record_drawing_diagnostic(DrawingStatus::invalid_argument, "ui_frame", "missing_required_callback",
                                         detail, __FILE__, __LINE__);
    }
    const auto prepared =
        frame.drawing.plain_palette(frame.drawing.types.backend_context, frame.resources.palette(), frame.palette);
    if (prepared != DrawingStatus::drawn) return prepared;
    const auto previous = active;
    auto* previous_font = BitFont::Instance;
    if (frame.resources.font()) BitFont::Instance = frame.resources.font();
    active = &frame;
    try {
        operation();
    } catch (...) {
        frame.status = DrawingStatus::backend_failure;
    }
    active = previous;
    BitFont::Instance = previous_font;
    return frame.status;
}
void draw_ui_shape(UiImage id, const Point2D& position, int index) noexcept {
    if (!active) return;
    auto* image = active->resources.image(id);
    if (!image) return;
    draw_ui_shape(image, position, index);
}
void draw_ui_shape(SHPStruct* image, const Point2D& position, int index) noexcept {
    if (!active || !image) return;
    ShapeDrawingRequest request;
    request.target = active->drawing.types.target;
    request.palette = active->palette;
    request.image = image;
    request.frame = index;
    request.position = position;
    request.clip = DSurface::WindowBounds;
    request.flags = 0;
    const auto result = submit_type_shape(active->drawing.types, request);
    if (!drawing_completed(result)) record_drawing_failure(result, "ui_shape", nullptr, &request);
    record_ui_drawing(result);
}
void draw_ui_shape_strip(UiImage id, const Point2D& position, Point2D step, int count, int index) noexcept {
    if (!active || count <= 0) return;
    auto* image = active->resources.image(id);
    if (!image) return;
    ShapeStripDrawingRequest request;
    request.resource_revision = active->resources.revision();
    request.shape.target = active->drawing.types.target;
    request.shape.palette = active->palette;
    request.shape.image = image;
    request.shape.frame = index;
    request.shape.position = position;
    request.shape.clip = DSurface::WindowBounds;
    request.step = step;
    request.count = count;
    record_ui_drawing(submit_type_shape_strip(active->drawing.types, request));
}
void draw_ui_fill(const RectangleStruct& rect, WORD color) noexcept {
    if (!active) return;
    RasterDrawingRequest request;
    request.target = active->drawing.types.target;
    request.position = {rect.X, rect.Y};
    request.clip = DSurface::WindowBounds;
    request.width = rect.Width;
    request.height = rect.Height;
    request.color = color;
    record_ui_drawing(submit_type_raster(active->drawing.types, request));
}
void draw_ui_bitmap(const Point2D& position, int width, int height, const WORD* pixels, unsigned count) noexcept {
    if (!active) return;
    RasterDrawingRequest request;
    request.target = active->drawing.types.target;
    request.position = position;
    request.clip = DSurface::WindowBounds;
    request.width = width;
    request.height = height;
    request.pitch = width;
    request.pixels = pixels;
    request.pixel_count = count;
    record_ui_drawing(submit_type_raster(active->drawing.types, request));
}
namespace {
WORD pack_color(const ColorStruct& color) noexcept {
    return WORD((unsigned(color.R) >> Drawing::RedShiftRight) << Drawing::RedShiftLeft |
                (unsigned(color.G) >> Drawing::GreenShiftRight) << Drawing::GreenShiftLeft |
                (unsigned(color.B) >> Drawing::BlueShiftRight) << Drawing::BlueShiftLeft);
}
}
bool ui_palette_color(unsigned index, WORD& color) noexcept {
    if (!active || index >= 256) return false;
    color = pack_color(active->resources.palette().Entries[index]);
    return true;
}
DrawingStatus draw_ui_text(const wchar_t* text, const RectangleStruct& clip, const Point2D& point,
                           const ColorScheme* scheme, TextPrintType flags) noexcept {
    if (!active) return DrawingStatus::unavailable;
    auto* font = active->resources.font();
    if (!font || !scheme) {
        record_ui_drawing(DrawingStatus::unavailable);
        return DrawingStatus::unavailable;
    }
    if (!text) return DrawingStatus::skipped;
    // 0x004A61C0 -> 0x004A5EB0 -> 0x00434500. YR's BitFont path
    // uses HSV BaseColor even with UseGradPal, and skips CR/LF while blitting.
    const WORD color = pack_color(ColorScheme::HSVToRGB(scheme->BaseColor));
    int width = 0, height = 0;
    if (!font->GetTextDimension(text, &width, &height, clip.Width)) {
        record_ui_drawing(DrawingStatus::unavailable);
        return DrawingStatus::unavailable;
    }
    int x = clip.X + point.X;
    const unsigned bits = static_cast<unsigned>(flags);
    if (bits & 0x100)
        x -= width / 2;
    else if (bits & 0x200)
        x -= width;
    // 0x004A5F61..0x004A5F93: Background fills the measured text box
    // (including a one-pixel border) before glyphs. FillRect clips to the
    // destination surface, independently of the text's clipping rectangle.
    if ((bits & 0x8400u) == 0x400u)
        draw_ui_fill({x - 1, clip.Y + point.Y - 1, width + 2, height + 2}, 0);
    const int old_tab_origin = font->field_20;
    font->field_20 = x;
    for (const auto* c = text; *c; ++c) {
        if (*c == L'\r' || *c == L'\n') continue;
        int next = x;
        record_ui_drawing(
            game::submit_bitfont_glyph(*font, active->drawing.types, *c, x, clip.Y + point.Y, clip, color, next));
        x = next;
    }
    font->field_20 = old_tab_origin;
    return active->status;
}
DrawingStatus draw_tactical_text(const wchar_t* text, const Point2D& point, const ColorScheme* scheme,
                                 TextPrintType flags) noexcept {
    const Point2D canvas{DSurface::ViewBounds.X + point.X, DSurface::ViewBounds.Y + point.Y};
    return draw_ui_text(text, DSurface::WindowBounds, canvas, scheme, flags);
}
}

namespace game {
namespace {
thread_local const GameUiInput* input_frame = nullptr;
}
const GameUiInput* game_ui_input() noexcept { return input_frame; }
bool with_game_ui_input(const GameUiInput& input, void (*operation)()) noexcept {
    if (!operation) return false;
    struct Restore {
        const GameUiInput* old;
        ~Restore() { input_frame = old; }
    } restore{input_frame};
    input_frame = &input;
    try {
        operation();
        return true;
    } catch (...) {
        return false;
    }
}
}
