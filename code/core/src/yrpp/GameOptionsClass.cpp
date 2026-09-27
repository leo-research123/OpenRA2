// GetAnimSpeed, YR 0x005FB2E0. Small rates: data at 0x00832D0C.
// EA REDALERT/OPTIONS.CPP (f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae)
// supplies the Options normalization design; YR uses this different table.
// Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with additional terms;
// see third_party/ea/LICENSE.TXT.
#include "yrpp/GameOptionsClass.h"
#include <climits>
int GameOptionsClass::GetAnimSpeed(int rate) {
    if (rate<=0||rate>INT_MAX/8||GameSpeed<0||GameSpeed>7) return 0;
    if (rate >= 5) return (8 * rate) / (GameSpeed + 1);
    constexpr int speeds[4][8] = {
        {2,2,1,1,1,1,1,1}, {3,3,3,2,2,2,1,1},
        {5,4,4,3,3,2,2,1}, {7,6,5,4,4,4,3,2}
    };
    // Invalid INI/options are rejected by the host before creating animations.
    return rate > 0 && GameSpeed >= 0 && GameSpeed < 8 ? speeds[rate-1][GameSpeed] : 0;
}
