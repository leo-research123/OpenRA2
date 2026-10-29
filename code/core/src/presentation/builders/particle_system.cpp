#include "presentation/builders/particle_system.hpp"
#include "presentation/builders/spotlight.hpp"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/RulesClassReaders.hpp"
#include <algorithm>
namespace game {
void build_particle_system(PresentationContext& ctx, const ParticleSystemClass& self, Point2D*, RectangleStruct*) {
    if (self.Type && self.Type->OneFrameLight && self.Type->LightSize > 0 && self.Particles.Count > 0) {
        const double fullness = std::clamp(double(self.Particles.Count) / self.Type->ParticleCap, 0.4, 1.0);
        build_spotlight(ctx, self.Location, rule_integer(fullness * self.Type->LightSize), self.SpotlightRadius, SpotlightFlags::None);
    }
}
}
