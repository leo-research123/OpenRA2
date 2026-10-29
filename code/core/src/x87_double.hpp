#pragma once
#include <cmath>

namespace game {
// Double arithmetic as the original executes it under x87 control word 0x0E7F (53-bit mantissa, rounding
// toward zero; YR 0x007C5F00 leaves that mode active). A host that rounds to nearest produces the same
// value once a result that was rounded away from zero is moved back by one ulp; the correction is
// derived from the exact rounding error and does nothing when the host already truncates.
inline double x87_toward_zero(double nearest, double error) noexcept {
    return (nearest > 0 && error < 0) || (nearest < 0 && error > 0) ? std::nextafter(nearest, 0.0) : nearest;
}
inline double x87_add(double a, double b) noexcept {
    const double sum = a + b, virtual_b = sum - a;
    return x87_toward_zero(sum, (a - (sum - virtual_b)) + (b - virtual_b));
}
inline double x87_multiply(double a, double b) noexcept {
    const double product = a * b;
    return x87_toward_zero(product, std::fma(a, b, -product));
}
inline double x87_divide(double a, double b) noexcept {
    const double quotient = a / b;
    if (!std::isfinite(quotient)) return quotient;
    const double remainder = std::fma(-quotient, b, a);
    return x87_toward_zero(quotient, b < 0 ? -remainder : remainder);
}
}
