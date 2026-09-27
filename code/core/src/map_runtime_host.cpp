#include "map_runtime.hpp"
#include <bit>
#include <cstdint>

namespace game {
const MapRuntimeServices& default_map_runtime() noexcept {
    // Fixed gamemd 7b8a0685: execute 6D1830, 6D18C0 and 6D1BB0 with the
    // original sqrt/sine tables. B0CD48 becomes 0x3fc25e5374344960, not the
    // result of the host libm sin(60 degrees)/sqrt(2) approximation.
    static constexpr double height_scale = std::bit_cast<double>(UINT64_C(0x3fc25e5374344960));
    static constexpr bool count_light_convert_references = true;
    static const MapRuntimeServices services{&height_scale, nullptr, &count_light_convert_references};
    return services;
}
}
