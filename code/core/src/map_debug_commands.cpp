#include "map_debug_commands.hpp"
#include "techno_spatial.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include "runtime_diagnostics.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/Unsorted.h"
#include "debug/debug_state.hpp"
#include <atomic>
#include <cstring>
#include <new>

namespace game {

const char* debug_command_status_name(DebugCommandStatus status) noexcept {
    switch (status) {
        case DebugCommandStatus::success:
            return "success";
        case DebugCommandStatus::partial_success:
            return "partial_success";
        case DebugCommandStatus::invalid_command:
            return "invalid_command";
        case DebugCommandStatus::invalid_argument:
            return "invalid_argument";
        case DebugCommandStatus::map_not_ready:
            return "map_not_ready";
        case DebugCommandStatus::stale_session:
            return "stale_session";
        case DebugCommandStatus::no_targets:
            return "no_targets";
        case DebugCommandStatus::resource_failure:
            return "resource_failure";
        case DebugCommandStatus::execution_failure:
            return "execution_failure";
    }
    return "execution_failure";
}

const char* debug_target_failure_name(DebugTargetFailure reason) noexcept {
    switch (reason) {
        case DebugTargetFailure::none:
            return "none";
        case DebugTargetFailure::unsupported_type:
            return "unsupported_type";
        case DebugTargetFailure::dead_object:
            return "dead_object";
        case DebugTargetFailure::expired_object:
            return "expired_object";
        case DebugTargetFailure::unavailable_player:
            return "unavailable_player";
        case DebugTargetFailure::exception:
            return "exception";
        case DebugTargetFailure::invalid_state:
            return "invalid_state";
        case DebugTargetFailure::blocked_destination:
            return "blocked_destination";
        case DebugTargetFailure::value_out_of_range:
            return "value_out_of_range";
    }
    return "exception";
}

bool debug_map_ready(MapViewHandle& view) noexcept {
    MapViewInfo info{};
    return !view.operation_depth && view.world && get_map_view_info(view, info) && info.state == MapViewState::ready;
}

bool get_debug_command_world(MapViewHandle& view, std::uint64_t& output) noexcept {
    output = 0;
    if (!debug_map_ready(view)) return false;
    output = view.world->impl->world_id;
    return output != 0;
}

void retire_map_object(ObjectClass& object) {
    const bool previous = Game::IsActive;
    struct RestoreActive {
        bool previous;
        ~RestoreActive() { Game::IsActive = previous; }
    } restore{previous};
    Game::IsActive = true;
    // Dispatch the original class-specific release before deferred deletion.
    object.UnInit();
    map_object_changed();
}

bool apply_debug_level(ObjectClass& object, int level) {
    if (object.IsDead()) return false;
    const auto kind = object.WhatAmI();
    if (kind != AbstractType::Building && kind != AbstractType::Infantry && kind != AbstractType::Unit &&
        kind != AbstractType::Aircraft)
        return false;
    static_cast<TechnoClass&>(object).Veterancy.Veterancy = static_cast<float>(level);
    object.NeedsRedraw = true;
    map_object_changed();
    return true;
}

bool apply_debug_unit_health(MapWorld& world, ObjectClass& object, int health) {
    if (object.IsDead()) return false;
    const auto kind = object.WhatAmI();
    if (kind != AbstractType::Building && kind != AbstractType::Terrain && kind != AbstractType::Infantry &&
        kind != AbstractType::Unit && kind != AbstractType::Aircraft)
        return false;
    if (health == 0) {
        if (kind == AbstractType::Building) {
            auto& building = static_cast<BuildingClass&>(object);
            // Retire through original production/ownership and deferred-delete
            // hooks, with no combat destruction or experience award.
            const bool previous = Game::IsActive;
            struct Restore {
                bool previous;
                ~Restore() { Game::IsActive = previous; }
            } restore{previous};
            Game::IsActive = true;
            building.Deselect();
            building.IsBeingRepaired = false;
            building.Health = building.EstimatedHealth = 0;
            techno_spatial_dirty(&building);
            // This host removal skips the next BuildingClass::Update tick.
            building.UpdatePowered();
            building.ClearBunker();
            while (building.Occupants.Count) {
                auto* occupant = building.Occupants[building.Occupants.Count - 1];
                building.Occupants.RemoveItem(building.Occupants.Count - 1);
                occupant->UnInit();
            }
            for (int i = 0; i < 21; ++i)
                building.DestroyNthAnim(static_cast<BuildingAnimSlot>(i));
            building.UnInit();
            map_object_changed();
            refresh_map_world_hover(world);
            return true;
        }
        object.Deselect();
        object.Health = object.EstimatedHealth = 0;
        techno_spatial_dirty(&object);
        retire_map_object(object);
        refresh_map_world_hover(world);
    } else {
        object.Health = object.EstimatedHealth = health;
        techno_spatial_dirty(&object);
        if (kind == AbstractType::Building) {
            auto& building = static_cast<BuildingClass&>(object);
            building.ToggleDamagedAnims(!building.IsGreenHP());
            building.NeedsRepairs = health < building.Type->Strength;
            if (!building.NeedsRepairs) building.IsBeingRepaired = false;
            if (building.Owner) building.Owner->RecheckPower = true;
        }
    }
    object.NeedsRedraw = true;
    map_object_changed();
    return true;
}

bool apply_debug_power(MapWorld& world) {
    auto* player = HouseClass::CurrentPlayer;
    if (!player) return false;
    world.impl->player_power_bonus = 1000000;
    player->RecheckPower = true;
    player->UpdatePower();
    return true;
}

void DebugCommandContext::record_failure(MapObjectId target, DebugTargetFailure reason) noexcept {
    if (result.first_failure == DebugTargetFailure::none) {
        result.first_failure = reason;
        result.first_failed_target = target;
        runtime_log_event("command_target_failure", "request=%llu world=%llu target=%u reason=%s",
                          static_cast<unsigned long long>(result.request),
                          static_cast<unsigned long long>(result.world), target.value,
                          debug_target_failure_name(reason));
    }
}

bool debug_current_player_available() noexcept { return HouseClass::CurrentPlayer != nullptr; }

struct DebugExecution {
    DebugCommandContext context;
    const ParsedDebugCommand& parsed;
    DebugCommandStatus status = DebugCommandStatus::execution_failure;
};

static void run_debug_command(void* pointer) {
    auto& execution = *static_cast<DebugExecution*>(pointer);
    auto& context = execution.context;
    auto& result = context.result;
    result.frame = Unsorted::CurrentFrame;
    auto& feedback = context.world.impl->view.debug.feedback;
    feedback = {};
    feedback.request = result.request;
    execution.parsed.arguments.get_real(0, feedback.number);
    try {
        execution.status = execution.parsed.command->execute(context, execution.parsed.arguments);
    } catch (const std::bad_alloc&) {
        result.skipped = result.candidates - result.applied;
        execution.status = result.applied           ? DebugCommandStatus::partial_success
                           : context.writes_started ? DebugCommandStatus::execution_failure
                                                    : DebugCommandStatus::resource_failure;
        context.record_failure({}, DebugTargetFailure::exception);
    } catch (...) {
        result.skipped = result.candidates - result.applied;
        execution.status = result.applied ? DebugCommandStatus::partial_success : DebugCommandStatus::execution_failure;
        context.record_failure({}, DebugTargetFailure::exception);
    }
}

DebugCommandStatus execute_debug_command(MapViewHandle* view, std::uint64_t expected_world, const char* text,
                                         std::uint32_t length, DebugCommandResult& output) noexcept {
    static std::atomic<std::uint64_t> next_request{0};
    output = {};
    output.request = ++next_request;
    ParsedDebugCommand parsed;
    auto status = parse_debug_command(text, length, parsed);
    output.command = parsed.command ? parsed.command->descriptor().id : DebugCommandId::none;
    output.value = parsed.value;
    if (view && view->world) output.world = view->world->impl->world_id;
    // Keep rejected arguments diagnosable without allowing controls or quotes
    // to turn an attempted command into additional log records.
    char request_text[debug_command_max_bytes + 1]{};
    for (std::uint32_t i = 0; text && i < length && i < debug_command_max_bytes; ++i) {
        const auto byte = static_cast<unsigned char>(text[i]);
        request_text[i] = byte >= 32 && byte <= 126 && byte != '"' && byte != '\\' ? char(byte) : '_';
    }
    runtime_log_event("command_begin",
                      "request=%llu world=%llu expected_world=%llu command=%s value=%d bytes=%u input=\"%s\"",
                      static_cast<unsigned long long>(output.request), static_cast<unsigned long long>(output.world),
                      static_cast<unsigned long long>(expected_world),
                      parsed.command ? parsed.command->descriptor().name : "", parsed.value, length, request_text);
    if (status == DebugCommandStatus::success) {
        if (!view || !debug_map_ready(*view))
            status = DebugCommandStatus::map_not_ready;
        else if (!expected_world || expected_world != output.world)
            status = DebugCommandStatus::stale_session;
        else {
            DebugExecution execution{{*view->world, output}, parsed};
            const bool completed = with_map_view(*view, run_debug_command, &execution);
            status = completed        ? execution.status
                     : output.applied ? DebugCommandStatus::partial_success
                                      : DebugCommandStatus::execution_failure;
            if (!completed) {
                output.skipped = output.candidates - output.applied;
                execution.context.record_failure({}, DebugTargetFailure::exception);
            }
        }
    }
    runtime_log_event(
        "command_end",
        "request=%llu world=%llu execution_frame=%d status=%s candidates=%u applied=%u skipped=%u failure=%s target=%u",
        static_cast<unsigned long long>(output.request), static_cast<unsigned long long>(output.world), output.frame,
        debug_command_status_name(status), output.candidates, output.applied, output.skipped,
        debug_target_failure_name(output.first_failure), output.first_failed_target.value);
    return status;
}

bool set_map_object_level(MapViewHandle& view, MapObjectId id, int level) noexcept {
    if (!debug_map_ready(view) || (level < debug_level_minimum || level > debug_level_maximum)) return false;
    struct Command {
        MapWorld& world;
        MapObjectId id;
        int value;
        bool applied = false;
    } command{*view.world, id, level};
    const bool completed = with_map_view(
        view,
        [](void* pointer) {
            auto& command = *static_cast<Command*>(pointer);
            if (auto* object = resolve_map_object(command.world, command.id))
                command.applied = apply_debug_level(*object, command.value);
        },
        &command);
    runtime_log_event("debug_level", "world=%llu id=%u level=%d applied=%d", static_cast<unsigned long long>(id.world),
                      id.value, level, completed && command.applied);
    return completed && command.applied;
}

bool set_map_unit_cheat_health(MapViewHandle& view, MapObjectId id, int health) noexcept {
    if (!debug_map_ready(view) || (health < debug_health_minimum || health > debug_health_maximum)) return false;
    struct Command {
        MapWorld& world;
        MapObjectId id;
        int value;
        bool applied = false;
    } command{*view.world, id, health};
    const bool completed = with_map_view(
        view,
        [](void* pointer) {
            auto& command = *static_cast<Command*>(pointer);
            if (auto* object = resolve_map_object(command.world, command.id))
                command.applied = apply_debug_unit_health(command.world, *object, command.value);
        },
        &command);
    runtime_log_event("debug_unit_health", "world=%llu id=%u hp=%d applied=%d",
                      static_cast<unsigned long long>(id.world), id.value, health, completed && command.applied);
    return completed && command.applied;
}

bool grant_map_player_power(MapViewHandle& view) noexcept {
    if (!debug_map_ready(view)) return false;
    struct Command {
        MapWorld& world;
        bool applied = false;
    } command{*view.world};
    const bool completed = with_map_view(
        view,
        [](void* pointer) {
            auto& command = *static_cast<Command*>(pointer);
            command.applied = apply_debug_power(command.world);
        },
        &command);
    runtime_log_event("debug_power", "applied=%d", completed && command.applied);
    return completed && command.applied;
}

}
