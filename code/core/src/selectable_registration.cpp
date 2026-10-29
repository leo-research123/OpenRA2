#include "selectable_registration.hpp"
#include "tactical_spatial.hpp"
#include "techno_drawing.hpp"
#include "map_runtime.hpp"
#include "debug/debug_state.hpp"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/FileFormats/HVA.h"
#include <cstring>
#include <stdexcept>

namespace game {
bool unit_selectable_anchor(const UnitClass& unit, Point2D input, Point2D& anchor) {
    if (!unit.Type || !unit.Locomotor) return false;
    anchor = input;
    anchor.Y -= unit.DrawingYOffset;
    if (unit.IsOnCarryall) anchor.Y += 14;
    return unit.VisualCharacter(false, nullptr) != VisualType::Hidden && !unit.Deploying && !unit.Undeploying;
}
DrawingStatus infantry_drop_pod(const InfantryClass& infantry, bool& airborne) {
    airborne = false;
    if (!infantry.Locomotor) return DrawingStatus::unavailable;
    constexpr GUID iid{0x10C, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    struct Held {
        IPersist* pointer = nullptr;
        ~Held() {
            if (pointer) pointer->Release();
        }
    } persist;
    if (infantry.Locomotor->QueryInterface(iid, reinterpret_cast<void**>(&persist.pointer)) < 0 || !persist.pointer)
        return DrawingStatus::backend_failure;
    CLSID clsid{};
    persist.pointer->GetClassID(&clsid);
    airborne = infantry.GetHeight() > 0 && !std::memcmp(&clsid, &LocomotionClass::CLSIDs::Droppod, sizeof(clsid));
    return DrawingStatus::drawn;
}
DrawingStatus aircraft_draw_anchor(const AircraftClass& aircraft, TechnoDrawing& d, Point2D input, Point2D& anchor,
                                   int& frame) {
    if (!aircraft.Type || !aircraft.Locomotor) return DrawingStatus::invalid_argument;
    if (!d.debug_map && d.window_active && d.fog_of_war) {
        auto destination = aircraft.Locomotor->Head_To_Coord();
        destination.Z = aircraft.Location.Z;
        if (techno_location_fogged(destination) && techno_location_fogged(aircraft.Location) &&
            (!aircraft.Owner || !aircraft.Owner->IsControlledByCurrentPlayer()))
            return DrawingStatus::skipped;
    }
    auto* apparent = static_cast<TechnoTypeClass*>(aircraft.Type);
    if (!aircraft.IsClearlyVisibleTo(HouseClass::CurrentPlayer))
        if (auto* disguise = aircraft.GetDisguise(true)) apparent = static_cast<TechnoTypeClass*>(disguise);
    if (!apparent->MainVoxel.HVA || apparent->MainVoxel.HVA->FrameCount <= 0) return DrawingStatus::skipped;
    frame = aircraft.WalkedFramesSoFar % apparent->MainVoxel.HVA->FrameCount;
    anchor = input;
    const auto offset = aircraft.Locomotor->Draw_Point();
    anchor.X += offset.X;
    anchor.Y += offset.Y;
    return DrawingStatus::drawn;
}
namespace {
void register_body(ObjectClass& object, Point2D input, TechnoDrawing& d, unsigned nesting = 0) {
    if (nesting > 32) throw std::runtime_error("Cyclic carryall passenger drawing");
    auto& tactical = *TacticalClass::Instance;
    Point2D anchor = input;
    switch (object.WhatAmI()) {
        case AbstractType::Unit:
            if (unit_selectable_anchor(static_cast<UnitClass&>(object), input, anchor))
                tactical.AddSelectable(&object, anchor.X, anchor.Y);
            break;
        case AbstractType::Infantry: {
            auto& infantry = static_cast<InfantryClass&>(object);
            if (!infantry.Type || !techno_drawing_cell(infantry)) return;
            if (infantry.TubeIndex == -1) {
                bool airborne = false;
                const auto result = infantry_drop_pod(infantry, airborne);
                if (!techno_drawing_complete(result)) {
                    record_techno_drawing(d, result);
                    return;
                }
                if (airborne || !infantry.GetImage()) return;
            }
            tactical.AddSelectable(&object, anchor.X, anchor.Y);
            break;
        }
        case AbstractType::Aircraft: {
            auto& aircraft = static_cast<AircraftClass&>(object);
            int frame = 0;
            const auto result = aircraft_draw_anchor(aircraft, d, input, anchor, frame);
            if (result != DrawingStatus::drawn) {
                record_techno_drawing(d, result);
                return;
            }
            if (aircraft.Passengers.NumPassengers && aircraft.Type->Carryall && aircraft.Passengers.FirstPassenger)
                register_body(*aircraft.Passengers.FirstPassenger, anchor, d, nesting + 1);
            if (!techno_drawing_complete(d.status) || !aircraft.Type->Voxel || !aircraft.Type->MainVoxel.VXL) return;
            if (!MapClass::Instance.TryGetCellAt(aircraft.Location)) {
                record_techno_drawing(d, DrawingStatus::unavailable);
                return;
            }
            tactical.AddSelectable(&object, anchor.X, anchor.Y);
            break;
        }
        default:
            break;
    }
}
}
void rebuild_tactical_selectables(TacticalClass& tactical) {
    const auto& runtime = map_runtime();
    if (!runtime.drawing_bounds) {
        tactical.SelectableCount = 0;
        invalidate_selectable_index();
        return;
    }
    flush_tactical_spatial();
    TechnoDrawing drawing;
    drawing.window_active = runtime.has_window && runtime.has_window();
    drawing.debug_map = runtime.debug_map && *runtime.debug_map;
    drawing.fog_of_war =
        !debug_fog_hidden() && ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar;
    struct Call {
        TacticalClass& tactical;
        TechnoDrawing& drawing;
        RectangleStruct bounds;
    } call{tactical, drawing, *runtime.drawing_bounds};
    const auto result = with_techno_drawing(
        drawing,
        [](void* data, game::PresentationContext& ctx) {
            auto& call = *static_cast<Call*>(data);
            auto& tactical = call.tactical;
            auto& drawing = call.drawing;
            tactical.SelectableCount = 0;
            invalidate_selectable_index();
            tactical.AddBuildingsToSelectables(TacticalClass::ViewBounds);
            const auto region = tactical_object_bounds(tactical, call.bounds);
            TacticalLayerQuery query;
            for (int layer = 0; layer < 5; ++layer) {
                query.begin(layer, region, false);
                int index = 0;
                ObjectClass* object = nullptr;
                while (query.next(object, index)) {
                    if ((object->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None) continue;
                    const auto at = object->Location;
                    if (at.X < region.left || at.X >= region.right || at.Y < region.top || at.Y >= region.bottom)
                        continue;
                    if (drawing.fog_of_war && techno_location_fogged(at)) continue;
                    const auto center = object->GetCoords();
                    Point2D point;
                    if (!tactical.CoordsToClient(&center, &point)) continue;
                    // DrawIfVisible's registration gate and anchor, without changing
                    // NeedsRedraw/IsVisible, shadows, animation or rendering resources.
                    if (!drawing.debug_map && drawing.window_active && object->InLimbo) continue;
                    const auto rendered = object->GetRenderCoords();
                    if (!tactical.CoordsToClient(&rendered, &point)) continue;
                    register_body(*object, point, drawing);
                    if (!techno_drawing_complete(drawing.status)) return;
                }
            }
        },
        &call);
    if (!techno_drawing_complete(result)) throw std::runtime_error("Independent selectable registration failed");
}
}
