// YR 0x00550260 / 0x005509F0: software laser presentation.
#include "yrpp/LaserDrawClass.h"
#include "presentation/builders/laser.hpp"
#include "surface_drawing.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "yrpp/Randomizer.h"
#include "presentation/random.hpp"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/FPSCounter.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>

namespace game {

namespace {
// Each entry is the pair of offsets for the two sides of an octant.
constexpr int houseOffsets[8][4] = {{-1, -1, 1, 1}, {0, -1, 0, 1}, {-1, 1, 1, -1}, {-1, 0, 1, 0},
                                    {-1, -1, 1, 1}, {0, -1, 0, 1}, {-1, 1, 1, -1}, {-1, 0, 1, 0}};
constexpr int outerOffsets[8][4] = {{0, -1, 1, 0}, {0, -1, 0, 1}, {1, 0, 0, 1},   {-1, 0, 1, 0},
                                    {-1, 0, 0, 1}, {0, -1, 1, 0}, {0, -1, -1, 0}, {-1, 0, 1, 0}};
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
int sub(int a, int b) { return std::bit_cast<int>(unsigned(a) - unsigned(b)); }
unsigned octant(const LaserDrawClass& laser) {
    const double angle = Math::atan2(double(laser.Source.Y) - laser.Target.Y, double(laser.Target.X) - laser.Source.X);
    const auto raw = static_cast<unsigned short>(game::x87_integer((angle - 1.570796326794897) * -10430.06004058427));
    return (((unsigned(raw) >> 12) + 1) >> 1) & 7;
}
float intensity(const LaserDrawClass& laser) {
    if (!laser.Fades) return 1.0f;
    return float((double(laser.StartIntensity) - double(laser.EndIntensity)) *
                     double(sub(laser.Duration, laser.Progress.Value)) / double(laser.Duration) +
                 double(laser.EndIntensity));
}
WORD pack(ColorStruct c) { return WORD(Drawing::RGB_To_Int(c.R, c.G, c.B)); }
// 0x6612C0 interpolates toward white using an 8-bit ratio.
ColorStruct dim(ColorStruct c, int opacity) {
    const auto channel = [opacity](BYTE v) { return BYTE(int(v) + (((opacity & 255) * (255 - int(v))) / 256)); };
    return {channel(c.R), channel(c.G), channel(c.B)};
}
struct Projection {
    Point2D first, last;
    int firstZ, lastZ;
    RectangleStruct bounds;
};
Projection project(const LaserDrawClass& laser) {
    Projection p{};
    TacticalClass::Instance->CoordsToClient(&laser.Source, &p.first);
    TacticalClass::Instance->CoordsToClient(&laser.Target, &p.last);
    p.firstZ = sub(sub(laser.ZAdjust, TacticalClass::AdjustForZ(laser.Source.Z)), 2);
    p.lastZ = sub(-2, TacticalClass::AdjustForZ(laser.Target.Z));
    p.bounds = DSurface::ViewBounds.Width > 0 ? DSurface::ViewBounds : TacticalClass::ViewBounds;
    return p;
}
Point2D offset(Point2D p, int x, int y) { return {add(p.X, x), add(p.Y, y)}; }
}

void build_house_laser(PresentationContext& ctx, const LaserDrawClass& self) {
    const auto direction = octant(self);
    const auto p = project(self);
    const bool detailed =
        FPSCounter::CurrentFrameRate >= Detail::GetMinFrameRate() && GameOptionsClass::Instance.DetailLevel != 0;
    const float fade = intensity(self);
    const int opacity = game::x87_integer(double(fade) * 255.0);
    const auto twice = [](BYTE c) { return BYTE(std::min(255, 2 * int(c))); };
    const auto half = [](ColorStruct c) { return ColorStruct{BYTE(c.R >> 1), BYTE(c.G >> 1), BYTE(c.B >> 1)}; };
    const ColorStruct center =
        self.IsSupported ? ColorStruct{twice(self.InnerColor.R), twice(self.InnerColor.G), twice(self.InnerColor.B)} : self.InnerColor;
    auto side = self.IsSupported ? center : half(self.InnerColor);
    auto a = p.first, b = p.last, c = a, d = b;
    const auto line = [&](Point2D first, Point2D last, ColorStruct color) {
        if (detailed)
            game::submit_rgb_multiplying_line(ctx, p.bounds, first, last, color, fade, p.firstZ, p.lastZ);
        else
            game::submit_color_line(ctx, p.bounds, first, last, pack(dim(color, opacity)), p.firstZ, p.lastZ);
    };
    for (int layer = 1; layer <= self.Thickness; ++layer) {
        const auto* o = houseOffsets[direction];
        const bool x = (direction & 1) || (layer & 1), y = (direction & 1) || !(layer & 1);
        a = offset(a, x ? o[0] : 0, y ? o[1] : 0);
        b = offset(b, x ? o[0] : 0, y ? o[1] : 0);
        c = offset(c, y ? o[2] : 0, x ? o[3] : 0);
        d = offset(d, y ? o[2] : 0, x ? o[3] : 0);
        line(a, b, side);
        line(c, d, side);
        if (self.IsSupported && layer == 1)
            side = self.InnerColor;
        else {
            const auto next = half(side);
            const int threshold = detailed ? 8 : 64;
            if (next.R < threshold && next.G < threshold && next.B < threshold) break;
            side = next;
        }
    }
    line(p.first, p.last, center);
}

void build_laser(PresentationContext& ctx, const LaserDrawClass& self) {
    if (self.Duration <= 0) return;
    if (self.IsHouseColor) {
        build_house_laser(ctx, self);
        return;
    }
    if (self.BlinkState) return;
    const auto direction = octant(self);
    const auto p = project(self);
    const bool hasOuter = self.OuterColor.R || self.OuterColor.G || self.OuterColor.B;
    auto outer = ColorStruct{};
    if (hasOuter) {
        const auto channel = [](BYTE color, BYTE spread) {
            return BYTE(std::clamp(int(color) + presentation_random().RandomRanged(-int(spread), spread), 0, 255));
        };
        outer.R = channel(self.OuterColor.R, self.OuterSpread.R);
        outer.G = channel(self.OuterColor.G, self.OuterSpread.G);
        outer.B = channel(self.OuterColor.B, self.OuterSpread.B);
    }
    const float fade = intensity(self);
    const auto side = [&](bool multiplying) {
        const auto* o = outerOffsets[direction];
        for (int i = 0; i < 2; ++i) {
            const auto a = offset(p.first, o[2 * i], o[2 * i + 1]), b = offset(p.last, o[2 * i], o[2 * i + 1]);
            if (multiplying)
                game::submit_rgb_multiplying_line(ctx, p.bounds, a, b, outer, fade, p.firstZ, p.lastZ);
            else if (GameOptionsClass::Instance.DetailLevel)
                game::submit_subtractive_line(ctx, p.bounds, a, b, outer, p.firstZ, p.lastZ, true, true, true, 1.0f);
            else
                game::submit_color_line(ctx, p.bounds, a, b, pack(outer), p.firstZ, p.lastZ);
        }
    };
    if (GameOptionsClass::Instance.DetailLevel) {
        if (self.Blinks) {
            game::submit_subtractive_line(ctx, p.bounds, p.first, p.last, self.InnerColor, p.firstZ, p.lastZ, true, true, true,
                                          fade);
            if (hasOuter) side(false);
        } else if (hasOuter) {
            side(true);
            game::submit_rgb_multiplying_line(ctx, p.bounds, p.first, p.last, self.InnerColor, fade, p.firstZ, p.lastZ);
        } else
            game::submit_subtractive_line(ctx, p.bounds, p.first, p.last, self.InnerColor, p.firstZ, p.lastZ, self.InnerColor.R != 0,
                                          self.InnerColor.G != 0, self.InnerColor.B != 0, fade);
    } else {
        game::submit_color_line(ctx, p.bounds, p.first, p.last, pack(self.InnerColor), p.firstZ, p.lastZ);
        if (hasOuter) side(false);
    }
}

}

namespace game { void build_lasers(PresentationContext& ctx) { for (int i=LaserDrawClass::Array.Count-1;i>=0;--i) build_laser(ctx,*LaserDrawClass::Array[i]); } }
