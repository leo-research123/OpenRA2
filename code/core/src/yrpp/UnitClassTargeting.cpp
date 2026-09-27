// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 unit.cpp Greatest_Threat; YR 0x743190 adds the active
// multi-turret weapon branch. EA Section 7: third_party/opents/LICENSE.md.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/WeaponTypeClass.h"

AbstractClass* UnitClass::GreatestThreat(ThreatType threat,CoordStruct* origin,bool onlyEnemy) {
    if(Owner->IsControlledByHuman()&&Type->DeployToFire)return nullptr;
    unsigned flags=unsigned(threat);
    if(!(flags&0x1B978u)) {
        if(Type->HasMultipleTurrets()&&!Type->IsGattling)
            flags|=unsigned(GetWeapon(CurrentWeaponNumber)->WeaponType->AllowedThreats());
        else for(int index=0;index<2;++index)
            if(auto* weapon=GetWeapon(index)->WeaponType)flags|=unsigned(weapon->AllowedThreats());
    }
    return FootClass::GreatestThreat(ThreatType(flags),origin,onlyEnemy);
}
