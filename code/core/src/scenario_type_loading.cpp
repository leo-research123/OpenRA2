// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 scenario.cpp Read_Scenario_INI; YR 0x68797A–0x6879E8.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "scenario_loading.hpp"
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
namespace game {
void load_scenario_type_definitions(CCINIClass &ai, CCINIClass &map) {
  ScenarioInitializeServices s{};
  s.ai_ini = &ai;
  s.read_objects = [](void *, ScenarioObjectReader reader, CCINIClass *ini,
                      bool global) {
    switch (reader) {
    case ScenarioObjectReader::teams:
      TeamTypeClass::LoadFromINIList(ini, global);
      break;
    case ScenarioObjectReader::scripts:
      ScriptTypeClass::LoadFromINIList(ini, global);
      break;
    case ScenarioObjectReader::task_forces:
      TaskForceClass::LoadFromINIList(ini, global);
      break;
    case ScenarioObjectReader::trigger_types:
      TriggerTypeClass::LoadFromINIList(ini);
      break;
    case ScenarioObjectReader::tags:
      TagTypeClass::LoadFromINIList(ini);
      break;
    case ScenarioObjectReader::ai_triggers:
      AITriggerTypeClass::LoadFromINIList(ini, global);
      break;
    default:
      break;
    }
  };
  s.step = [](void *, ScenarioInitializationStep) {
    TeamTypeClass::ProcessAllTaskforces();
  };
  read_scenario_type_definitions(s, map);
}
} // namespace game
