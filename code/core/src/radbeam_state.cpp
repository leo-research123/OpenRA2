// YR 0x659650 / 0x659CA0: original per-round state transitions, with terminal pixels supplied by a sink.
#include "radbeam_state.hpp"
#include "yrpp/TechnoClass.h"
#include "yrpp/FPSCounter.h"
#include "yrpp/Randomizer.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
namespace game {
namespace {
int integer(DWORD v) { return std::bit_cast<int>(v); }
CoordStruct point(DWORD x, DWORD y, DWORD z) { return {integer(x), integer(y), integer(z)}; }
bool advance_beam(RadBeam& beam, RadBeamSegmentSink sink, void* context) try {
    const auto prepare = [&] {
        beam.unknown_5C = unsigned(beam.TargetLocation.X) - unsigned(beam.SourceLocation.X);
        beam.unknown_60 = unsigned(beam.TargetLocation.Y) - unsigned(beam.SourceLocation.Y);
        beam.unknown_64 = unsigned(beam.TargetLocation.Z) - unsigned(beam.SourceLocation.Z);
        const unsigned square = beam.unknown_5C * beam.unknown_5C + beam.unknown_60 * beam.unknown_60 + beam.unknown_64 * beam.unknown_64;
        beam.unknown_68 = Math::sqrt(double(integer(square)));
    };
    if (!beam.unknown_8) {
        beam.unknown_8 = 1;
        prepare();
    }
    if (beam.unknown_58 && beam.Owner) {
        CoordStruct buffer;
        beam.TargetLocation = (beam.Owner->AbstractFlags & AbstractFlags::Object) ? *beam.Owner->GetTargetCoords(&buffer)
                                                                        : *beam.Owner->GetCenterCoords(&buffer);
        prepare();
    }
    if (beam.unknown_9) {
        beam.unknown_8C = unsigned(game::x87_integer(game::x87_truncated_quotient(beam.unknown_68, beam.unknown_18)));
        // Integer vector division in 0x65A1B0 retains the vector for zero.
        const int divisor = integer(beam.unknown_8C);
        beam.AndAnotherLocation = point(beam.unknown_5C, beam.unknown_60, beam.unknown_64);
        if (divisor) {
            beam.AndAnotherLocation.X /= divisor;
            beam.AndAnotherLocation.Y /= divisor;
            beam.AndAnotherLocation.Z /= divisor;
        }
        beam.unknown_88 = 0;
        beam.unknown_80 = game::x87_truncated_product(
            game::x87_truncated_quotient(double(integer(beam.unknown_7C)), integer(beam.Period)), beam.Amplitude);
        beam.unknown_9C = unsigned(beam.SourceLocation.X);
        beam.unknown_A0 = unsigned(beam.SourceLocation.Y);
        beam.unknown_A4 = unsigned(beam.SourceLocation.Z);
        beam.unknown_C0 = beam.Color.R;
        beam.unknown_C1 = beam.Color.G;
        beam.unknown_C2 = beam.Color.B;
        while (double(integer(beam.unknown_88)) * beam.unknown_18 < beam.unknown_68) {
            beam.PrepareBeamSegment();
            if (sink) sink(context, beam);
            ++beam.unknown_88;
        }
    }
    if (beam.unknown_58) {
        beam.unknown_7C += beam.unknown_54;
        if (integer(beam.unknown_7C) <= 0 || integer(beam.unknown_7C) >= integer(beam.Period)) {
            beam.unknown_54 = 0u - beam.unknown_54;
            if (integer(beam.unknown_7C) < 0) beam.unknown_7C = 0;
        }
        return false;
    }
    ++beam.unknown_7C;
    return integer(beam.unknown_7C) >= integer(beam.Period);
} catch (...) {
    std::abort();
}
bool advance_eruption(RadBeam& beam, RadBeamSegmentSink sink, void* context) try {
    if (!beam.unknown_8) {
        beam.unknown_8 = 1;
        beam.InitializeEruption();
    }
    if (beam.unknown_9) {
        beam.AdvanceEruption();
        beam.unknown_88 = 0;
        while (integer(beam.unknown_88) < integer(beam.unknown_8C)) {
            beam.PrepareEruptionSegment();
            if (sink) sink(context, beam);
            ++beam.unknown_88;
        }
    }
    ++beam.unknown_7C;
    return integer(beam.unknown_7C) >= integer(beam.Period);
} catch (...) {
    std::abort();
}
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
bool advance_radbeam(RadBeam& beam, RadBeamSegmentSink sink, void* context) {
    if (beam.Type == RadBeamType::RadBeam) return advance_beam(beam, sink, context);
    if (beam.Type == RadBeamType::Eruption) return advance_eruption(beam, sink, context);
    return false;
}
void radbeam_before_draw() {
    int total = 0, visible = 0;
    for (int i = RadBeam::Array.Count - 1; i >= 0; --i)
        if (RadBeam::Array[i]->Type == RadBeamType::Eruption) {
            ++total;
            if (RadBeam::Array[i]->unknown_9) ++visible;
        }
    const double ratio =
        game::x87_truncated_quotient(double(FPSCounter::CurrentFrameRate), Game::Network::RequestedFPS);
    const double cube = game::x87_truncated_product(ratio, game::x87_truncated_product(ratio, ratio));
    const double fraction =
        total ? game::x87_truncated_difference(game::x87_truncated_quotient(double(visible), total), cube) : 0.0;
    const double drop =
        visible && fraction > 0.0
            ? game::x87_truncated_quotient(game::x87_truncated_product(double(total), fraction), visible)
            : 0.0;
    const double spacing = game::x87_truncated_quotient(1.0, cube);
    for (int i = RadBeam::Array.Count - 1; i >= 0; --i) {
        auto* beam = RadBeam::Array[i];
        if (beam->Type == RadBeamType::RadBeam)
            beam->unknown_18 = std::clamp(x87_integer(x87_truncated_product(spacing, 10.0)), 10, 100);
        else if (beam->Type == RadBeamType::Eruption && beam->unknown_9) {
            const int pick = Randomizer::Global.RandomRanged(0, 99);
            if (pick < x87_integer(x87_truncated_product(drop, 100.0))) beam->unknown_9 = 0;
        }
    }
}
void radbeam_after_draw() {
    for (int i = RadBeam::Array.Count - 1; i >= 0; --i) {
        auto* beam = RadBeam::Array[i];
        if (!advance_radbeam(*beam)) continue;
        clear_links(beam);
        RadBeam::Array.RemoveItem(i);
        GameDelete(beam);
    }
}
}
