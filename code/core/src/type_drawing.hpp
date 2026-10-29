#pragma once
#include "type_drawing_context.hpp"
#include "presentation/context.hpp"
#include "yrpp/Matrix3D.h"
class ObjectClass;
struct VoxelStruct;
namespace game {
// Private, borrowed resources for original object/UI drawing. Original classes
// choose the palette role/index; hosts only resolve it and own the storage.
enum class DrawingPaletteKind { normal, animation, color_scheme };
struct ObjectVoxelRequest {
    const VoxelStruct* resource = nullptr;
    Matrix3D matrix;
    Point2D position{};
    RectangleStruct clip{};
    const DrawingPaletteHandle* palette = nullptr;
    unsigned frame = 0, flags = 0x800;
    int intensity = 1000;
    bool flat = false;
    bool shadow = false;
    bool apply_hva = true;
    int depth_adjustment = 0;
    int voxel_index = -1;
};
struct DrawingResources {
    void* context = nullptr;
    DrawingStatus (*palette)(void*, DrawingPaletteKind, int, const DrawingPaletteHandle*&) noexcept = nullptr;
    DrawingStatus (*voxel)(void*, const ObjectVoxelRequest&) noexcept = nullptr;
    const Matrix3D* voxel_camera = nullptr;
    const BytePalette* normal_palette = nullptr;
    RectangleStruct clip{};
};
// Runs `operation` with a context that names `context` as its type drawing target and accumulates the
// status of the lines, text and palette lookups submitted through it. The scopes `ctx` already holds
// (frame, resources) are inherited; the callback is synchronous, an exception becomes backend_failure.
// The first failed entry is reported; unsupported modes must never be retried through the original EXE.
DrawingStatus with_type_drawing(const TypeDrawingContext&, void (*operation)(void*, PresentationContext&), void*,
                                PresentationContext = {}) noexcept;
// The same, with the borrowed object resources bound in the context.
DrawingStatus with_drawing_resources(const DrawingResources&, const TypeDrawingContext&,
                                     void (*operation)(void*, PresentationContext&), void*,
                                     PresentationContext = {}) noexcept;
bool resolve_drawing_palette(PresentationContext&, DrawingPaletteKind, int, const DrawingPaletteHandle*&) noexcept;
// Folds a result into the status of the type drawing scope the context is in; nothing outside one.
void record_type_drawing_result(PresentationContext&, DrawingStatus) noexcept;
// Status accumulated by that scope so far; skipped when the context is in none.
DrawingStatus type_drawing_status(const PresentationContext&) noexcept;
bool drawing_completed(DrawingStatus) noexcept;
// Internal, per-thread frame diagnostics; never add original object pointers or
// std::string to the host drawing ABI. The first failure survives later passes.
void clear_drawing_failure() noexcept;
void set_drawing_diagnostic_frame(std::int32_t frame) noexcept;
const char* drawing_failure() noexcept;
DrawingStatus record_drawing_failure(DrawingStatus, const char* stage, const ObjectClass* object = nullptr,
                                     const ShapeDrawingRequest* shape = nullptr) noexcept;
}
