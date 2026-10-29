#include "rules_runtime.hpp"
#include <cfenv>

namespace game {
void rules_integer_rounding() noexcept {
    if (!rules_integer_rounding_is_known_active()) std::fesetround(FE_TOWARDZERO);
}
}
