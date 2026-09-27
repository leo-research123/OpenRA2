#pragma once
#include "api/map_view.hpp"
#include "ui_resources.hpp"
#include "yrpp/GadgetClass.h"
class ColorScheme;

namespace game {
// Borrowed event scope, not another input or widget state owner.
struct GameUiInput {
    Point2D point{};
    DWORD key=0;
    GadgetFlag flags{};
    KeyModifier modifier{};
    // Native equivalent of the original hWnd-presence gate. This is borrowed
    // device context, not a fabricated Windows handle or game visibility flag.
    bool has_window=true;
};
const GameUiInput* game_ui_input() noexcept;
bool with_game_ui_input(const GameUiInput&,void (*operation)()) noexcept;
// Synchronous original Draw operation. Owns no world/UI state. Original class
// methods submit to the selected generic backend through this borrowed scope.
// Borrowed device composition. TextLabel/OwnerDraw retain all edit/scroll
// state; the host supplies only the current IME preedit and UTF-16-unit cursor.
// Native hosts without an IME context leave it null, as the original hIMC=0.
struct TextComposition { const wchar_t* text=nullptr; int cursor=0; };
struct GameUiFrame {
    const MapDrawingContext& drawing;
    UiResources& resources;
    MapDrawStatistics& statistics;
    DrawingStatus status=DrawingStatus::skipped;
    const DrawingPaletteHandle* palette=nullptr;
    // One original presentation phase per main-loop iteration. Additional
    // host repaints compose current state without advancing event animation.
    bool advance_presentation=false;
    const TextComposition* composition=nullptr;
};
GameUiFrame* game_ui_frame() noexcept;
DrawingStatus with_game_ui_frame(GameUiFrame&,void (*operation)()) noexcept;
void draw_ui_shape(SHPStruct*,const Point2D&,int frame=0) noexcept;
void draw_ui_shape(UiImage,const Point2D&,int frame=0) noexcept;
void draw_ui_fill(const RectangleStruct&,WORD color) noexcept;
void draw_ui_bitmap(const Point2D&,int width,int height,const WORD*,unsigned count) noexcept;
// Original text convention: position is relative to clip; flags select alignment.
void draw_ui_text(const wchar_t*,const RectangleStruct& clip,const Point2D&,
    const ColorScheme*,TextPrintType) noexcept;
bool ui_palette_color(unsigned index,WORD& color) noexcept;
void record_ui_drawing(DrawingStatus) noexcept;
}
