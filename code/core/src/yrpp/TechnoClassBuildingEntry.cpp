// Original YR 0x70FB50: tank-bunker admission, independent of player input.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"

bool TechnoClass::CanEnterTankBunker() const {
    if (!GetTechnoType()->Bunkerable || !GetTechnoType()->Turret) return false;
    const auto* weapon = GetTurretWeapon();
    if (!weapon || !weapon->WeaponType) return false;
    if (GetTechnoType()->SpeedType == SpeedType::Hover) return false;
    return (AbstractFlags & ::AbstractFlags::Foot) == ::AbstractFlags::None ||
           !static_cast<const FootClass*>(this)->ParasiteEatingMe;
}
