// OpenTS 44fac744 suprtype.cpp From_Action; YR 0x006CEEB0.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md.
#include "yrpp/SuperWeaponTypeClass.h"
#if !defined(RA2_YRPP_GAME)
SuperWeaponTypeClass* YRPP_FASTCALL SuperWeaponTypeClass::FindFirstOfAction(::Action action) noexcept {
    for(int i=0;i<Array.Count;++i)if(Array[i]->Action==action)return Array[i];
    return nullptr;
}
#endif
