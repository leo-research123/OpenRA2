#pragma once
#include "api/scenario_runtime.hpp"
namespace game {
const ScenarioRuntimeServices& scenario_runtime();
const ScenarioRuntimeServices& default_scenario_runtime();
const wchar_t* scenario_text(const char* label, int source_line);
}
