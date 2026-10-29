// YR 0x4521C0 / 0x452210: freeze the original 21 animation slots.
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include <cstdlib>
#if !defined(RA2_YRPP_GAME)
void BuildingClass::DisableTemporal() try {
    HasPower = false;
    for (auto* anim : Anims)
        if (anim) {
            anim->Pause();
            anim->UnderTemporal = true;
        }
} catch (...) {
    std::abort();
}
void BuildingClass::EnableTemporal() try {
    HasPower = true;
    for (auto* anim : Anims)
        if (anim) {
            anim->Unpause();
            anim->UnderTemporal = false;
        }
} catch (...) {
    std::abort();
}

#endif
