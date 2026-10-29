#include "scenario_runtime.hpp"
#include "yrpp/StringTable.h"
namespace game {
namespace {
// Detached scenarios have no trigger listener callback; session state remains
// in SessionClass::Instance independently of this operation scope.
void variable_changed(void*, bool, int) {}
bool stringtable(void*, const char* label, int line, const wchar_t*& result) {
    result = StringTable::LoadString(label, nullptr, "Scenario.CPP", line);
    return result != nullptr;
}
}
const ScenarioRuntimeServices& default_scenario_runtime() {
    static const ScenarioRuntimeServices services{nullptr, variable_changed, stringtable};
    return services;
}
}
