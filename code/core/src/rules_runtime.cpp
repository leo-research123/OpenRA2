#include "rules_runtime.hpp"
#include <stdexcept>

namespace game {
namespace {
thread_local const RulesRuntimeServices* active = nullptr;
}
const RulesRuntimeServices& rules_runtime() {
    const auto* services = active ? active : default_rules_runtime();
    if (!services) throw std::logic_error("RulesClass requires the host's existing game registries");
    return *services;
}
bool with_rules_runtime(const RulesRuntimeServices& services,
        void (*operation)(void*), void* context) {
    if (!operation) return false;
    struct Restore {
        const RulesRuntimeServices* previous;
        ~Restore() { active = previous; }
    } restore{active};
    active = &services;
    operation(context);
    return true;
}
bool rules_resolve_type(AbstractType type, const char* name, AbstractTypeClass*& result) {
    const auto& services = rules_runtime();
    return services.resolve && services.resolve(services.context, type, name, result);
}
bool rules_sound_index(const char* name, int& result) {
    const auto& services = rules_runtime();
    return services.sound_index && services.sound_index(services.context, name, result);
}
bool rules_type_count(AbstractType type, int& result) {
    const auto& services = rules_runtime();
    return services.type_count && services.type_count(services.context, type, result) && result >= 0;
}
bool rules_type_at(AbstractType type, int index, AbstractTypeClass*& result) {
    const auto& services = rules_runtime();
    return services.type_at && services.type_at(services.context, type, index, result) && result;
}
}
