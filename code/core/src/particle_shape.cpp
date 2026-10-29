#include "particle_shape.hpp"
#include "yrpp/ParticleClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/YRMath.h"
#include <bit>
namespace game {
int particle_shape_number(const ParticleClass& self) {
    const auto behavior = self.Type->BehavesLike;
    const int frame = static_cast<signed char>(self.StartStateAI);
    if (behavior == ParticleBehavior::Gas || behavior == ParticleBehavior::Smoke) return frame;
    if (behavior != ParticleBehavior::Fire) return 0;
    Point2D from, to;
    TacticalClass::Instance->CoordsToClient(&self.unknown_coords_F4, &from);
    TacticalClass::Instance->CoordsToClient(&self.unknown_coords_E8, &to);
    // 0x75F230 classifies eight screen directions at pi/8 boundaries.
    const int dx = std::bit_cast<int>(unsigned(to.X) - unsigned(from.X));
    const int dy = std::bit_cast<int>(unsigned(from.Y) - unsigned(to.Y));
    int direction = dy > 0 ? 0 : 4;
    if (dx) {
        const double threshold = Math::sin(Math::Pi / 8) / Math::cos(Math::Pi / 8);
        const double slope = double(dy) / dx;
        if (slope >= threshold && slope < 1.0 / threshold)
            direction = dx > 0 ? 1 : 5;
        else if (slope >= -threshold && slope < threshold)
            direction = dx > 0 ? 2 : 6;
        else if (slope >= -1.0 / threshold && slope < -threshold)
            direction = dx > 0 ? 3 : 7;
    }
    const int column = direction & 3;
    return frame + column * static_cast<signed char>(self.Type->EndStateAI);
}
}
