// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tactical.cpp::Draw_Objects; calibrated to YR 0x006D8DB0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR adds world-space rejection, IsVisible, and a separate second Extras pass.
#include "presentation/tactical_presenter.hpp"
#include "presentation/visibility.hpp"
#include "tactical_spatial.hpp"
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_runtime.hpp"
#if !defined(RA2_YRPP_GAME)
#include "presentation/state.hpp"
#endif
#include <array>
#include "tactical_drawing.hpp"
#include "debug/debug_state.hpp"
#include <bit>

namespace game {
void present_objects(PresentationContext& ctx, TacticalClass& tactical, bool forced) {
    const auto& runtime = map_runtime();
    if (!runtime.drawing_bounds) return;
    auto& visible = visible_objects();
#if defined(RA2_YRPP_GAME)
    const auto set_visible = [&](const ObjectClass* object, bool value, int, int) { visible.set(object, value); };
#else
    std::vector<std::uint8_t> local_visibility;
    std::vector<ObjectClass*> local_buildings;
    auto* state = ctx.frame ? ctx.frame->state : nullptr;
    auto& flags = state ? state->layer_visibility : local_visibility;
    auto& visible_buildings = state ? state->visible_buildings : local_buildings;
    std::array<std::size_t, 6> offsets{};
    for (int layer = 0; layer < 5; ++layer)
        offsets[layer + 1] = offsets[layer] + MapClass::ObjectsInLayers[layer].Count;
    flags.assign(offsets.back(), 0);
    const auto layer_revision = tactical_spatial_layer_revision();
    const auto set_visible = [&](const ObjectClass* object, bool value, int layer, int index) {
        visible.set(object, value);
        if (index >= 0 && std::size_t(index) < offsets[layer + 1] - offsets[layer])
            flags[offsets[layer] + index] = value;
    };
#endif
    auto bounds = *runtime.drawing_bounds;
#if defined(RA2_YRPP_GAME)
    const auto wrap = [](unsigned value) { return std::bit_cast<int>(value); };
    const auto top = tactical.ApplyMatrix_Pixel({wrap(unsigned(tactical.TacticalPos.X) + unsigned(bounds.Width / 2)),
                                        wrap(unsigned(tactical.TacticalPos.Y) - unsigned(bounds.Height))});
    const auto right = tactical.ApplyMatrix_Pixel({wrap(unsigned(tactical.TacticalPos.X) + 2u * unsigned(bounds.Width)),
                                          wrap(unsigned(tactical.TacticalPos.Y) + unsigned(bounds.Height / 2))});
    const auto left = tactical.ApplyMatrix_Pixel({wrap(unsigned(tactical.TacticalPos.X) - unsigned(bounds.Width)),
                                         wrap(unsigned(tactical.TacticalPos.Y) + unsigned(bounds.Height / 2))});
    const TacticalWorldBounds region{top.X, top.Y,
                                           wrap(unsigned(top.X) + 2u * (unsigned(right.X) - unsigned(top.X))),
                                           wrap(unsigned(top.Y) + 2u * (unsigned(left.Y) - unsigned(top.Y)))};
#else
    const auto region = tactical_object_bounds(tactical, bounds);
    TacticalLayerQuery query;
#endif
    const auto inside = [&](CoordStruct at) {
        return at.X >= region.left && at.X < region.right && at.Y >= region.top && at.Y < region.bottom;
    };
    const auto fog = [](const CoordStruct& at) { return debug_player_location_fogged(at); };
    const auto memory_fog = []() {
        return !debug_fog_hidden() && ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar;
    };
    const auto invoke = [&](ObjectClass& object, TacticalObjectPass pass, Point2D* point, RectangleStruct* clip) {
        const auto result = dispatch_tactical_object(ctx, object, pass, point, clip, forced);
        record_tactical_drawing(ctx, result);
        return drawing_completed(result);
    };
    for (int layer = 0; layer < 5; ++layer) {
#if defined(RA2_YRPP_GAME)
        auto& objects = MapClass::ObjectsInLayers[layer];
        for (int index = 0; index < objects.Count; ++index) {
            auto* object = objects[index];
            if (!object) continue;
#else
        query.begin(layer, region, true);
        int index = 0;
        ObjectClass* object = nullptr;
        while (query.next(object, index)) {
            // The spatial index mirrors the visible set; it only has to hear about an object whose entry changed.
            struct Visibility {
                ObjectClass* object;
                bool was_visible;
                ~Visibility() {
                    if (visible_objects().contains(object) != was_visible) tactical_spatial_visible(object);
                }
            } visibility{object, visible.contains(object)};
#endif
            set_visible(object, false, layer, index);
            Point2D point;
            CoordStruct ignored;
            if ((object->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None) {
                const auto at = object->Location;
                if (!inside(at)) continue;
                object->GetDestination(&ignored, nullptr);  // Original query even without memory fog.
                if (memory_fog() && fog(at)) continue;
                const auto center = object->GetCoords();
                if (tactical.CoordsToClient(&center, &point)) {
                    set_visible(object, true, layer, index);
                    if (!invoke(*object, TacticalObjectPass::behind, &point, &bounds) ||
                        !invoke(*object, TacticalObjectPass::body, nullptr, &bounds))
                        return;
                }
            } else if ((object->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None) {
                if (!inside(object->Location)) continue;
                set_visible(object, true, layer, index);
                const auto center = object->GetCoords();
                if (tactical.CoordsToClient(&center, &point)) {
                    if (!invoke(*object, TacticalObjectPass::behind, &point, &bounds) ||
                        !invoke(*object, TacticalObjectPass::extras, &point, &bounds))
                        return;
                }
                if (!invoke(*object, TacticalObjectPass::building_upper, nullptr, &bounds)) return;
            } else {
                const auto kind = object->WhatAmI();
                if (kind == AbstractType::Anim) {
                    auto& anim = static_cast<AnimClass&>(*object);
                    const auto at = anim.GetCoords();
                    if (!inside(at)) continue;
                    set_visible(&anim, true, layer, index);
                    if ((anim.IsBuildingAnim || !anim.Type->ShouldFogRemove || !fog(anim.GetCoords())) &&
                        !invoke(anim, TacticalObjectPass::body, nullptr, &bounds))
                        return;
                } else if (kind == AbstractType::Terrain) {
                    auto& terrain = static_cast<TerrainClass&>(*object);
                    if (!(terrain.Type->IsAnimated || terrain.IsCrumbling) || !inside(terrain.Location) ||
                        fog(terrain.Location))
                        continue;
                    set_visible(&terrain, true, layer, index);
                    if (!invoke(terrain, TacticalObjectPass::body, nullptr, &bounds)) return;
                } else if (inside(object->Location)) {
                    const auto center = object->GetCoords();
                    if (tactical.CoordsToClient(&center, &point)) {
                        set_visible(object, true, layer, index);
                        if (!invoke(*object, TacticalObjectPass::behind, &point, &bounds) ||
                            !invoke(*object, TacticalObjectPass::body, nullptr, &bounds))
                            return;
                    }
                }
            }
        }
        if (layer == int(Layer::Ground)) {
#if defined(RA2_YRPP_GAME)
            for (int n = 0; n < BuildingClass::Array.Count; ++n) {
                auto* building = BuildingClass::Array[n];
#else
            visible_buildings.clear();
            collect_visible_tactical_buildings(visible_buildings);
            for (auto* object : visible_buildings) {
                auto* building = static_cast<BuildingClass*>(object);
#endif
                if (!building->IsOnMap || !visible.contains(building)) continue;
                const auto at = building->GetRenderCoords();
                Point2D point;
                if (tactical.CoordsToClient(&at, &point)) {
                    auto clip = *runtime.drawing_bounds;
                    if (!invoke(*building, TacticalObjectPass::building_info, &point, &clip)) return;
                }
            }
        }
    }
    // YR performs this separate pass over all five layers. Buildings therefore
    // receive both their first-pass Extras and this call; do not deduplicate.
    for (int layer = 0; layer < 5; ++layer) {
        const int count = MapClass::ObjectsInLayers[layer].Count;
#if defined(RA2_YRPP_GAME)
        for (int index = 0; index < count; ++index) {
            auto* object = MapClass::ObjectsInLayers[layer][index];
#else
        query.begin(layer, region, true);
        const bool stable = tactical_spatial_unique_entries() && layer_revision == tactical_spatial_layer_revision() &&
            std::size_t(count) == offsets[layer + 1] - offsets[layer];
        int index = 0;
        ObjectClass* object = nullptr;
        while (query.next(object, index)) {
            if (index >= count) break;
#endif
#if defined(RA2_YRPP_GAME)
            const bool was_visible = visible.contains(object);
#else
            const bool was_visible = stable ? flags[offsets[layer] + index] != 0 : visible.contains(object);
#endif
            if (!object || !was_visible || (object->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None)
                continue;
            if ((object->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None) {
                CoordStruct ignored;
                object->GetDestination(&ignored, nullptr);
                if (runtime.has_window && runtime.has_window() && !(runtime.debug_map && *runtime.debug_map) &&
                    memory_fog() && fog(object->Location))
                    continue;
            }
            const auto center = object->GetCoords();
            Point2D point;
            if (tactical.CoordsToClient(&center, &point) && !invoke(*object, TacticalObjectPass::extras, &point, &bounds))
                return;
        }
    }
}
}
