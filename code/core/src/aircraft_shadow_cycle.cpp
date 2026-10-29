#include "aircraft_shadow_cycle.hpp"
#include "debug/debug_state.hpp"
#include "map_runtime.hpp"
#include "selectable_registration.hpp"
#include "tactical_spatial.hpp"
#include "techno_drawing.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Unsorted.h"
#include <vector>
namespace game {
namespace {
std::vector<AircraftShadowSample> samples;
}
AircraftCycleEnvironment aircraft_cycle_environment() noexcept {
    const auto& runtime = map_runtime();
    AircraftCycleEnvironment env;
    env.window_active = runtime.has_window && runtime.has_window();
    env.debug_map = runtime.debug_map && *runtime.debug_map;
    env.memory_fog = !debug_fog_hidden() && ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar;
    env.fog_of_war = env.memory_fog;
    env.level_height = Unsorted::LevelHeight;
    return env;
}
bool aircraft_reaches_shadow_cycle(const AircraftClass& aircraft, const AircraftCycleEnvironment& env,
                                   const TacticalWorldBounds& region, int& frame) {
    auto* tactical = TacticalClass::Instance;
    if (!tactical) return false;
    // TacticalClass::DrawObjects, Foot branch.
    const auto at = aircraft.Location;
    if (at.X < region.left || at.X >= region.right || at.Y < region.top || at.Y >= region.bottom) return false;
    if (env.memory_fog && debug_player_location_fogged(at)) return false;
    Point2D point;
    const auto center = aircraft.GetCoords();
    if (!tactical->CoordsToClient(&center, &point)) return false;
    // ObjectClass::DrawIfVisible with the forced flag set.
    if (!env.debug_map && env.window_active && aircraft.InLimbo) return false;
    CoordStruct render;
    if (!tactical->CoordsToClient(aircraft.GetRenderCoords(&render), &point)) return false;
    // AircraftClass::DrawIt up to the cycle.
    if (!aircraft.Type || !aircraft.Locomotor) return false;
    TechnoDrawing environment;
    environment.window_active = env.window_active;
    environment.debug_map = env.debug_map;
    environment.fog_of_war = env.fog_of_war;
    Point2D anchor;
    if (aircraft_draw_anchor(aircraft, environment, point, anchor, frame) != DrawingStatus::drawn) return false;
    if (!aircraft.Type->Voxel || !aircraft.Type->MainVoxel.VXL) return false;
    return MapClass::Instance.TryGetCellAt(aircraft.Location) != nullptr;
}
const AircraftShadowSample* aircraft_shadow_sample(const AircraftClass* aircraft) noexcept {
    for (const auto& sample : samples)
        if (sample.aircraft == aircraft) return &sample;
    return nullptr;
}
void run_aircraft_shadow_cycle() {
    samples.clear();
    // Every aircraft is registered in its class array when it is constructed; without one the layers hold none.
    if (AircraftClass::Array.Count == 0) return;
    auto* tactical = TacticalClass::Instance;
    const auto& runtime = map_runtime();
    if (!tactical || !runtime.drawing_bounds) return;
    const auto env = aircraft_cycle_environment();
    const int bridge_height = 4 * env.level_height;
    const auto region = tactical_object_bounds(*tactical, *runtime.drawing_bounds);
    for (int layer = 0; layer < 5; ++layer) {
        auto& objects = MapClass::ObjectsInLayers[layer];
        for (int index = 0; index < objects.Count; ++index) {
            auto* object = objects[index];
            if (!object || object->WhatAmI() != AbstractType::Aircraft) continue;
            auto& aircraft = static_cast<AircraftClass&>(*object);
            int frame = 0;
            if (!aircraft_reaches_shadow_cycle(aircraft, env, region, frame)) continue;
            // AircraftClass::DrawIt 0x4144B0: the original lowers the aircraft for the shadow and raises it again.
            const int saved_height = aircraft.GetHeight();
            const bool saved_frozen = aircraft.IsStandingStill();
            aircraft.FrozenStill = false;
            auto* cell = MapClass::Instance.TryGetCellAt(aircraft.Location);
            AircraftShadowSample sample;
            sample.aircraft = &aircraft;
            if ((!aircraft.OnBridge && saved_height >= bridge_height) || (aircraft.OnBridge && saved_height >= 0)) {
                const auto flags = unsigned(cell->Flags);
                if (flags & 0x100u) {
                    const auto* neighbor =
                        cell->GetNeighbourCell((flags & 0x800u) ? FacingType::North : FacingType::West);
                    sample.raised = neighbor && (unsigned(neighbor->Flags) & 0x100u);
                }
            }
            aircraft.SetHeight(sample.raised ? DWORD(bridge_height) : 0);
            sample.key = VoxelIndexKey(frame & 0x1F);
            sample.local = aircraft.Locomotor->Shadow_Matrix(&sample.key);
            sample.depth = aircraft.GetZAdjustment();
            sample.has_shadow = aircraft.Locomotor->Is_To_Have_Shadow();
            sample.shadow_delta = aircraft.Locomotor->Shadow_Point();
            aircraft.SetHeight(DWORD(saved_height));
            aircraft.FrozenStill = saved_frozen;
            samples.push_back(sample);
        }
    }
}
}
