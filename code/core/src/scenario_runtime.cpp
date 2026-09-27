#include "scenario_runtime.hpp"
#include <stdexcept>
namespace game {
namespace { thread_local const ScenarioRuntimeServices* active = nullptr; }
const ScenarioRuntimeServices& scenario_runtime() {
    return active ? *active : default_scenario_runtime();
}
const wchar_t* scenario_text(const char* label, int source_line) {
    const auto& runtime = scenario_runtime();
    const wchar_t* text = nullptr;
    if (!runtime.stringtable(runtime.context, label, source_line, text) || !text)
        throw std::runtime_error("Scenario string-table lookup failed");
    return text;
}
bool with_scenario_runtime(const ScenarioRuntimeServices& services,
        void (*operation)(void*), void* context) {
    if (!services.session_mode || !services.variable_changed || !services.stringtable || !operation)
        return false;
    struct Restore {
        const ScenarioRuntimeServices* previous;
        ~Restore() { active = previous; }
    } restore{active};
    active = &services;
    operation(context);
    return true;
}
}
