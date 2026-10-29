// Godot command metadata and results are materialized only at this host boundary.
#include "bridge/ra2_core.hpp"
#include "api/debug_commands.hpp"
#include "core_call_scope.hpp"
#include <new>

godot::Array RA2Core::get_debug_commands() const {
    godot::Array commands;
    for (std::uint32_t index = 0; index < game::debug_command_count(); ++index) {
        game::DebugCommandDescriptor descriptor;
        if (!game::get_debug_command_descriptor(index, descriptor)) continue;
        godot::Dictionary item;
        item["id"] = static_cast<std::int64_t>(descriptor.id);
        item["name"] = godot::String::utf8(descriptor.name);
        item["usage"] = godot::String::utf8(descriptor.usage);
        item["description"] = godot::String::utf8(descriptor.description);
        item["success_message"] = godot::String::utf8(descriptor.success_message);
        item["argument"] = static_cast<std::int64_t>(descriptor.argument);
        item["minimum"] = descriptor.minimum;
        item["maximum"] = descriptor.maximum;
        commands.append(item);
    }
    return commands;
}

std::int64_t RA2Core::get_debug_command_world() const {
    game::CoreCallScope floating_point;
    try {
        const std::lock_guard lock(progress_mutex_);
        std::uint64_t world = 0;
        return map_state_ == MapState::ready && map_ && game::get_debug_command_world(*map_, world)
                   ? static_cast<std::int64_t>(world)
                   : 0;
    } catch (...) {
        return 0;
    }
}

godot::Dictionary RA2Core::get_debug_performance(std::int64_t expected_world) const {
    game::CoreCallScope floating_point;
    godot::Dictionary result;
    result["visible"] = false;
    try {
        const std::lock_guard lock(progress_mutex_);
        game::DebugPerformanceSnapshot snapshot;
        if (map_state_ == MapState::ready && map_ && expected_world > 0 &&
            game::get_debug_performance(*map_, static_cast<std::uint64_t>(expected_world), snapshot)) {
            result["visible"] = snapshot.visible;
            result["message"] = godot::String::utf8(snapshot.message);
        }
    } catch (...) {
        result["visible"] = false;
    }
    return result;
}

godot::Dictionary RA2Core::execute_debug_command(const godot::String& text, std::int64_t expected_world) {
    game::CoreCallScope floating_point;
    game::DebugCommandResult result;
    auto status = game::DebugCommandStatus::map_not_ready;
    game::DebugCommandFeedback feedback;
    try {
        // One lock covers parse, target capture and all original-object writes.
        const std::lock_guard lock(progress_mutex_);
        const auto utf8 = text.utf8();
        status = game::execute_debug_command(map_state_ == MapState::ready ? map_ : nullptr,
                                             expected_world > 0 ? static_cast<std::uint64_t>(expected_world) : 0,
                                             utf8.get_data(), static_cast<std::uint32_t>(utf8.length()), result);
        if (map_) game::get_debug_command_feedback(*map_, result.request, feedback);
    } catch (const std::bad_alloc&) {
        status = game::DebugCommandStatus::resource_failure;
    } catch (...) {
        status = game::DebugCommandStatus::execution_failure;
    }
    godot::Dictionary output;
    output["status"] = game::debug_command_status_name(status);
    output["request"] = static_cast<std::int64_t>(result.request);
    output["world"] = static_cast<std::int64_t>(result.world);
    output["command"] = static_cast<std::int64_t>(result.command);
    output["frame"] = result.frame;
    output["value"] = result.value;
    output["number"] = feedback.number;
    output["message"] = godot::String::utf8(feedback.message);
    output["candidates"] = static_cast<std::int64_t>(result.candidates);
    output["applied"] = static_cast<std::int64_t>(result.applied);
    output["skipped"] = static_cast<std::int64_t>(result.skipped);
    output["first_failure"] = game::debug_target_failure_name(result.first_failure);
    output["first_failed_world"] = static_cast<std::int64_t>(result.first_failed_target.world);
    output["first_failed_id"] = static_cast<std::int64_t>(result.first_failed_target.value);
    return output;
}

void RA2Core::record_host_performance(std::uint64_t world, const game::DebugHostPerformance& sample) noexcept {
    try {
        const std::lock_guard lock(progress_mutex_);
        if (map_state_ == MapState::ready && map_) game::record_debug_host_performance(*map_, world, sample);
    } catch (...) {
    }
}
