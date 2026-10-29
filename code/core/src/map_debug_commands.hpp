#pragma once

#include "debug/debug_command.hpp"

class ObjectClass;
namespace game {
class MapWorld;

// Internal original-object operations; callers already own the map scope.
// They may throw recoverable exceptions, which their API entry must contain.
bool debug_map_ready(MapViewHandle&) noexcept;
void retire_map_object(ObjectClass&);
bool apply_debug_level(ObjectClass&, int level);
bool apply_debug_unit_health(MapWorld&, ObjectClass&, int health);
bool apply_debug_power(MapWorld&);
inline constexpr int debug_level_minimum = 0, debug_level_maximum = 2;
inline constexpr int debug_health_minimum = 0, debug_health_maximum = 1000000;

struct ParsedDebugCommand {
    const DebugConsoleCommand* command = nullptr;
    DebugCommandArguments arguments{};
    std::int32_t value = 0;
};
DebugCommandStatus parse_debug_command(const char*, std::uint32_t, ParsedDebugCommand&) noexcept;
}
