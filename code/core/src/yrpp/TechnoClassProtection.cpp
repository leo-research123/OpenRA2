// YR 0x0070E2B0: shared iron-curtain/force-shield application.
#include "yrpp/TechnoClass.h"
#if !defined(RA2_YRPP_GAME)
DamageState TechnoClass::IronCurtain(int duration, HouseClass*, bool forceShield) {
    IronCurtainTimer.Start(duration);
    IronTintStage = 0;
    ForceShielded = forceShield ? 1 : 0;
    return DamageState::Unaffected;
}
#endif
