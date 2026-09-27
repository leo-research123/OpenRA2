// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: code/third_party/opents/LICENSE.md.
// OpenTS 44fac744 techno.cpp; YR 0x0070E380 / 0x0070E4B0.
#include "yrpp/TechnoClass.h"
#include <algorithm>
#include <bit>
int TechnoClass::GetInvulnerabilityTintIntensity(int intensity) const {
    const int remaining=IronTintTimer.GetTimeLeft();int scale;
    switch(IronTintStage){
        case 1:scale=((12-remaining)*256)/6;break;
        case 2:case 8:scale=512;break;
        case 3:scale=(461*remaining+1020)/20;break;
        case 4:scale=(1024-77*remaining)/8;break;
        case 5:scale=(77*remaining+816)/16;break;
        case 6:scale=51;break;
        case 7:scale=(3072-461*remaining)/6;break;
        case 9:scale=((remaining+20)*256)/20;break;
        default:return intensity;
    }
    return std::min(std::bit_cast<int>(unsigned(intensity)*unsigned(scale))>>8,2000);
}
int TechnoClass::GetAirstrikeTintIntensity(int intensity) const {
    const int remaining=AirstrikeTintTimer.GetTimeLeft();int scale;
    switch(AirstrikeTintStage){
        case 1:case 7:scale=((12-remaining)*256)/6;break;
        case 2:case 8:scale=512;break;
        case 3:case 9:scale=((remaining+20)*256)/20;break;
        case 4:scale=((128-remaining)*256)/64;break;
        case 5:scale=((remaining+64)*256)/64;break;
        case 6:scale=256;break;
        default:return intensity;
    }
    return std::min(std::bit_cast<int>(unsigned(intensity)*unsigned(scale))>>8,2000);
}
