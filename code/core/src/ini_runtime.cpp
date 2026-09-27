#include "ini_runtime.hpp"
#include <stdexcept>

namespace game {
namespace {
thread_local const IniRuntimeServices* active = nullptr;
}
const IniRuntimeServices& ini_runtime() {
    const auto* services = active ? active : default_ini_runtime();
    if (!services)
        throw std::logic_error("INI game-type lookup requires the host's original game registries (with_ini_runtime)");
    return *services;
}
bool with_ini_runtime(const IniRuntimeServices& services, void (*operation)(void*),
        void* context, std::string& error) {
    error.clear();
    if (!services.name || !services.find || !services.stringtable || !operation) {
        error = "INI runtime requires name, find, stringtable and operation callbacks";
        return false;
    }
    struct Restore {
        const IniRuntimeServices* previous;
        ~Restore() { active = previous; }
    } restore{active};
    active = &services;
    try { operation(context); return true; }
    catch (const std::exception& exception) { error = exception.what(); }
    catch (...) { error = "INI runtime operation failed"; }
    return false;
}
}
