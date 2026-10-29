// OpenTS 44fac744 suprtype.cpp From_Action; YR 0x006CEEB0.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md.
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#if !defined(RA2_YRPP_GAME)
::Action SuperWeaponTypeClass::MouseOverObject(const CellStruct&, ObjectClass* object) const {
    if (Type != SuperWeaponType::ForceShield) return Action;
    if (object && object->WhatAmI() == AbstractType::Building && object->WhatAmI() == AbstractType::Building &&
        static_cast<BuildingClass*>(object)->Owner->IsAlliedWith(HouseClass::CurrentPlayer))
        return Action;
    return ::Action::NoForceShield;
}
SuperWeaponTypeClass* YRPP_FASTCALL SuperWeaponTypeClass::FindFirstOfAction(::Action action) noexcept {
    for (int i = 0; i < Array.Count; ++i)
        if (Array[i]->Action == action) return Array[i];
    return nullptr;
}
#endif
