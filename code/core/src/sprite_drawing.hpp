#pragma once
#include "api/type_drawing.hpp"
class ObjectClass;
class AnimClass;
class CellClass;
class BuildingClass;
namespace game {
// Borrowed synchronous services for the original Anim/Terrain DrawIt methods.
// Objects, animation state, branches and draw ordering remain in those classes.
enum class SpritePalette { cell, animation, tiberium, neutral, player, alternative };
struct SpriteTriangleVertex {
    float x, y, z, rhw;
    unsigned color, specular;
    float u, v;
};
struct SpriteTriangle { SpriteTriangleVertex vertices[3]; };
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
    RectangleStruct (*frame_bounds)(SHPStruct*, int) = nullptr;
    int (*height)(int) noexcept = nullptr;
    bool (*reduce_effects)(void*) noexcept = nullptr;
    BuildingClass* (*building)(CellClass&) noexcept = nullptr;
    bool (*shrouded)(CellClass&) noexcept = nullptr;
    int (*pixel_format)() noexcept = nullptr;
    // Original ColorAdd bytes are already component values for packed color;
    // DrawIt intentionally preserves its fall-through OR conversions.
    const unsigned char* laser_color = nullptr;
    const unsigned char* shield_color = nullptr;
    int detail_level = 2;
    bool draw_shadows = true;
    RectangleStruct tactical_rect{};
    // The legacy Direct3D RING1 path precedes every ordinary visibility test.
    bool hardware = false, depth_available = false;
    bool (*is_ring)(const AnimClass&) noexcept = nullptr;
    DrawingStatus (*triangle)(void*, const SpriteTriangle&) noexcept = nullptr;
    int frame = 0, depth_origin = 0;
    // GPU hosts bind an existing owner converter identity here; no fabricated
    // LightConvert object is stored in the original animation.
    const DrawingPaletteHandle* alternative_palette = nullptr;
    int alternative_intensity = 1000;
    DrawingStatus status = DrawingStatus::skipped;
};
SpriteDrawing* sprite_drawing() noexcept;
DrawingStatus with_sprite_drawing(SpriteDrawing&,void(*)(void*),void*) noexcept;
DrawingStatus draw_object_sprite(const ObjectClass&, SpriteDrawing&, Point2D, RectangleStruct) noexcept;
bool sprite_drawing_complete(DrawingStatus) noexcept;
bool sprite_submit(SpriteDrawing&, SpritePalette, const AnimClass*, CellClass*,
    ShapeDrawingRequest&) noexcept;
bool sprite_initialize_light(SpriteDrawing&, CellClass&) noexcept;
}
