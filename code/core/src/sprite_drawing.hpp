#pragma once
#include "presentation/context.hpp"
#include "type_drawing_context.hpp"
class ObjectClass;
class AnimClass;
class CellClass;
class BuildingClass;
namespace game {
// Borrowed synchronous services for typed animation and terrain extraction.
// Original objects retain their state and lifetime.
enum class SpritePalette { cell, animation, tiberium, neutral, player, alternative };
struct SpriteTriangleVertex {
    float x, y, z, rhw;
    unsigned color, specular;
    float u, v;
};
struct SpriteTriangle {
    SpriteTriangleVertex vertices[3];
};
static_assert(sizeof(SpriteTriangle) == 0x60);
struct SpriteDrawing {
    TypeDrawingContext types;
    void* context = nullptr;
    CellClass* (*cell_at)(void*, const CellStruct&) noexcept = nullptr;
    CellClass* (*cell_at_world)(void*, const CoordStruct&) noexcept = nullptr;
    DrawingStatus (*initialize_light)(void*, CellClass&) noexcept = nullptr;
    DrawingStatus (*palette)(void*, SpritePalette, const AnimClass*, CellClass*,
                             const DrawingPaletteHandle*&) noexcept = nullptr;
    SHPStruct* (*shape_data)(SHPStruct*) = nullptr;
    RectangleStruct (*frame_bounds)(void*, SHPStruct*, int) = nullptr;
    int (*height)(int) noexcept = nullptr;
    bool (*reduce_effects)(void*) noexcept = nullptr;
    BuildingClass* (*building)(void*, CellClass&) noexcept = nullptr;
    bool (*shrouded)(void*, CellClass&) noexcept = nullptr;
    int (*pixel_format)(void*) noexcept = nullptr;
    // Original ColorAdd bytes are already component values for packed color;
    // DrawIt intentionally preserves its fall-through OR conversions.
    const unsigned char* laser_color = nullptr;
    const unsigned char* shield_color = nullptr;
    int detail_level = 2;
    bool draw_shadows = true;
    RectangleStruct tactical_rect{};
    // The legacy Direct3D RING1 path precedes every ordinary visibility test.
    bool hardware = false, depth_available = false;
    bool (*is_ring)(void*, const AnimClass&) noexcept = nullptr;
    DrawingStatus (*triangle)(void*, const SpriteTriangle&) noexcept = nullptr;
    int frame = 0, depth_origin = 0;
    DrawingStatus status = DrawingStatus::skipped;
};
DrawingStatus with_sprite_drawing(SpriteDrawing&, void (*)(void*, PresentationContext&), void*,
                                          PresentationContext = {}) noexcept;
DrawingStatus draw_object_sprite(const ObjectClass&, SpriteDrawing&, Point2D, RectangleStruct,
                                 bool force_draw = false, PresentationContext outer = {}) noexcept;
bool sprite_drawing_complete(DrawingStatus) noexcept;
bool sprite_submit(SpriteDrawing&, SpritePalette, const AnimClass*, CellClass*, ShapeDrawingRequest&) noexcept;
bool sprite_initialize_light(SpriteDrawing&, CellClass&) noexcept;
}
