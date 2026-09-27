#pragma once
// Internal algorithms over original class registries; no parallel object model.
#include "yrpp/AbstractTypeClass.h"
#include <new>
namespace game {
template<class T> int find_type_index(const DynamicVectorClass<T*>& array, const char* id) noexcept {
    if (!id) return -1;
    for (int i = 0; i < array.Count; ++i)
        if (array[i] && !_strcmpi(array[i]->ID, id)) return i;
    return -1;
}
template<class T> T* find_type(const DynamicVectorClass<T*>& array, const char* id) noexcept {
    const int index = find_type_index(array, id);
    return index < 0 ? nullptr : array[index];
}
template<class T> T* allocate_type(const char* id) {
    if (!id || !_strcmpi(id, "none") || !_strcmpi(id, "<none>")) return nullptr;
    if (auto* found = T::Find(id)) return found;
    void* memory = YRMemory::Allocate(sizeof(T));
    if (!memory) return nullptr;
    try { return new (memory) T(id); }
    catch (...) { YRMemory::Deallocate(memory); throw; }
}
}
