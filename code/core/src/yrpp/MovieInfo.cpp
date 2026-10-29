// Original movie-name registry lookup 48DF30 and exit cleanup 6BE294..6BE31B.
#include "yrpp/Unsorted.h"

int YRPP_FASTCALL MovieInfo::FindIndex(const char* name) {
    if (!name || !_strcmpi(name, "<none>")) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (!_strcmpi(name, Array[i])) return i;
    return -1;
}

void MovieInfo::ClearArray() {
    // Original exit removes the first name each time. Strings and pointer-buffer
    // storage have separate owners and are freed independently.
    while (Array.Count > 0) {
        const char* name = Array[0];
        YRMemory::Deallocate(name);
        Array.RemoveItem(0);
    }
    if (Array.Items && Array.IsAllocated) {
        DLLDeleteArray(Array.Items, static_cast<size_t>(Array.Capacity));
        Array.Items = nullptr;
    }
    Array.Count = 0;
    Array.IsAllocated = false;
    Array.Capacity = 0;
}
