#pragma once
#include "api/type_drawing.hpp"
#include "yrpp/BasicStructures.h"
#include <array>
class BuildingClass;class TechnoClass;
namespace game {
// Borrowed drawing scope for the original TechnoClass::DrawHealthBar entry.
// Resource ownership and backend errors remain on this side of the host ABI.
struct BuildingHealthDrawing {
    const TypeDrawingContext& drawing;
    SHPStruct* pips=nullptr;
    const DrawingPaletteHandle* palette=nullptr;
    DrawingStatus status=DrawingStatus::skipped;
    SHPStruct* wrench=nullptr;
    const DrawingPaletteHandle* wrench_palette=nullptr;
    const BytePalette* selection_palette=nullptr;
    Point2D camera{};
    SHPStruct* pip_border=nullptr;
    SHPStruct* mobile_pips=nullptr;
};
DrawingStatus draw_depth_glow_line(const TypeDrawingContext&, const RectangleStruct&, Point2D, Point2D, int, int, int) noexcept;
DrawingStatus draw_depth_alpha_line(const TypeDrawingContext&, const RectangleStruct&, Point2D, Point2D, int, int, ColorStruct, int) noexcept;
BuildingHealthDrawing* building_health_drawing() noexcept;
DrawingStatus with_building_health_drawing(BuildingHealthDrawing&,void (*)(void*),void*) noexcept;
DrawingStatus draw_building_health(const BuildingClass&, const TypeDrawingContext&,
    SHPStruct*, const DrawingPaletteHandle*, Point2D, RectangleStruct) noexcept;
DrawingStatus draw_building_extras(const BuildingClass&,BuildingHealthDrawing&,Point2D,RectangleStruct) noexcept;
DrawingStatus draw_techno_extras(const TechnoClass&,BuildingHealthDrawing&,Point2D,RectangleStruct) noexcept;
// Native world-space endpoints; independent of SHP crop, camera and renderer.
struct BuildingSelectionEdge { CoordStruct first{}, last{}; };
struct BuildingSelectionGeometry {
    std::array<BuildingSelectionEdge, 11> edges{};
    unsigned count = 0;
    unsigned palette_index = 15;
};
// 0x006F5190 (building branch), 0x006F5EF0 and 0x00464AF0.
bool building_selection_geometry(const BuildingClass&, BuildingSelectionGeometry&) noexcept;
DrawingStatus draw_building_selection(const TypeDrawingContext&, const RectangleStruct&,
    const Point2D& camera, const BuildingSelectionGeometry&, const BytePalette&) noexcept;
}
