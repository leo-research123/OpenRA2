// YR 0x004566B0/0x00456750: building range and concentric range animation.
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WeaponTypeClass.h"
#include <cstdlib>
#include "x87_integer.hpp"
#if !defined(RA2_YRPP_GAME)
int BuildingClass::GetRadialIndicatorRange() const {
    if (Type->PsychicDetectionRadius > 0) return Type->PsychicDetectionRadius;
    if (Type->GapGenerator)
        return static_cast<signed char>(GapSuperCharged ? Type->SuperGapRadiusInCells : Type->GapRadiusInCells);
    if (Type->SensorArray || Type->CloakGenerator) return static_cast<signed char>(Type->CloakRadiusInCells);
    const auto* weapon = TechnoClass::GetTurretWeapon();
    return weapon && weapon->WeaponType && weapon->WeaponType->Range > 0 ? weapon->WeaponType->Range / 256 : 0;
}
void BuildingClass::DrawRadialIndicator(DWORD drawLine) { std::abort(); }
#endif
