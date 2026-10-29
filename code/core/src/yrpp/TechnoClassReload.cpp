// YR 0x6FB080: start the timer without changing the ammunition count.
#include "yrpp/TechnoClass.h"
#include <bit>

void TechnoClass::StartReloading() {
    const auto* type = GetTechnoType();
    if (Ammo >= type->Ammo) return;
    if (!Ammo && type->EmptyReload != -1) {
        ReloadTimer.Start(type->EmptyReload);
        return;
    }
    // PipWrap (+0x3E4) is the integer divisor, not the floating-point Size.
    const int factor = type->PipWrap ? Ammo / type->PipWrap : 1;
    const unsigned duration =
        unsigned(factor) * unsigned(factor) * unsigned(type->ReloadIncrement) + unsigned(type->Reload);
    ReloadTimer.Start(std::bit_cast<int>(duration));
}
