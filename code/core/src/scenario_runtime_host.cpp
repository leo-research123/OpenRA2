#include "scenario_runtime.hpp"
#include "yrpp/StringTable.h"
namespace game {
namespace {
// A detached standalone Scenario has no active session or trigger listeners.
// Hosts with a world must bind the real session and notification dependencies.
int session_mode(void*) noexcept { return -1; }
void variable_changed(void*, bool, int) {}
bool stringtable(void*, const char* label, int line, const wchar_t*& result) {
    result = StringTable::LoadString(label, nullptr, "Scenario.CPP", line);
    return result != nullptr;
}
}
const ScenarioRuntimeServices& default_scenario_runtime() {
    static const ScenarioRuntimeServices services{nullptr, session_mode, variable_changed, stringtable};
    return services;
}
}
