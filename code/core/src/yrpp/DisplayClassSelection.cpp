// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 display.cpp: Bandbox_Selection_Callback; YR 0x4AC2B0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/DisplayClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Unsorted.h"

void YRPP_FASTCALL DisplayClass::BandboxSelectionCallback(ObjectClass* object) noexcept {
    auto* owner=object->GetOwningHouse();
    if(owner && owner->IsControlledByCurrentPlayer() && object->CanBeSelected()
        && object->CanBeSelectedNow() && !object->InLimbo && object->Select())Unsorted::MoveFeedback=false;
}
