#pragma once
#include "api/type_drawing.hpp"
class ObjectClass;
namespace game {
// Private, borrowed resources for original object/UI drawing. Original classes
// choose the palette role/index; hosts only resolve it and own the storage.
enum class DrawingPaletteKind { normal, animation, color_scheme };
struct DrawingResources {
    void* context = nullptr;
    DrawingStatus (*palette)(void*, DrawingPaletteKind, int,
        const DrawingPaletteHandle*&) noexcept = nullptr;
    const BytePalette* normal_palette = nullptr;
    RectangleStruct clip{};
};
const DrawingResources* active_drawing_resources() noexcept;
DrawingStatus with_drawing_resources(const DrawingResources&, const TypeDrawingContext&,
    void (*operation)(void*), void*) noexcept;
bool resolve_drawing_palette(DrawingPaletteKind, int, const DrawingPaletteHandle*&) noexcept;
const TypeDrawingContext* active_type_drawing() noexcept;
void record_type_drawing_result(DrawingStatus) noexcept;
bool drawing_completed(DrawingStatus) noexcept;
// Internal, per-thread frame diagnostics; never add original object pointers or
// std::string to the host drawing ABI. The first failure survives later passes.
void clear_drawing_failure() noexcept;
const char* drawing_failure() noexcept;
DrawingStatus record_drawing_failure(DrawingStatus, const char* stage,
    const ObjectClass* object=nullptr, const ShapeDrawingRequest* shape=nullptr) noexcept;
}
