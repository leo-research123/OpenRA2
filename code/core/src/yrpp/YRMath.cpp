// The target uses quantized float lookup tables, not the host libm results.
// Table generation is mathematical reconstruction, not embedded EXE data.
#include "yrpp/YRMath.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

namespace {
float positive_chop(double value) {
    float result = static_cast<float>(value);
    if (double(result) > value)
        result = std::bit_cast<float>(std::bit_cast<std::uint32_t>(result) - 1);
    return result;
}
constexpr float atan_step = 0.024413354694843292f;
const std::array<float, 4097>& atan_table() {
    static const auto table = [] {
        std::array<float, 4097> result{};
        float value = 0;
        for (auto& entry : result) {
            entry = positive_chop(std::atan(double(value)));
            value = positive_chop(double(value) + atan_step);
        }
        return result;
    }();
    return table;
}
}

double YRPP_CDECL Math::atan2(double a, double b) {
    const float y = static_cast<float>(a), x = static_cast<float>(b);
    constexpr float half_pi = 1.5707963705062866f;
    if (x == 0) return y == 0 ? 0.0 : y > 0 ? double(half_pi) : -double(half_pi);
    const double scaled = std::abs(double(y) / double(x) / atan_step);
    double result = scaled >= 4097 ? double(half_pi) : double(atan_table()[static_cast<unsigned>(scaled)]);
    if (x < 0) result = Pi - result;
    return y < 0 ? -result : result;
}

double YRPP_CDECL Math::sqrt(double value) {
    if (value == 0) return 0;
    const auto bits = std::bit_cast<std::uint32_t>(positive_chop(std::abs(value)));
    const int exponent = int(bits >> 23) - 127;
    const auto index = ((bits & 0x7FFFFFu) | ((exponent & 1) ? 0x800000u : 0u)) >> 10;
    const double normalized = index < 8192 ? 1.0 + double(index)/8192.0
        : 2.0 + double(index-8192)/4096.0;
    const auto mantissa = static_cast<std::uint32_t>((std::sqrt(normalized)-1.0)*8388608.0);
    const auto result = (std::uint32_t((exponent >> 1) + 127) << 23) + mantissa;
    return std::bit_cast<float>(result);
}
