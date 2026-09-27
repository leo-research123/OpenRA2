// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Adjust_Threat; YR 0x4FA2E0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include <bit>
#include <cstdlib>

void HouseClass::AdjustThreat(int region,int amount) {
    constexpr int offsets[]{-131,-130,-129,-1,0,1,129,130,131};
    constexpr int shifts[]{2,1,2,1,0,1,2,1,2};
    const bool negative=amount<0;
    const int magnitude=negative?std::bit_cast<int>(0u-static_cast<unsigned>(amount)):amount;
    for(int i=0;i<9;++i) {
        const int index=region+offsets[i];
        if(index<0 || index>=130*130)std::abort(); // corrupted map/region, not another threat model
        auto& value=ThreatPosedEstimates[index/130][index%130];
        const auto delta=static_cast<unsigned>(magnitude>>shifts[i]);
        const auto updated=negative?value-delta:value+delta;
        value=std::bit_cast<int>(updated)<0?0:updated;
    }
}
