// YR 0x00577920: tag cell registrations and a distinct pending-tag list.
#include "yrpp/MapClass.h"
#include "yrpp/TagClass.h"
#if defined(RA2_YRPP_GAME)
DynamicVectorClass<TagClass*>& MapClass::PendingTags = *reinterpret_cast<DynamicVectorClass<TagClass*>*>(0x008B41A8);
#else
namespace { DynamicVectorClass<TagClass*> pending_tags; }
DynamicVectorClass<TagClass*>& MapClass::PendingTags = pending_tags;
#endif
void MapClass::PointerGotInvalid(AbstractClass* object, bool) {
    if (!object || object->WhatAmI() != AbstractType::Tag) return;
    for (int i = 0; i < TaggedCells.Count; ++i) {
        auto* cell = GetCellAt(TaggedCells[i]);
        if (cell->AttachedTag == object) {
            cell->ReplaceTag(nullptr);
            TaggedCells.Remove(cell->MapCoords);
            --i;
        }
    }
    InvalidCell.PointerExpired(object, true);
    PendingTags.Remove(static_cast<TagClass*>(object)); // first only, unlike Logic
}
