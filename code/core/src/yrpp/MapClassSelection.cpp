// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 conquer.cpp Unselect_All; YR 0x0048DC90 adds selection
// command reset and clears all beacon selection bits.
#include "yrpp/MapClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/BeaconClass.h"
#include "yrpp/Unsorted.h"
#if !defined(RA2_YRPP_GAME)
void YRPP_FASTCALL MapClass::UnselectAll() {
    while(ObjectClass::CurrentObjects.Count)ObjectClass::CurrentObjects[0]->Deselect();
    Game::SetSelectionCommandMode(0);
    if(BeaconClass::Count)for(auto& house:BeaconClass::Array)for(auto* beacon:house)
        if(beacon)beacon->Bitfield&=static_cast<BYTE>(~2u);
}
#endif
