#pragma once
#include "presentation/context.hpp"
#include "type_drawing_context.hpp"
class CellClass;
namespace game {
// Internal synchronous host contract. Original overlay decisions and cache
// fields stay in CellClass. No Godot or replacement cell/type model is involved.
enum class OverlayPalette { cell, theater, wall };
struct OverlayDrawing {
    TypeDrawingContext types;
    void* context = nullptr;
    DrawingStatus (*initialize_light)(void*, CellClass&) noexcept = nullptr;
    DrawingStatus (*palette)(void*, CellClass&, OverlayPalette, const DrawingPaletteHandle*&) noexcept = nullptr;
    // YR 0x00AA105C: slot zero is null, slots 1..4 are SLOP01Z..04Z.
    SHPStruct* slope_depth[5]{};
    int frame = 0;
    unsigned char redraws = 0;
    RectangleStruct tactical_rect{};
    DrawingStatus status = DrawingStatus::skipped;
};
DrawingStatus with_overlay_drawing(OverlayDrawing&, void (*)(void*, PresentationContext&), void*,
                                          PresentationContext = {}) noexcept;
Point2D cell_overlay_offset(const CellClass&, int top);
RectangleStruct cell_overlay_bounds(const CellClass&, int top, bool shadow);
// Both host and original-game adapter enter the same original methods here.
// No exception may escape this boundary; callbacks must not throw.
DrawingStatus draw_cell_overlay(CellClass&, OverlayDrawing&, const Point2D&, const RectangleStruct&,
                                bool shadow, PresentationContext outer = {}) noexcept;
}
