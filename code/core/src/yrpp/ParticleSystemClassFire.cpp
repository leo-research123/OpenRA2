// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 partsys.cpp Fire_AI/Spawn_Held_Particle_Random.
// YR 0x0062F9A0 / 0x0062E4C0; EA terms: third_party/opents/LICENSE.md.
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WaveClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
int wrapped_square_sum(const CoordStruct& a, const CoordStruct& b) {
    const unsigned x = unsigned(a.X) - unsigned(b.X), y = unsigned(a.Y) - unsigned(b.Y),
                   z = unsigned(a.Z) - unsigned(b.Z);
    return std::bit_cast<int>(x * x + y * y + z * z);
}
int fire_direction(const CoordStruct& from, const CoordStruct& to) {
    const int dx = to.X - from.X, dy = from.Y - to.Y;
    const double tangent = Math::sin(Math::Pi / 8) / Math::cos(Math::Pi / 8);
    if (dx) {
        const double slope = double(dy) / dx;
        if (slope >= tangent && slope < 1 / tangent) return dx > 0 ? 1 : 5;
        if (slope >= -tangent && slope < tangent) return dx > 0 ? 2 : 6;
        if (slope < -tangent && slope >= -1 / tangent) return dx > 0 ? 3 : 7;
    }
    return dy > 0 ? 0 : 4;
}
}
ParticleClass* ParticleSystemClass::SpawnParticle(const CoordStruct& from, const CoordStruct& to, int window) {
    try {
        if (Type->HoldsWhat == -1) return nullptr;
        auto a = from, b = to;
        auto* storage = YRMemory::Allocate(sizeof(ParticleClass));
        if (!storage) return nullptr;
        auto* particle = ::new (storage) ParticleClass(ParticleTypeClass::Array[Type->HoldsWhat], &a, &b, this);
        // The original returns null after construction for a nonpositive window.
        if (!particle || window <= 0) return nullptr;
        Particles.AddItem(particle);
        const int count = Particles.Count, limit = std::min(count, window);
        if (limit <= 0) std::abort();
        const int sample = ScenarioClass::Instance->Random.Random();
        const int absolute = std::bit_cast<int>(sample < 0 ? 0u - unsigned(sample) : unsigned(sample));
        const int index = count - absolute % limit - 1;
        for (int i = count - 2; i >= index; --i)
            Particles.Items[i + 1] = Particles.Items[i];
        Particles.Items[index] = particle;
        return particle;
    } catch (...) {
        std::abort();
    }
}
void ParticleSystemClass::FireAI() {
    try {
        for (int i = Particles.Count - 1; i >= 0; --i) {
            auto* p = Particles[i];
            p->BehaviorAI();
            p->MotionAI();
        }
        for (int i = Particles.Count - 1; i >= 0; --i)
            if (Particles[i]->IsToDie) Particles[i]->UnInit();
        bool reaimed = false;
        TechnoClass* source = nullptr;
        if (Owner) {
            switch (Owner->WhatAmI()) {
                case AbstractType::Aircraft:
                case AbstractType::Building:
                case AbstractType::Infantry:
                case AbstractType::Unit:
                    source = static_cast<TechnoClass*>(Owner);
                    break;
                default:
                    break;
            }
        }
        if (source) {
            if (source->IsAlive && source->Target && source->PrimaryFacing.IsRotating()) {
                const auto target = source->Target->GetCoords(), at = source->Location;
                const double dx = double(target.X - at.X), dy = double(target.Y - at.Y), dz = double(target.Z - at.Z);
                const int distance = game::x87_integer(Math::sqrt(dx * dx + dy * dy + dz * dz));
                const auto direction = source->PrimaryFacing.Current();
                const double angle = double(int(static_cast<short>(direction.Raw)) - 0x3FFF) * -0.00009587672516830327;
                TargetCoords = {game::x87_integer(Math::cos(angle) * distance + at.X),
                                game::x87_integer(at.Y - Math::sin(angle) * distance), at.Z};
                CoordStruct muzzle{};
                source->GetFLH(&muzzle, 0, CoordStruct{});
                SetLocation(muzzle);
                reaimed = true;
            }
            if (!source->IsAlive) UnInit();
        } else
            UnInit();
        if (TimeToDie) return;
        const int frame = Unsorted::CurrentFrame;
        if (frame % Type->SpawnFrames && (frame % 3 || !reaimed)) return;
        // 0x0075F020 fills the shared wave table when the world is finished.
        const int sine = WaveClass::SonicAmplitudeTable[frame % 500];
        const int divisor = Math::sqrt(double(wrapped_square_sum(Location, TargetCoords))) < 200.0 ? 3 : 1;
        const int shift = game::x87_integer(double(12 * sine) / (double(divisor) * 3.0));
        constexpr int stepX[]{1, 1, 0, -1, -1, -1, 0, 1}, stepY[]{0, 1, 1, 1, 0, -1, -1, -1};
        const int facing = fire_direction(Location, TargetCoords);
        CoordStruct target{TargetCoords.X + shift * stepX[facing], TargetCoords.Y + shift * stepY[facing],
                           TargetCoords.Z};
        auto origin = Location;
        ++origin.Z;
        SpawnParticle(origin, target, 4);
    } catch (...) {
        std::abort();
    }
}
#endif
