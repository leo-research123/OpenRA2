#include "overlay_drawing.hpp"
#include "yrpp/CellClass.h"
namespace game {
namespace { thread_local OverlayDrawing* current = nullptr; }
OverlayDrawing* overlay_drawing() noexcept { return current; }
DrawingStatus draw_cell_overlay(CellClass& cell, OverlayDrawing& drawing,
        const Point2D& point, const RectangleStruct& clip, bool shadow) noexcept {
    auto* previous = current;
    current = &drawing;
    drawing.status = DrawingStatus::skipped;
    try {
        if (shadow) cell.DrawOverlayShadow(point, clip);
        else cell.DrawOverlay(point, clip);
    } catch (...) { drawing.status = DrawingStatus::backend_failure; }
    current = previous;
    return drawing.status;
}
}
