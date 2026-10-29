// Original 0x71AB60 / 0x71AD40 / 0x71ADE0: chain expiration.
#include "yrpp/TemporalClass.h"
#include "yrpp/TechnoClass.h"
#include <cstdlib>

#if !defined(RA2_YRPP_GAME)
void TemporalClass::JustLetGo() noexcept {
    if (Target) {
        Target->BeingWarpedOut = false;
        Target->TemporalTargetingMe = nullptr;
        Target = nullptr;
    }
    if (PrevTemporal) {
        if (PrevTemporal->NextTemporal == this) PrevTemporal->NextTemporal = nullptr;
        PrevTemporal->Detach();
        PrevTemporal = nullptr;
    }
    if (NextTemporal) {
        if (NextTemporal->PrevTemporal == this) NextTemporal->PrevTemporal = nullptr;
        NextTemporal->Detach();
        NextTemporal = nullptr;
    }
    PrevTemporal = NextTemporal = nullptr;
    SourceSW = nullptr;
    unknown_pointer_38 = nullptr;
}

void TemporalClass::Detach() noexcept {
    try {
        Target->BeingWarpedOut = false;
        Target->TemporalTargetingMe = nullptr;
        Target = nullptr;
        if (PrevTemporal) {
            if (PrevTemporal->NextTemporal == this) PrevTemporal->NextTemporal = nullptr;
            PrevTemporal->Detach();
        }
        if (NextTemporal) {
            if (NextTemporal->PrevTemporal == this) NextTemporal->PrevTemporal = nullptr;
            NextTemporal->Detach();
        }
        PrevTemporal = NextTemporal = nullptr;
        SourceSW = nullptr;
        unknown_pointer_38 = nullptr;
        if (Owner) Owner->EnterIdleMode(false, 1);
    } catch (...) {
        std::abort();
    }
}
void TemporalClass::UnlinkPointer(AbstractClass* object) try {
    if (Owner == object) {
        if (Target) {
            LetGo();
            Target = nullptr;
        }
    } else if (Target == object) {
        Target = nullptr;
        NextTemporal = PrevTemporal = nullptr;
        SourceSW = nullptr;
        unknown_pointer_38 = nullptr;
        if (Owner) Owner->EnterIdleMode(false, 1);
    }
} catch (...) {
    std::abort();
}
#endif
