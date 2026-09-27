#pragma once
#include "yrpp/HashTable.h"
#include <memory>
namespace game {
// Original HashTable storage only. Ownership stays with its original class.
template<class Key, class Value>
HashTable<Key,Value>* create_map_hash(DWORD (YRPP_FASTCALL *hash)(const Key&), int growth) noexcept {
    try {
        auto table = std::make_unique<HashTable<Key,Value>>();
        auto buckets = std::make_unique<DynamicVectorClass<HashObject<Key,Value>>[]>(256);
        for (int i = 0; i < 256; ++i) buckets[i].CapacityIncrement = growth;
        table->Buckets = buckets.release();
        table->BucketHashFunction = hash;
        table->BucketCount = 256;
        table->BucketGrowthStep = growth;
        return table.release();
    } catch (...) { return nullptr; }
}
template<class Key, class Value>
void destroy_map_hash(HashTable<Key,Value>* table) noexcept {
    if (!table) return;
    delete[] table->Buckets;
    delete table;
}
}

