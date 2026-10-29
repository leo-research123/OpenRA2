// Original eruption geometry. Native callee failures do not cross the boundary.
#include "yrpp/RadBeam.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
namespace {
int integer(DWORD v) { return std::bit_cast<int>(v); }
int add(int a, int b) { return integer(unsigned(a) + unsigned(b)); }
int multiply(int a, int b) { return integer(unsigned(a) * unsigned(b)); }
double length(DWORD x, DWORD y, DWORD z) { return Math::sqrt(double(integer(x * x + y * y + z * z))); }
CoordStruct divide(DWORD x, DWORD y, DWORD z, int divisor) {
    CoordStruct result{integer(x), integer(y), integer(z)};
    if (divisor) {
        result.X /= divisor;
        result.Y /= divisor;
        result.Z /= divisor;
    }
    return result;
}
}
void RadBeam::InitializeEruption() try {
    unknown_5C = unsigned(TargetLocation.X) - unsigned(SourceLocation.X);
    unknown_60 = unsigned(TargetLocation.Y) - unsigned(SourceLocation.Y);
    unknown_64 = unsigned(TargetLocation.Z) - unsigned(SourceLocation.Z);
    unknown_68 = length(unknown_5C, unknown_60, unknown_64);
    unknown_8C = unsigned(game::x87_integer(game::x87_truncated_quotient(unknown_68, unknown_18)));
    AnotherLocation = divide(unknown_5C, unknown_60, unknown_64, integer(unknown_8C));
} catch (...) {
    std::abort();
}
void RadBeam::AdvanceEruption() try {
    const int age = integer(unknown_7C), period = integer(Period), twice = multiply(age, 2);
    const int envelope = age < period / 2 ? twice : add(period, -(twice % period));
    unknown_80 = game::x87_truncated_product(game::x87_truncated_quotient(double(envelope), period), Amplitude);
    const int extension =
        game::x87_integer(game::x87_truncated_product(double(multiply(age, integer(unknown_50))), unknown_48));
    unknown_5C += unsigned(multiply(extension, AnotherLocation.X));
    unknown_60 += unsigned(multiply(extension, AnotherLocation.Y));
    unknown_64 += unsigned(multiply(extension, AnotherLocation.Z));
    unknown_68 = length(unknown_5C, unknown_60, unknown_64);
    unknown_8C = unsigned(game::x87_integer(game::x87_truncated_quotient(unknown_68, unknown_18)));
    AndAnotherLocation = divide(unknown_5C, unknown_60, unknown_64, integer(unknown_8C));
} catch (...) {
    std::abort();
}
void RadBeam::PrepareEruptionSegment() try {
    const auto displacement = [&](int index) {
        const double angle = game::x87_truncated_product(
            0.017453292519943295,
            game::x87_truncated_product(game::x87_truncated_quotient(double(index), integer(unknown_8C)), 180.0));
        return game::x87_truncated_product(Math::sin(angle), unknown_80);
    };
    const auto at = [&](int index) {
        CoordStruct result{add(SourceLocation.X, multiply(index, AndAnotherLocation.X)),
                           add(SourceLocation.Y, multiply(index, AndAnotherLocation.Y)),
                           add(SourceLocation.Z, multiply(index, AndAnotherLocation.Z))};
        result.Z = game::x87_integer(game::x87_truncated_sum(double(result.Z), displacement(index)));
        return result;
    };
    const int index = integer(unknown_88), next = add(index, 1);
    const auto a = at(index), b = at(next);
    unknown_9C = unsigned(a.X);
    unknown_A0 = unsigned(a.Y);
    unknown_A4 = unsigned(a.Z);
    unknown_A8 = unsigned(b.X);
    unknown_AC = unsigned(b.Y);
    unknown_B0 = unsigned(b.Z);
    unknown_B8 = game::x87_truncated_sum(
        game::x87_truncated_quotient(double(std::abs(game::x87_integer(displacement(next)))) * 0.5, Amplitude), 0.5);
    unknown_C0 = BYTE(game::x87_integer(game::x87_truncated_product(double(Color.R), unknown_B8)));
    unknown_C1 = BYTE(game::x87_integer(game::x87_truncated_product(double(Color.G), unknown_B8)));
    unknown_C2 = BYTE(game::x87_integer(game::x87_truncated_product(double(Color.B), unknown_B8)));
} catch (...) {
    std::abort();
}

#endif
