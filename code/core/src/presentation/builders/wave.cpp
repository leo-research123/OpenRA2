// YR 0x75F9F0, 0x75FA90, 0x7602E0, 0x7609E0: ordered background refraction.
#include "yrpp/WaveClass.h"
#include "presentation/builders/wave.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TacticalClass.h"
#include "type_drawing.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <cstdlib>
#include <vector>

namespace game {
namespace {
// Original polygon edge walker 0x63F2E0, with one output per scanline.
void edge(Point2D from, Point2D to, bool left, bool skip, Point2D*& output) {
    const int dx = to.X - from.X, dy = to.Y - from.Y;
    if (dy <= 0) return;
    const int direction = dx > 0 ? 1 : -1, absolute = std::abs(dx);
    int error = dx < 0 ? 1 - dy : 0;
    const int whole = direction * (absolute / dy), remainder = absolute % dy;
    const auto advance = [&]() {
        from.X += whole;
        error += remainder;
        if (error > 0) {
            from.X += direction;
            error -= dy;
        }
    };
    if (skip) advance();
    for (int row = int(skip); row < dy; ++row) {
        if (left)
            output->X = from.X;
        else
            output->Y = from.X;
        ++output;
        advance();
    }
}
// Original convex polygon conversion 0x63F080. The top and right endpoint
// rules matter for the narrow triangular sonic wave and horizontal edges.
void spans(const WaveClass& wave, std::vector<Point2D>& scanlines, int& rows, int& top_y) {
    const int count = wave.PointData_Counter;
    const auto* points = wave.PointData_Pointer;
    if (count < 2) return;
    int top = 0, bottom = 0;
    for (int i = 1; i < count; ++i) {
        if (points[i].Y < points[top].Y) top = i;
        if (points[i].Y > points[bottom].Y) bottom = i;
    }
    const int low = points[top].Y, high = points[bottom].Y;
    if (low == high) return;
    int forward = top, backward = top;
    while (points[forward].Y == low)
        forward = (forward + 1) % count;
    forward = (forward + count - 1) % count;
    while (points[backward].Y == low)
        backward = (backward + count - 1) % count;
    backward = (backward + 1) % count;
    const bool flat = points[forward].X != points[backward].X;
    int left = backward, right = forward, direction = -1;
    if (flat ? points[left].X > points[right].X
             : (points[(forward + 1) % count].X - points[left].X) *
                           (points[(backward + count - 1) % count].Y - points[left].Y) -
                       (points[(backward + count - 1) % count].X - points[left].X) *
                           (points[(forward + 1) % count].Y - points[left].Y) <
                   0) {
        std::swap(left, right);
        direction = 1;
    }
    rows = int(flat) - low + high - 1;
    if (rows <= 0) return;
    top_y = low - int(flat) + 1;
    scanlines.resize(rows + 200);
    for (int side = 0; side < 2; ++side) {
        int index = side ? right : left, walk = side ? -direction : direction;
        auto* output = scanlines.data();
        bool skip = !flat;
        do {
            const int next = (index + count + walk) % count;
            auto first = points[index], last = points[next];
            if (side) {
                --first.X;
                --last.X;
            }
            edge(first, last, !side, skip, output);
            skip = false;
            index = next;
        } while (index != bottom);
    }
}
}

void build_wave(PresentationContext& ctx, const WaveClass& self, Point2D* location, RectangleStruct* bounds) {
    const auto* context = ctx.type;
    if (!context || !location || !bounds) return;
    if (ScenarioClass::Instance->SpecialFlags.FogOfWar && MapClass::Instance.IsLocationFogged(self.LimboCoords) &&
        MapClass::Instance.IsLocationFogged(self.Pos0))
        return;
    std::vector<Point2D> scanlines;
    int rows = 0, top_y = 0;
    try {
        spans(self, scanlines, rows, top_y);
        if (scanlines.empty()) return;
        const int offsetX = location->X - self.WaveStartMiddle.X;
        const int offsetY = location->Y - self.WaveStartMiddle.Y;
        const int first = offsetY + context->overlay_offset.Y + top_y;
        const int last = std::min(first + rows - 1, bounds->Y + bounds->Height);
        constexpr Point2D directions[]{{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
        const bool laser = self.Type == WaveType::Laser || self.Type == WaveType::BigLaser;
        const int margin = laser ? 0 : 3;
        const bool forward = laser || (self.FacingIndex > 1 && self.FacingIndex < 6);
        std::vector<game::WavePixel> pixels;
        for (int y = forward ? first : last; forward ? y <= last : y >= first; y += forward ? 1 : -1) {
            if (y < bounds->Y + margin || y > bounds->Y + bounds->Height - margin) continue;
            const auto span = scanlines[y - first];
            const int left = std::max(offsetX + span.X, bounds->X + margin);
            const int right = std::min(offsetX + span.Y, bounds->X + bounds->Width - margin);
            for (int x = forward ? left : right; forward ? x <= right : x >= left; x += forward ? 1 : -1) {
                Point2D sample{x, y};
                int strength = self.LaserEC;
                if (!laser) {
                    const int dx = std::abs(x - self.WaveStartMiddle.X - offsetX),
                              dy = std::abs(y - offsetY - self.WaveStartMiddle.Y);
                    const int distance = game::x87_integer(Math::sqrt(double(dx * dx + dy * dy)));
                    // 0x0075EDF0 / 0x00760190 read the tables filled by 0x0075F020. The original indexes past
                    // their end for far pixels; those indices keep the equivalent direct evaluation.
                    const bool sonic = self.Type == WaveType::Sonic;
                    const int index = self.WaveEC + distance;
                    const int size = sonic ? 500 : 496;
                    int amplitude =
                        std::abs(index >= 0 && index < size
                                     ? (sonic ? WaveClass::SonicAmplitudeTable[index]
                                              : WaveClass::MagnetronAmplitudeTable[index])
                                     : game::x87_integer(Math::sin(double(index) * (sonic ? 0.125 : 0.1)) * 12 + 0.49));
                    if (self.Type == WaveType::Magnetron) amplitude = std::min(amplitude, 9);
                    const int shift = amplitude < 2 ? 0 : amplitude < 7 ? 1 : amplitude < 10 ? 2 : 3;
                    sample.X += directions[self.FacingIndex].X * shift;
                    sample.Y += directions[self.FacingIndex].Y * shift;
                    strength = WaveClass::StrengthTable[amplitude];
                }
                pixels.push_back({{x, y}, sample, strength});
            }
        }
        game::RasterDrawingRequest request;
        request.target = context->target;
        request.blend_mode = game::RasterBlendMode::wave;
        request.clip = *bounds;
        request.color = static_cast<WORD>(self.Type);
        request.line_z = -TacticalClass::AdjustForZ(self.LimboCoords.Z) - 2;
        request.wave_pixels = pixels.data();
        request.wave_pixel_count = static_cast<unsigned>(pixels.size());
        game::record_type_drawing_result(ctx, game::submit_type_raster(*context, request));
    } catch (...) {
        game::record_type_drawing_result(ctx, game::DrawingStatus::backend_failure);
    }
}

}
