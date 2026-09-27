// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 logic.cpp::Detach, YR 0x0055B880; original worklist 0x008B40C8.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/TagClass.h"
#if defined(RA2_YRPP_GAME)
DynamicVectorClass<TagClass*>& LogicClass::PendingTags = *reinterpret_cast<DynamicVectorClass<TagClass*>*>(0x008B40C8);
#else
namespace { DynamicVectorClass<TagClass*> pending_tags; }
DynamicVectorClass<TagClass*>& LogicClass::PendingTags = pending_tags;
#endif
void LogicClass::PointerGotInvalid(AbstractClass* object, bool) {
    if (object && object->WhatAmI() == AbstractType::Tag)
        while (PendingTags.Remove(static_cast<TagClass*>(object))) {}
}
