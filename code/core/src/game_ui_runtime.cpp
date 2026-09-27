#include "game_ui_runtime.hpp"
#include "yrpp/Surface.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Drawing.h"

namespace game {
namespace { thread_local GameUiFrame* active=nullptr; }
GameUiFrame* game_ui_frame() noexcept { return active; }
void record_ui_drawing(DrawingStatus result) noexcept {
    if (!active || (active->status!=DrawingStatus::drawn && active->status!=DrawingStatus::skipped)) return;
    if (result!=DrawingStatus::skipped) active->status=result;
}
DrawingStatus with_game_ui_frame(GameUiFrame& frame,void (*operation)()) noexcept {
    if (!operation || !frame.drawing.plain_palette) return DrawingStatus::invalid_argument;
    const auto prepared=frame.drawing.plain_palette(frame.drawing.types.backend_context,
        frame.resources.palette(),frame.palette);
    if (prepared!=DrawingStatus::drawn) return prepared;
    const auto previous=active;
    auto* previous_font=BitFont::Instance;
    if(frame.resources.font())BitFont::Instance=frame.resources.font();
    active=&frame;
    try { operation(); }
    catch (...) { frame.status=DrawingStatus::backend_failure; }
    active=previous;
    BitFont::Instance=previous_font;
    return frame.status;
}
void draw_ui_shape(UiImage id,const Point2D& position,int index) noexcept {
    if (!active) return;
    auto* image=active->resources.image(id);
    if (!image) return;
    draw_ui_shape(image,position,index);
}
void draw_ui_shape(SHPStruct* image,const Point2D& position,int index) noexcept {
    if (!active || !image) return;
    ShapeDrawingRequest request;
    request.target=active->drawing.types.target; request.palette=active->palette;
    request.image=image; request.frame=index;
    request.position=position; request.clip=DSurface::WindowBounds; request.flags=0;
    record_ui_drawing(submit_type_shape(active->drawing.types,request));
}
void draw_ui_fill(const RectangleStruct& rect,WORD color) noexcept {
    if (!active) return;
    RasterDrawingRequest request;
    request.target=active->drawing.types.target; request.position={rect.X,rect.Y};
    request.clip=DSurface::WindowBounds; request.width=rect.Width; request.height=rect.Height; request.color=color;
    record_ui_drawing(submit_type_raster(active->drawing.types,request));
}
void draw_ui_bitmap(const Point2D& position,int width,int height,const WORD* pixels,unsigned count) noexcept {
    if (!active) return;
    RasterDrawingRequest request;
    request.target=active->drawing.types.target; request.position=position;
    request.clip=DSurface::WindowBounds; request.width=width; request.height=height; request.pitch=width;
    request.pixels=pixels; request.pixel_count=count;
    record_ui_drawing(submit_type_raster(active->drawing.types,request));
}
namespace {
WORD pack_color(const ColorStruct& color) noexcept {
    return WORD((unsigned(color.R)>>Drawing::RedShiftRight)<<Drawing::RedShiftLeft |
        (unsigned(color.G)>>Drawing::GreenShiftRight)<<Drawing::GreenShiftLeft |
        (unsigned(color.B)>>Drawing::BlueShiftRight)<<Drawing::BlueShiftLeft);
}
}
bool ui_palette_color(unsigned index,WORD& color) noexcept {
    if (!active || index>=256) return false;
    color=pack_color(active->resources.palette().Entries[index]);
    return true;
}
void draw_ui_text(const wchar_t* text,const RectangleStruct& clip,const Point2D& point,
    const ColorScheme* scheme,TextPrintType flags) noexcept {
    if (!active) return;
    auto* font=active->resources.font();
    if (!font || !scheme) {record_ui_drawing(DrawingStatus::unavailable);return;}
    if (!text) return;
    // 0x004A61C0 -> 0x004A5EB0 -> 0x00434500. YR's BitFont path
    // uses HSV BaseColor even with UseGradPal, and skips CR/LF while blitting.
    const WORD color=pack_color(ColorScheme::HSVToRGB(scheme->BaseColor));
    int width=0,height=0;
    if (!font->GetTextDimension(text,&width,&height,clip.Width)) {
        record_ui_drawing(DrawingStatus::unavailable);return;
    }
    int x=clip.X+point.X;
    const unsigned bits=static_cast<unsigned>(flags);
    if (bits&0x100) x-=width/2;
    else if (bits&0x200) x-=width;
    const int old_tab_origin=font->field_20;
    font->field_20=x;
    for (const auto* c=text;*c;++c) {
        if (*c==L'\r' || *c==L'\n') continue;
        int next=x;
        record_ui_drawing(font->SubmitGlyph(active->drawing.types,*c,x,clip.Y+point.Y,clip,color,next));
        x=next;
    }
    font->field_20=old_tab_origin;
}
}

namespace game {
namespace { thread_local const GameUiInput* input_frame=nullptr; }
const GameUiInput* game_ui_input() noexcept { return input_frame; }
bool with_game_ui_input(const GameUiInput& input,void (*operation)()) noexcept {
    if (!operation) return false;
    struct Restore { const GameUiInput* old; ~Restore(){input_frame=old;} } restore{input_frame};
    input_frame=&input;
    try { operation(); return true; } catch (...) { return false; }
}
}
