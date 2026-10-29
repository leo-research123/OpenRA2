// YR 0x0062E6D0: the default system updates only explicitly spawned particles.
#include "yrpp/ParticleSystemClass.h"

void ParticleSystemClass::GasAI() {
    for (int i = Particles.Count - 1; i >= 0; --i)
        Particles[i]->BehaviorAI();
    for (int i = Particles.Count - 1; i >= 0; --i) {
        auto* particle = Particles[i];
        if (!particle->IsToDie) {
            particle->MotionAI();
            continue;
        }
        auto* next = ParticleTypeClass::Array.GetItemOrDefault(particle->Type->NextParticle);
        if (next) {
            auto at = particle->Location + particle->Type->NextParticleOffset, target = CoordStruct::Empty;
            if (auto* child = GameCreate<ParticleClass>(next, &at, &target, this)) {
                if (Particles.AddItem(child)) {
                    child->Speed = particle->Speed;
                    child->Velocity = particle->Velocity;
                } else
                    delete child;
            }
        }
        particle->UnInit();
    }
}
