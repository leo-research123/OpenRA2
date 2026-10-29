#include "debug/debug_command.hpp"
#include "map_world_internal.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/UnitClass.h"
#include <vector>

namespace game {
struct DebugTarget {
    MapObjectId id;
    ObjectClass* object;
};

template <class Array>
static void collect_debug_targets(MapWorld& world, const Array& array, int baseline,
                                  std::vector<DebugTarget>& targets) {
    for (int index = baseline; index < array.Count; ++index) {
        auto* object = array[index];
        if (object->IsSelected) targets.push_back({object_id(world, object), object});
    }
}

DebugTargetFailure debug_live_unit_filter(ObjectClass& object) {
    const auto kind = object.WhatAmI();
    if (kind != AbstractType::Infantry && kind != AbstractType::Unit && kind != AbstractType::Aircraft)
        return DebugTargetFailure::unsupported_type;
    return object.IsDead() ? DebugTargetFailure::dead_object : DebugTargetFailure::none;
}

DebugTargetFailure debug_live_techno_filter(ObjectClass& object) {
    const auto kind = object.WhatAmI();
    if (kind != AbstractType::Building && kind != AbstractType::Infantry && kind != AbstractType::Unit &&
        kind != AbstractType::Aircraft)
        return DebugTargetFailure::unsupported_type;
    return object.IsDead() ? DebugTargetFailure::dead_object : DebugTargetFailure::none;
}

DebugTargetFailure debug_live_health_filter(ObjectClass& object) {
    if (object.WhatAmI() == AbstractType::Terrain)
        return object.IsDead() ? DebugTargetFailure::dead_object : DebugTargetFailure::none;
    return debug_live_techno_filter(object);
}

DebugCommandStatus execute_selected_debug_objects(DebugCommandContext& context, const DebugCommandArguments& arguments,
                                                  DebugTargetFilter filter, DebugTargetApply apply,
                                                  DebugTargetLifetime lifetime) {
    auto& world = context.world;
    auto& result = context.result;
    // Capture every selected original object in legacy order before any writes.
    // IsSelected remains authoritative even if the original selected list failed to grow.
    std::vector<DebugTarget> targets;
    const auto& state = *world.impl;
    collect_debug_targets(world, BuildingClass::Array, state.buildings, targets);
    collect_debug_targets(world, TerrainClass::Array, state.terrains, targets);
    collect_debug_targets(world, InfantryClass::Array, state.infantry, targets);
    collect_debug_targets(world, UnitClass::Array, state.units, targets);
    collect_debug_targets(world, AircraftClass::Array, state.aircraft, targets);
    result.candidates = static_cast<std::uint32_t>(targets.size());
    for (const auto& target : targets) {
        auto* object =
            lifetime == DebugTargetLifetime::resolve_each_target ? resolve_map_object(world, target.id) : target.object;
        if (!object) {
            context.record_failure(target.id, DebugTargetFailure::expired_object);
            continue;
        }
        const auto failure = filter(*object);
        if (failure != DebugTargetFailure::none) {
            context.record_failure(target.id, failure);
            continue;
        }
        try {
            context.writes_started = true;
            const auto applied = apply(context, *object, arguments);
            if (applied == DebugTargetFailure::none)
                ++result.applied;
            else
                context.record_failure(target.id, applied);
        } catch (...) {
            context.record_failure(target.id, DebugTargetFailure::exception);
            throw;
        }
    }
    result.skipped = result.candidates - result.applied;
    return !result.applied  ? DebugCommandStatus::no_targets
           : result.skipped ? DebugCommandStatus::partial_success
                            : DebugCommandStatus::success;
}
}
