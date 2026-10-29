// YR 0x4C1F20 (three-strand subdivision, jitter order and depth interpolation), 0x4C2790 and 0x4C2830.
#include "presentation/builders/ebolt.hpp"
#include "presentation/random.hpp"
#include "yrpp/EBolt.h"
#include "surface_drawing.hpp"
#include "yrpp/Randomizer.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/YRMath.h"
#include "map_runtime.hpp"
#include "type_drawing.hpp"
#include "x87_integer.hpp"
#include <bit>
#include <cmath>
#include <climits>

namespace game {
namespace {
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
int sub(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
int mul(int a, int b) { return std::bit_cast<int>(unsigned(a) * unsigned(b)); }
void segment(PresentationContext& ctx, const CoordStruct& first, const CoordStruct& last, int index, int firstDepth, int lastDepth) {
    Point2D a{}, b{};
    TacticalClass::Instance->CoordsToClient(&first, &a);
    TacticalClass::Instance->CoordsToClient(&last, &b);
    const int za = sub(sub(firstDepth, TacticalClass::AdjustForZ(first.Z)), 2);
    const int zb = sub(sub(lastDepth, TacticalClass::AdjustForZ(last.Z)), 2);
    const auto* palette = map_runtime().action_line_palette;
    if (!palette) {
        record_type_drawing_result(ctx, DrawingStatus::unavailable);
        return;
    }
    const auto& c = palette->Entries[index];
    const WORD color = WORD((c.R >> 3) << 11 | (c.G >> 2) << 5 | (c.B >> 3));
    submit_color_line(ctx, DSurface::ViewBounds, a, b, color, za, zb);
}
}
void ebolt_draw(PresentationContext& ctx, const EBolt& bolt, CoordStruct first, CoordStruct last, int colorIndex) {
    const int x = sub(first.X, last.X), y = sub(first.Y, last.Y), z = sub(first.Z, last.Z);
    int distance = x87_integer(Math::sqrt(double(x) * x + double(z) * z + double(y) * y));
    if (!distance) return;
    const int separateAt = mul(102, distance) >> 8;
    int spread = mul(23, distance) >> 8;
    // These are the original eight stack records (0x58 bytes each on x86).
    struct Segment {
        CoordStruct first[3], last[3];
        int distance, spread, firstDepth, lastDepth;
    };
    Segment pending[8];
    int count = 0;
    CoordStruct starts[3]{first, first, first}, ends[3]{last, last, last};
    int firstDepth = std::bit_cast<int>(bolt.unknown_18), lastDepth = 0;
    bool wave = true;
    for (;;) {
        while (distance > 64 && count < 8) {
            CoordStruct middle[3];
            for (int i = 0; i < 3; ++i)
                middle[i] = {add(starts[i].X, ends[i].X) >> 1, add(starts[i].Y, ends[i].Y) >> 1,
                             add(starts[i].Z, ends[i].Z) >> 1};
            if (wave) {
                wave = false;
                int offset[6];
                for (int i = 0; i < 6; ++i)
                    offset[i] =
                        x87_integer(Math::sin(double(bolt.Random) * Math::Pi / double(i + 7)) * double(spread));
                for (auto& at : middle) {
                    at.X = add(at.X, add(offset[0], offset[3]));
                    at.Y = add(at.Y, add(offset[1], offset[5]));
                    at.Z = add(at.Z, add(add(offset[2], offset[4]), mul(2, spread)) >> 1);
                }
            }
            const auto jitter = [&]() {
                return distance > 128 ? presentation_random().RandomRanged(-spread, spread)
                                      : mul(mul(2, spread), presentation_random().RandomRanged(-1, 1));
            };
            middle[0].X = add(middle[0].X, jitter());
            middle[0].Y = add(middle[0].Y, jitter());
            middle[0].Z = add(middle[0].Z, jitter());
            for (int i = 1; i < 3; ++i) {
                const int dx = presentation_random().RandomRanged(-spread, spread);
                const int dy = presentation_random().RandomRanged(-spread, spread);
                const int dz = presentation_random().RandomRanged(-spread, spread);
                if (distance > separateAt)
                    middle[i] = {add(middle[0].X, dx >> 1), add(middle[0].Y, dy >> 1), add(middle[0].Z, dz >> 1)};
                else
                    middle[i] = {add(middle[i].X, dx), add(middle[i].Y, dy), add(middle[i].Z, dz)};
            }
            distance >>= 1;
            spread >>= 1;
            const int middleDepth = add(firstDepth, lastDepth) >> 1;
            auto& saved = pending[count++];
            for (int i = 0; i < 3; ++i) {
                saved.first[i] = middle[i];
                saved.last[i] = ends[i];
                ends[i] = middle[i];
            }
            saved.distance = distance;
            saved.spread = spread;
            saved.firstDepth = middleDepth;
            saved.lastDepth = lastDepth;
            lastDepth = middleDepth;
        }
        segment(ctx, starts[1], ends[1], colorIndex, firstDepth, lastDepth);
        segment(ctx, starts[2], ends[2], colorIndex, firstDepth, lastDepth);
        segment(ctx, starts[0], ends[0], 15, firstDepth, lastDepth);
        if (!count) break;
        const auto& saved = pending[--count];
        for (int i = 0; i < 3; ++i) {
            starts[i] = saved.first[i];
            ends[i] = saved.last[i];
        }
        distance = saved.distance;
        spread = saved.spread;
        firstDepth = saved.firstDepth;
        lastDepth = saved.lastDepth;
    }
}

void build_ebolts(PresentationContext& ctx) {
    // The bolt state (start, Random, Lifetime, existence) belongs to the render-phase steps in ebolt_lifetime.cpp.
    for (int i = EBolt::Array.Count - 1; i >= 0; --i) {
        const auto& bolt = *EBolt::Array[i];
        if (!bolt.Lifetime) continue;
        Point2D first{}, last{};
        TacticalClass::Instance->CoordsToClient(&bolt.Point1, &first);
        TacticalClass::Instance->CoordsToClient(&bolt.Point2, &last);
        auto bounds = DSurface::ViewBounds;
        if (Line_In_Bounds(&first, &last, &bounds)) ebolt_draw(ctx, bolt, bolt.Point1, bolt.Point2, bolt.AlternateColor ? 5 : 10);
    }
}
}
