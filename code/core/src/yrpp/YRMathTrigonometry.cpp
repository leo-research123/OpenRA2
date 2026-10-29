// Original 0x004CACB0/0x004CAD00. Reconstruct the original quantized sine
// table mathematically; no binary asset is embedded in the native core.
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cfenv>
namespace {
float chop(double value) {
    float result = float(value);
    if (std::abs(double(result)) > std::abs(value))
        result = std::bit_cast<float>(std::bit_cast<std::uint32_t>(result) - 1);
    return result;
}
const std::array<float, 10240>& sine_table() {
    static const auto table = []() {
        std::array<float, 10240> result{};
        // Rule integer parsing can leave FE_TOWARDZERO active. Host libm then
        // produces sin(pi/2) just below 1, unlike the target x87 table initializer.
        // Build deterministically, apply the target float chop explicitly, and
        // restore the caller's rounding contract before publishing the table.
        const int previous = std::fegetround();
        struct Restore {
            int mode;
            ~Restore() {
                if (mode != -1) std::fesetround(mode);
            }
        } restore{previous};
        std::fesetround(FE_TONEAREST);
        for (unsigned i = 0; i < result.size(); ++i)
            result[i] = chop(std::sin(i * (Math::TwoPi / 8192.0)));
        // x87 FSIN's range reduction leaves these residuals at pi and 2*pi.
        result[4096] = 1.2246063538223773e-16f;
        result[8192] = -2.4492127076447545e-16f;
        return result;
    }();
    return table;
}
int scaled(double value) { return game::x87_integer(value * 2607.594482421875); }
}
double YRPP_CDECL Math::sin(double value) {
    const int raw = scaled(value);
    int index = (raw / 2) % 8192;
    if (index < 0) index += 8192;
    if ((raw & 1) && index < 8191) ++index;
    return sine_table()[index];
}
double YRPP_CDECL Math::cos(double value) {
    const int raw = scaled(value);
    const int rem = (raw / 2) % 8192;
    int index = rem + (rem < 0 ? 10240 : 2048);
    if ((raw & 1) && index < 10239) ++index;
    return sine_table()[index];
}

double YRPP_CDECL Math::asin(double value) {
    const int index = game::x87_integer((value + 1.0) * 2048.0);
    if (index < 0 || index > 4096) return 0;
    return chop(std::asin(-1.0 + index / 2048.0));
}

// 0x004CADB0 uses the same quantized inverse-sine table as 0x004CAD80.
double YRPP_CDECL Math::acos(double value) { return Math::HalfPi - Math::asin(value); }

#if !defined(RA2_YRPP_GAME)
// 0x004CAD50 indexes the original 4096-entry tangent table.
double YRPP_CDECL Math::tan(double value) {
    static const auto table = []() {
        std::array<float, 4096> result{};
        const int previous = std::fegetround();
        struct Restore {
            int mode;
            ~Restore() {
                if (mode != -1) std::fesetround(mode);
            }
        } restore{previous};
        std::fesetround(FE_TONEAREST);
        for (unsigned i = 0; i < result.size(); ++i)
            result[i] = chop(std::tan(i * (Math::TwoPi / 4096.0)));
        // FPTAN's x87 range reduction at the two poles and pi differs from libm.
        result[1024] = 1.6331778499280896e16f;
        result[2048] = -1.2246063538223773e-16f;
        result[3072] = 5443925808513024.0f;
        return result;
    }();
    int index = game::x87_integer(value * 651.8986469044033) % 4096;
    if (index < 0) index += 4096;
    return table[index];
}

#endif
