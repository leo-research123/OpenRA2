// SPDX-License-Identifier: GPL-3.0-or-later
// Existing OpenTS baseline license: third_party/opents/LICENSE.md (EA Section 7).
// YR-only disguise presentation, using the existing Techno fields/virtual slots.
// Original 0x70ED80 / 0x70EE30; unlike cloak, fake blinking does not clear Disguised.
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include <bit>

DWORD TechnoClass::GetDisguiseFlags(DWORD flags) const {
    if(DisguiseBlinkTimer.GetTimeLeft() && Owner && !Owner->IsControlledByCurrentPlayer())return flags;
    const int phase=std::bit_cast<int>(unsigned(Unsorted::CurrentFrame)-DisguiseCreationFrame+64u)%256;
    if((phase>=64&&phase<68)||(phase>=76&&phase<80)||(phase>=112&&phase<116)||(phase>=124&&phase<128))return flags|2u;
    if((phase>=68&&phase<76)||(phase>=116&&phase<124))return flags|4u;
    return flags;
}
bool TechnoClass::IsClearlyVisibleTo(HouseClass* house) const {
    const bool local=Owner&&Owner->IsControlledByCurrentPlayer();
    if(DisguiseBlinkTimer.GetTimeLeft() && !local)return true;
    if(local && IsDisguised()) {
        const int phase=std::bit_cast<int>(unsigned(Unsorted::CurrentFrame)-DisguiseCreationFrame+64u)%256;
        return phase<72 || phase>119;
    }
    if(!IsDisguised())return true;
    const auto* cell=GetCell();
    return !cell || (house && cell->DisguiseSensors_InclHouse(house->ArrayIndex));
}
