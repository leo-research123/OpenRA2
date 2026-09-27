#include "map_runtime.hpp"
#include <cmath>

namespace game {
namespace { thread_local const MapRuntimeServices* active = nullptr; }
const MapRuntimeServices& map_runtime() noexcept {
    return active ? *active : default_map_runtime();
}
bool with_map_runtime(const MapRuntimeServices& services,
        void (*operation)(void*), void* argument) noexcept {
    if (!operation || !services.height_scale || !std::isfinite(*services.height_scale) ||
        *services.height_scale < 0.0 || *services.height_scale > 0.5) return false;
    struct Restore {
        const MapRuntimeServices* previous;
        ~Restore() { active = previous; }
    } restore{active};
    active = &services;
    try { operation(argument); return true; }
    catch (...) { return false; }
}
bool map_view_bounds(RectangleStruct& output) noexcept {
    const auto* bounds = map_runtime().view_bounds;
    if (!bounds) return false;
    output = *bounds;
    return true;
}
}
