#pragma once
#include "yrpp/platform/ABI.h"
#include "yrpp/ArrayClasses.h"

template<typename key_type, typename value_type>
struct HashObject
{
    key_type Key;
    value_type Value;
    bool operator==(const HashObject&) const = default;
};

template<typename key_type, typename value_type>
struct HashTable
{
    DynamicVectorClass<HashObject<key_type,value_type>> * Buckets;
    DWORD (YRPP_FASTCALL *BucketHashFunction)(const key_type&);
    int BucketCount;
    int BucketGrowthStep;
};

struct HashIterator
{
    int BucketIndex;
    int InBucketIndex;
    bool OutOfBuckets;
};
