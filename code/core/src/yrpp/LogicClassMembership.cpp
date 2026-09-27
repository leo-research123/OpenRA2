// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 logic.cpp::Submit / Remove; YR 0x0055BAA0 / 0x0055BAE0.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/ObjectClass.h"
bool LogicClass::AddObject(ObjectClass* object,bool sorted) {
    if(!object)return false;
    if(object->IsInLogic)return true;
    if(!LayerClass::AddObject(object,sorted))return false;
    object->IsInLogic=true;return true;
}
void LogicClass::RemoveObject(ObjectClass* object) {
    if(object && object->IsInLogic) {
        Remove(object); // first occurrence only; flag clears even if not found
        object->IsInLogic=false;
    }
}
