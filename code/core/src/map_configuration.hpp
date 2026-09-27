#pragma once
#include <cstddef>
class CCINIClass;
namespace game {
class MapWorld;
// Internal native loading contract; all exceptions become a diagnostic/false.
// Type registries must already exist. Retained map/rules/art stay world-owned.
bool parse_map_rule_fields(MapWorld&, CCINIClass&, char* error, std::size_t capacity) noexcept;
bool parse_map_scenario_fields(MapWorld&, char* error, std::size_t capacity) noexcept;
}
