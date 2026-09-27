// Original 0x0071AB60; LetGo remains the separate temporal release dependency.
#include "yrpp/TemporalClass.h"
#include "yrpp/TechnoClass.h"
void TemporalClass::UnlinkPointer(AbstractClass* object) {
    if (Owner == object) {
        if (Target) { LetGo(); Target = nullptr; }
    } else if (Target == object) {
        Target = nullptr;
        WarpRemaining = WarpPerStep = 0;
        NextTemporal = PrevTemporal = nullptr;
        if (Owner) Owner->EnterIdleMode(false, 1);
    }
}
