// Original 660000/660050: four rotating, contracting gradient edges.
#include "yrpp/RadarEventClass.h"
#include "surface_drawing.hpp"
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "game_ui_runtime.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

#include "presentation/ui/radar_events.hpp"
namespace game {
void build_radar_event(GameUiFrame& context, const RadarEventClass& self) noexcept {
    auto* frame = &context;
    ColorStruct high{}, low{};
    switch (self.Type) {
        case RadarEventType::Combat:
        case RadarEventType::BaseAttacked:
        case RadarEventType::HarvesterAttacked:
            high = {255, 0, 255};
            low = {128, 0, 128};
            break;
        case RadarEventType::Noncombat:
        case RadarEventType::DropZone:
        case RadarEventType::BeaconPlaced:
        case RadarEventType::SuperweaponDetected:
            high = {255, 255, 0};
            low = {128, 128, 0};
            break;
        case RadarEventType::EnemySensed:
            high = {0, 255, 255};
            low = {0, 128, 128};
            break;
        default:
            return;
    }
    Point2D points[4];
    if (!self.GetVertices(points, 4)) return;
    const int denominator = std::max(std::abs(points[0].X - points[1].X), std::abs(points[0].Y - points[1].Y));
    if (!denominator) return;
    const double raw_step = (double(self.Speed) * 2) * 0.7071067811865475 / denominator * self.ColorSpeed;
    const float nearest = static_cast<float>(raw_step);
    float step = std::abs(double(nearest)) > std::abs(raw_step)
                     ? std::bit_cast<float>(std::bit_cast<unsigned>(nearest) - 1u)
                     : nearest;
    float phase = self.ColorValue;
    const auto& radar = RadarClass::Instance;
    const auto& terrain = radar.unknown_rect_149C;
    const RectangleStruct clip{DSurface::SidebarBounds.X + terrain.X, terrain.Y, terrain.Width, terrain.Height};
    for (auto& p : points) {
        p.X += self.RadarX;
        p.Y += self.RadarY;
    }
    for (unsigned i = 0; i < 4; ++i)
        game::record_ui_drawing(game::submit_gradient_line(frame->drawing.types, clip, points[i], points[(i + 1) % 4],
                                                           high, low, step, phase));
}

void build_radar_events(GameUiFrame& context) noexcept {
    for (const auto* event : RadarEventClass::Array)
        if (event->Rotating || event->VisibilityTimer.HasTimeLeft()) build_radar_event(context, *event);
}
}
