#pragma once
#include <bit>
#include <cfenv>
#include <cstdint>

namespace game {
// YR 0x007C5F00 leaves the 0x0E7F rounding mode active, stores an int64,
// and these callers consume EAX. Invalid conversions have low word zero.
// Evaluate the argument before changing the caller's floating-point mode.
inline int x87_integer(double value) noexcept {
    std::fesetround(FE_TOWARDZERO);
    if(!(value>=-0x1p63&&value<0x1p63))return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
