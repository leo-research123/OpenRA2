// Native callee failures are fatal; exceptions do not cross the weapon boundary.
// Original YR beam storage and type-1 geometry, 0x659110 / 0x659650 / 0x659AC0.
#include "yrpp/RadBeam.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <new>
namespace {
DynamicVectorClass<RadBeam*> beams;
int integer(DWORD v) { return std::bit_cast<int>(v); }
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
CoordStruct point(DWORD x, DWORD y, DWORD z) { return {integer(x), integer(y), integer(z)}; }
void clear_links(RadBeam* beam) {
    if (beam->unknown_0 && beam->unknown_0->FiringRadBeam == beam) {
        beam->unknown_0->FiringRadBeam = nullptr;
        beam->unknown_0 = nullptr;
    }
    if (beam->Owner && (beam->Owner->AbstractFlags & AbstractFlags::Techno) && beam->Owner->FiringRadBeam == beam) {
        beam->Owner->FiringRadBeam = nullptr;
        beam->Owner = nullptr;
    }
}
}
#if !defined(RA2_YRPP_GAME)
DynamicVectorClass<RadBeam*>& RadBeam::Array = beams;
RadBeam* YRPP_FASTCALL RadBeam::Allocate(RadBeamType mode) try {
    auto* memory = YRMemory::Allocate(sizeof(RadBeam));
    if (!memory) std::abort();
    auto* beam = ::new (memory) RadBeam;
    beam->unknown_0 = beam->Owner = nullptr;
    beam->unknown_8 = 0;
    beam->unknown_9 = 1;
    beam->unknown_C = 0;
    beam->Type = mode;
    beam->unknown_18 = mode == RadBeamType::RadBeam ? 10.0 : 20.0;
    beam->Color = {};
    beam->SourceLocation = beam->TargetLocation = CoordStruct::Empty;
    beam->Period = 0;
    beam->Amplitude = beam->unknown_48 = 0;
    beam->unknown_50 = 0;
    beam->unknown_54 = 1;
    beam->unknown_58 = 0;
    beam->unknown_7C = 0;
    beam->unknown_C0 = beam->unknown_C1 = beam->unknown_C2 = 0;
    try {
        Array.AddItem(beam);
    } catch (const std::bad_alloc&) {
        Array.IsInitialized = true;
    }
    return beam;
} catch (...) {
    std::abort();
}
void RadBeam::PrepareBeamSegment() try {
    const double degrees = 2.0 * std::max(360.0, unknown_68);
    const auto at = [&](int index) {
        const double angle = game::x87_truncated_product(
            0.017453292519943295,
            game::x87_truncated_product(degrees, game::x87_truncated_quotient(double(index), integer(unknown_8C))));
        const double displacement = game::x87_truncated_product(Math::sin(angle), unknown_80);
        const auto step = AndAnotherLocation;
        CoordStruct result{add(SourceLocation.X, std::bit_cast<int>(unsigned(index) * unsigned(step.X))),
                           add(SourceLocation.Y, std::bit_cast<int>(unsigned(index) * unsigned(step.Y))),
                           add(SourceLocation.Z, std::bit_cast<int>(unsigned(index) * unsigned(step.Z)))};
        result.Z = game::x87_integer(game::x87_truncated_sum(double(result.Z), displacement));
        return result;
    };
    const auto a = at(integer(unknown_88)), b = at(add(integer(unknown_88), 1));
    unknown_9C = unsigned(a.X);
    unknown_A0 = unsigned(a.Y);
    unknown_A4 = unsigned(a.Z);
    unknown_A8 = unsigned(b.X);
    unknown_AC = unsigned(b.Y);
    unknown_B0 = unsigned(b.Z);
    const double angle = game::x87_truncated_product(
        0.017453292519943295,
        game::x87_truncated_product(
            degrees, game::x87_truncated_quotient(double(add(integer(unknown_88), 1)), integer(unknown_8C))));
    const double displacement = game::x87_truncated_product(Math::sin(angle), unknown_80);
    unknown_B8 = game::x87_truncated_sum(
        game::x87_truncated_quotient(double(std::abs(game::x87_integer(displacement))) * 0.5, Amplitude), 0.5);
    unknown_C0 = BYTE(game::x87_integer(game::x87_truncated_product(double(Color.R), unknown_B8)));
    unknown_C1 = BYTE(game::x87_integer(game::x87_truncated_product(double(Color.G), unknown_B8)));
    unknown_C2 = BYTE(game::x87_integer(game::x87_truncated_product(double(Color.B), unknown_B8)));
} catch (...) {
    std::abort();
}


#endif
void YRPP_CDECL RadBeam::DeleteAll() try {
    while (Array.Count) {
        auto* beam = Array[0];
        clear_links(beam);
        Array.RemoveItem(0);
        GameDelete(beam);
    }
} catch (...) {
    std::abort();
}
