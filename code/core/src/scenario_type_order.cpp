// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 scenario.cpp Read_Scenario_INI; YR 0x68797A–0x6879E8.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_loading.hpp"
#include <initializer_list>
namespace game {
void read_scenario_type_definitions(const ScenarioInitializeServices &s,
                                    CCINIClass &map) {
  for (auto reader :
       {ScenarioObjectReader::teams, ScenarioObjectReader::scripts,
        ScenarioObjectReader::task_forces}) {
    s.read_objects(s.context, reader, s.ai_ini, true);
    s.read_objects(s.context, reader, &map, false);
  }
  s.read_objects(s.context, ScenarioObjectReader::trigger_types, &map, false);
  s.read_objects(s.context, ScenarioObjectReader::tags, &map, false);
  s.read_objects(s.context, ScenarioObjectReader::ai_triggers, s.ai_ini, true);
  s.read_objects(s.context, ScenarioObjectReader::ai_triggers, &map, false);
  s.step(s.context, ScenarioInitializationStep::finish_teams);
}
} // namespace game
