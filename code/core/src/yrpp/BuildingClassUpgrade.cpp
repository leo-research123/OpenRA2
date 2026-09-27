// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 building.cpp Can_Upgrade; YR 0x00452670.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md.
#include "yrpp/BuildingClass.h"
#if !defined(RA2_YRPP_GAME)
bool BuildingClass::CanUpgrade(const BuildingTypeClass* upgrade,const HouseClass* owner) const noexcept {
    if(owner!=Owner || _strcmpi(upgrade->PowersUpBuilding,Type->ID))return false;
    const int level=upgrade->PowersUpToLevel;
    if(level==-1) {
        if(static_cast<signed char>(UpgradeLevel)<Type->Upgrades)return true;
    } else if(level<=0 || level>3)return false;
    return UpgradeLevel==0;
}
#endif
