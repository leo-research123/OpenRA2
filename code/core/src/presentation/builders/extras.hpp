#pragma once
#include "yrpp/GeneralStructures.h"
class TechnoClass; class BuildingClass; class FootClass; class CaptureManagerClass;
namespace game {
struct PresentationContext;
// YR 0x6DBB60: explicit world-space line terminal.
bool build_world_line(PresentationContext&, const CoordStruct&, const CoordStruct&, unsigned);
void build_selection_bracket(PresentationContext& ctx, CoordStruct* from, CoordStruct* to, int color);
void build_techno_extras(PresentationContext& ctx, const TechnoClass& self, Point2D* point, RectangleStruct* bounds);
void build_veterancy_pips(PresentationContext& ctx, const TechnoClass& self, Point2D* point, RectangleStruct* bounds);
void build_extra_info(PresentationContext& ctx, const TechnoClass& self, const Point2D& origin, const Point2D& center, const RectangleStruct& bounds);
void build_health_bar(PresentationContext& ctx, const TechnoClass& self, Point2D* location, RectangleStruct* bounds, bool hover);
void build_health_pips(PresentationContext& ctx, const TechnoClass& self, Point2D* location, Point2D* center, RectangleStruct* bounds);
void build_techno_behind(PresentationContext& ctx, const TechnoClass& self, Point2D* point, RectangleStruct* bounds);
void build_building_info(PresentationContext& ctx, const BuildingClass& self, Point2D* point, RectangleStruct* bounds);
void build_action_line(PresentationContext& ctx, CoordStruct from, CoordStruct to, ColorStruct color, bool dashed, bool shadow);
void build_mind_control_line(PresentationContext& ctx, CoordStruct from, CoordStruct to, ColorStruct color);
void build_action_lines(PresentationContext& ctx, const FootClass& self, bool force, DWORD dashed);
void build_capture_links(PresentationContext& ctx, const CaptureManagerClass& self);
}
