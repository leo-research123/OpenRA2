// YRpp type-array lookup, with the fixed YR 422B20 null/none guard.
#include "yrpp/AnimTypeClass.h"

AnimTypeClass* YRPP_FASTCALL AnimTypeClass::Find(const char* id) {
    if (!id) return nullptr;
    for (auto* item : Array)
        if (!_strcmpi(item->ID, id)) return item;
    return nullptr;
}

int YRPP_FASTCALL AnimTypeClass::FindIndex(const char* id) {
    if (!id || !_strcmpi(id, "<none>")) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (!_strcmpi(Array[i]->ID, id)) return i;
    return -1;
}
