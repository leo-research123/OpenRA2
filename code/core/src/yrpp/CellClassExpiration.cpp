// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 cell.cpp::Detach, YR 0x00485130 / 0x00485250.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/CellClass.h"
#include "yrpp/FoggedObjectClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TagClass.h"

void CellClass::ReplaceTag(TagClass* tag) {
    if (AttachedTag) --AttachedTag->InstanceCount;
    AttachedTag = tag;
    if (tag) {
        // Original still increments the count if appending the cell fails.
        MapClass::Instance.TaggedCells.AddItem(MapCoords);
        ++tag->InstanceCount;
    }
}
void CellClass::PointerExpired(AbstractClass* object, bool) {
    if (AttachedTag == object) {
        if (AttachedTag) --AttachedTag->InstanceCount;
        AttachedTag = nullptr;
        MapClass::Instance.TaggedCells.Remove(MapCoords);
    }
    if (BridgeOwnerCell == object) BridgeOwnerCell = nullptr;
    if (unknown_30 == object) unknown_30 = nullptr;
    if (object && FoggedObjects && object->WhatAmI() == AbstractType::FoggedObject)
        FoggedObjects->Remove(static_cast<FoggedObjectClass*>(object));
}
