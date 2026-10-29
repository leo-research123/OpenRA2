#pragma once
#include "type_drawing_context.hpp"
class UnitClass;
class InfantryClass;
class AircraftClass;
namespace game {
struct TechnoDrawing;
bool unit_selectable_anchor(const UnitClass&, Point2D, Point2D&);
DrawingStatus infantry_drop_pod(const InfantryClass&, bool&);
DrawingStatus aircraft_draw_anchor(const AircraftClass&, TechnoDrawing&, Point2D, Point2D&, int&);
}
