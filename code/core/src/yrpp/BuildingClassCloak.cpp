// YR 0x00454DB0: building translucency and natural-particle cloak lifecycle.
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/MapClass.h"
#include "map_world.hpp"

#if !defined(RA2_YRPP_GAME)
void BuildingClass::UpdateCloak(bool) {
    if (!Type) return;
    const auto appearance = [&] {
        char level = Translucency;
        if (level == 15 && VisualCharacter(0, nullptr) == VisualType::Hidden) level = 16;
        for (auto* anim : Anims)
            if (anim) anim->TranslucencyLevel = level;
    };
    if (CloakState == ::CloakState::Cloaking && Translucency < 15) {
        ++Translucency;
        if (Translucency == 1 || Translucency == 6 || Translucency == 11) {
            NeedsRedraw = true;
            game::map_object_changed();
        }
        appearance();
        if (Translucency == 15) {
            CloakState = ::CloakState::Cloaked;
            // This entry deletes the natural system immediately, unlike damage-smoke UnInit.
            delete NaturalParticleSystem;
            NaturalParticleSystem = nullptr;
        }
    } else if (CloakState == ::CloakState::Uncloaking && Translucency >= 0) {
        if (Translucency > 0) --Translucency;
        if (Translucency == 0 || Translucency == 5 || Translucency == 10) {
            NeedsRedraw = true;
            game::map_object_changed();
        }
        appearance();
        if (Translucency == 0) {
            CloakState = ::CloakState::Uncloaked;
            if (!NaturalParticleSystem && Type->NaturalParticleSystemLocation != CoordStruct::Empty &&
                Type->NaturalParticleSystem)
                if (auto* memory = YRMemory::Allocate(sizeof(ParticleSystemClass)))
                    NaturalParticleSystem = ::new (memory) ParticleSystemClass(
                        Type->NaturalParticleSystem, Location + Type->NaturalParticleSystemLocation,
                        MapClass::Instance.GetCellAt(Location), nullptr, CoordStruct::Empty, nullptr);
        }
    }
    if (CloakState == ::CloakState::Cloaked && ShouldNotBeCloaked()) Uncloak(false);
    if (CloakState == ::CloakState::Uncloaked && IsReadyToCloak()) Cloak(false);
    // The original's subsequent cloak-generator radius raster is a separate,
    // still-unported map coverage dependency; it does not drive natural smoke.
}
#endif
