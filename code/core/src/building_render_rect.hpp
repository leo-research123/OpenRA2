#pragma once
// The rectangles the original building bounds queries keep in memo caches: BuildingTypeClass::GetDrawRect
// (YR 0x00465570) stores its result in the type, BuildingClass::GetRenderDimensions (YR 0x00455C20) stores the
// screen rectangle, the render coordinates and the camera it was computed for in the building. A cache entry
// never changes the answer, so the drawing asks for the value without writing it (the drawing must not write game
// state); the original methods use the same functions for their own computation.
#include "yrpp/BasicStructures.h"
#include "yrpp/GeneralStructures.h"
class BuildingClass;
class BuildingTypeClass;
namespace game {
struct PresentationState;
// The computation of GetDrawRect (union of the frame bounds of the image, buildup and bib), ignoring the cache.
// False when the type has no image (the original then returns an empty rectangle and stores nothing).
bool compute_building_type_draw_rect(BuildingTypeClass&, RectangleStruct& out);
// Preserve original memo priority; a bound presentation may cache the computed type bounds outside game state.
RectangleStruct building_type_draw_rect(BuildingTypeClass&, PresentationState* state = nullptr);
// GetRenderDimensions' computation for a render position and camera: the type's draw rectangle moved to the
// projected position.
RectangleStruct place_building_draw_rect(RectangleStruct type_rect, const CoordStruct& render, const Point2D& camera);
// The rectangle GetRenderDimensions returns now (the cached one while its render position and camera are still
// current, else computed); nothing is stored.
RectangleStruct building_render_rect(const BuildingClass&, PresentationState* state = nullptr);
}
