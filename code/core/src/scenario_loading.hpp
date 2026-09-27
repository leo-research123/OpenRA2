#pragma once
#include "api/scenario_runtime.hpp"
class CCINIClass;
class TeamTypeClass;
namespace game {
// Native admission of original map-authored campaign instances/reinforcements.
void load_scenario_tag_instances();
bool reinforce_team(TeamTypeClass& type, int waypoint) noexcept;
// Internal shared phase of ScenarioClass::InitializeWorldINI
// (0x68797A–0x6879E8). Callbacks retain the initializer's exception contract;
// the native host catches at load_scenario_objects. Neither function is an
// original-class entry point.
void read_scenario_type_definitions(const ScenarioInitializeServices &,
                                    CCINIClass &map);
void load_scenario_type_definitions(CCINIClass &ai, CCINIClass &map);
bool load_scenario_objects(CCINIClass &map, CCINIClass &rules, CCINIClass &art,
                           unsigned int &rejectedRecords) noexcept;
} // namespace game
