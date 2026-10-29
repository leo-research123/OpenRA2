#pragma once
#include <bit>
#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>

namespace game {
// Keep the multiply/divide on x87 until the original _ftol-style int64 store.
// This preserves the incoming precision/rounding for evaluation and leaves
// 0x0E7F active afterwards, without changing MXCSR on x86 hosts.
inline int x87_scaled_quotient_to_integer(double value, double multiplier, int divisor) noexcept {
    const double denominator = divisor;
    const unsigned short control = 0x0E7F;
    std::uint64_t result;
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    __asm fld value
    __asm fmul multiplier
    __asm fdiv denominator
    __asm fldcw control
    __asm fistp result
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("fldl %1; fmull %2; fdivl %3; fldcw %4; fistpq %0"
                     : "=m"(result)
                     : "m"(value), "m"(multiplier), "m"(denominator), "m"(control)
                     : "st");
#else
    const double quotient = value * multiplier / denominator;
    std::fesetround(FE_TOWARDZERO);
    if (!(quotient >= -0x1p63 && quotient < 0x1p63)) return 0;
    result = static_cast<std::uint64_t>(static_cast<std::int64_t>(quotient));
#endif
        return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(result));
}
// Preserve an x87 multiply followed by a float store without changing MXCSR.
inline float x87_multiply_to_float(double a, double b) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    float result;
    __asm fld a
    __asm fmul b
    __asm fstp result
    return result;
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    float result;
    __asm__ volatile("fldl %1; fmull %2; fstps %0" : "=m"(result) : "m"(a), "m"(b) : "st");
    return result;
#else
    return float(a * b);
#endif
}
// A double stored to a float while the 0x0E7F control word is active (fstp dword uses its rounding
// control) is truncated toward zero. static_cast<float> rounds to nearest and can differ by one ulp.
inline float x87_double_to_float(double value) noexcept {
    const float nearest = static_cast<float>(value);
    if (!std::isfinite(value)) return nearest;
    if (!std::isfinite(nearest)) return std::copysign(std::numeric_limits<float>::max(), nearest);
    // |nearest| exceeds |value|, so it is nonzero: one ulp toward zero is one step in the bit pattern.
    return std::fabs(static_cast<double>(nearest)) > std::fabs(value)
               ? std::bit_cast<float>(std::bit_cast<std::uint32_t>(nearest) - 1)
               : nearest;
}
// YR 0x007C5F00 leaves the 0x0E7F rounding mode active, stores an int64,
// and these callers consume EAX. Invalid conversions have low word zero.
// Evaluate the argument before changing the caller's floating-point mode.
inline int x87_integer(double value) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    // The EXE writes the x87 control word only; MXCSR remains unchanged.
    const unsigned short control=0x0E7F;
    __asm fldcw control
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    const unsigned short control = 0x0E7F;
    __asm__ volatile("fldcw %0" : : "m"(control));
#else
    std::fesetround(FE_TOWARDZERO);
#endif
        if (!(value >= -0x1p63 && value < 0x1p63)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// YR 0x007C5F00 leaves the x87 control word at 0x0E7F: 53-bit mantissa, round toward zero.
// Every multiply that reaches the helper after its first use is therefore truncated, not
// rounded to nearest. This returns that product (Dekker two-product, no libm call).
inline double x87_truncated_product(double a, double b) noexcept {
    const double product = a * b;
    if (!(product > -1e290 && product < 1e290) || product == 0.0) return product;
    const auto split = [](double value, double& high, double& low) noexcept {
        const double scaled = 134217729.0 * value;
        high = scaled - (scaled - value);
        low = value - high;
    };
    double ah, al, bh, bl;
    split(a, ah, al);
    split(b, bh, bl);
    const double error = ((ah * bh - product) + ah * bl + al * bh) + al * bl;
    if (error == 0.0 || (error < 0.0) != (product > 0.0)) return product;
    return std::bit_cast<double>(std::bit_cast<std::uint64_t>(product) - 1);
}
// Division under the beam caller's 0x0E7F control word. FMA recovers the
// quotient error on hosts without x87, so a nearest overshoot is chopped.
inline double x87_truncated_quotient(double a, double b) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    // clang-format off
    double result;
    __asm fld a
    __asm fdiv b
    __asm fstp result
    return result;
// clang-format on
#elif defined(__i386__) || defined(__x86_64__)
    double result;
    __asm__ volatile("fldl %1; fdivl %2; fstpl %0" : "=m"(result) : "m"(a), "m"(b) : "st");
    return result;
#else
    const double quotient = a / b;
    if (!std::isfinite(quotient) || quotient == 0.0) return quotient;
    const double error = std::fma(-quotient, b, a);
    if (error == 0.0 || (error < 0.0) != (a > 0.0)) return quotient;
    return std::bit_cast<double>(std::bit_cast<std::uint64_t>(quotient) - 1);
#endif
}
// Sum under the same 0x0E7F control word. The host must use round-to-nearest.
// Knuth two-sum gives the exact error; a nearest result that overshoots is stepped toward zero.
inline double x87_truncated_sum(double a, double b) noexcept {
    const double sum = a + b;
    if (!(sum > -1e290 && sum < 1e290)) return sum;
    const double virtual_b = sum - a;
    const double error = (a - (sum - virtual_b)) + (b - virtual_b);
    if (error == 0.0 || (error > 0.0) == (sum > 0.0)) return sum;
    return std::bit_cast<double>(std::bit_cast<std::uint64_t>(sum) - 1);
}
inline double x87_truncated_difference(double a, double b) noexcept { return x87_truncated_sum(a, -b); }
}
