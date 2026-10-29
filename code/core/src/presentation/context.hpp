#pragma once
// The drawing scopes one builder call runs inside. A builder never looks the scopes up itself: the
// dispatcher that calls it hands this struct in, so what a builder may use is visible in its signature.
#include "type_drawing_context.hpp"
class ObjectClass;
class UnitTypeClass;
namespace game {
struct PresentationFrame;
struct TechnoDrawing;
struct TechnoExtrasDrawing;
struct SpriteDrawing;
struct BuildingDrawing;
struct OverlayDrawing;
struct TypeDrawingContext;
struct DrawingResources;
struct PresentationContext {
    UnitTypeClass* unit_type = nullptr;
    bool unit_terrain_palette = false;
    PresentationFrame* frame = nullptr;
    TechnoDrawing* techno = nullptr;
    TechnoExtrasDrawing* extras = nullptr;
    SpriteDrawing* sprite = nullptr;
    BuildingDrawing* building = nullptr;
    OverlayDrawing* overlay = nullptr;
    const TypeDrawingContext* type = nullptr;
    // The status the type drawing scope in progress accumulates (with_type_drawing); null outside one.
    DrawingStatus* type_status = nullptr;
    const DrawingResources* resources = nullptr;
};
// What a nested drawing entry inherits from its caller: the frame, the type context with its status and
// the resources. The class scopes (techno, sprite, building, overlay, extras) belong to the call that
// opened them and are not handed on.
inline PresentationContext inherit_scopes(const PresentationContext& outer) noexcept {
    PresentationContext ctx;
    ctx.frame = outer.frame;
    ctx.type = outer.type;
    ctx.type_status = outer.type_status;
    ctx.resources = outer.resources;
    return ctx;
}
}
