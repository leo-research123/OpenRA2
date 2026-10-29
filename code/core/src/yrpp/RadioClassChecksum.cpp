// YR 0x0065AB10: radio slot capacity and non-null contacts, in slot order.
#include "yrpp/RadioClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/CRC.h"
#if !defined(RA2_YRPP_GAME)
void RadioClass::ComputeCRC(CRCEngine& crc) const {
    MissionClass::ComputeCRC(crc);
    crc(RadioLinks.Capacity);
    for (int i = 0; i < RadioLinks.Capacity; ++i)
        if (auto* link = RadioLinks[i]) {
            crc(link->Fetch_ID());
            crc(int(link->WhatAmI()));
        }
}
#endif
