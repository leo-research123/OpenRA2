#pragma once
#include "presentation/context.hpp"
#include "type_drawing_context.hpp"
#include "yrpp/BasicStructures.h"
class BuildingClass;
class TechnoClass;
namespace game {
// Borrowed drawing scope for the original TechnoClass extras: health bar,
// veteran pips, repair wrench and the building selection box. Resource ownership
// and backend errors remain on this side of the host ABI.
struct TechnoExtrasDrawing {
    const TypeDrawingContext& drawing;
    SHPStruct* pips = nullptr;
    const DrawingPaletteHandle* palette = nullptr;
    DrawingStatus status = DrawingStatus::skipped;
    SHPStruct* wrench = nullptr;
    const DrawingPaletteHandle* wrench_palette = nullptr;
    const BytePalette* selection_palette = nullptr;
    Point2D camera{};
    void* line_context = nullptr;
    void (*world_line)(void*, const CoordStruct&, const CoordStruct&, unsigned) = nullptr;
    SHPStruct* pip_border = nullptr;
    SHPStruct* mobile_pips = nullptr;
};
// Passes the borrowed extras services explicitly and binds the terminal device.
// No class-specific drawing context is stored in thread-local state.
// The returned status folds in lines submitted through the drawing scope.
DrawingStatus with_techno_extras_drawing(TechnoExtrasDrawing&, void (*)(void*, PresentationContext&), void*,
                                          PresentationContext = {}) noexcept;
// Folds lines submitted through the context's drawing scope into the frame status.
void collect_line_status(const PresentationContext&, TechnoExtrasDrawing&) noexcept;
DrawingStatus draw_building_health(const BuildingClass&, const TypeDrawingContext&, SHPStruct*,
                                   const DrawingPaletteHandle*, Point2D, RectangleStruct) noexcept;
DrawingStatus draw_techno_extras(const TechnoClass&, TechnoExtrasDrawing&, Point2D, RectangleStruct) noexcept;
// Screen-format colour of PALETTE.PAL entry `index` (RGB565), the value the
// original reads from the pixel-format table of the selection palette.
unsigned selection_color(const TechnoExtrasDrawing&, int index) noexcept;
}
