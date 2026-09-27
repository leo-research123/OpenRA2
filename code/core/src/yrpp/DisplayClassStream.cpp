// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 display.cpp Load/Save, calibrated to YR 0x004AE6F0/0x004AE720.
// Unlike OpenTS, YR Load continues after errors and returns the last layer's HR.
// Additional terms: third_party/opents/LICENSE.md.
#include "yrpp/DisplayClass.h"

#if !defined(RA2_YRPP_GAME)
HRESULT DisplayClass::Load(IStream* stream) {
    HRESULT result=0;
    for(auto& layer:ObjectsInLayers) result=layer.Load(stream);
    return result;
}
HRESULT DisplayClass::Save(IStream* stream) {
    HRESULT result=0;
    for(auto& layer:ObjectsInLayers) {
        result=layer.Save(stream);
        if(result<0) break;
    }
    return result;
}
#endif
