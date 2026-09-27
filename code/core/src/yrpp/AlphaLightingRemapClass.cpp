// YRpp AlphaLightingRemapClass 9402d7da.
#include "yrpp/Memory.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include "images/original_abi.hpp"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <bit>
#include <new>

#ifndef RA2_IMAGE_GAME
namespace { DynamicVectorClass<AlphaLightingRemapClass*> remappers; }
DynamicVectorClass<AlphaLightingRemapClass*>& AlphaLightingRemapClass::Array = remappers;
#endif

AlphaLightingRemapClass::AlphaLightingRemapClass(int count) noexcept {
    IntensityCount = count;
    RefCount = 0;
    const auto maximum = std::bit_cast<std::int32_t>(std::uint32_t(count) - 1u);
    for (std::uint32_t i = 0; i < 65536; ++i) {
        const auto product = std::uint32_t(maximum) * (i >> 8) * (i & 255);
        const auto value = std::bit_cast<std::int32_t>(product) / 32258;
        Table[i >> 8][i & 255] = WORD(BYTE(std::min(value, maximum)) << 8);
    }
}
AlphaLightingRemapClass* YRPP_STDCALL AlphaLightingRemapClass::FindOrAllocate(int count) {
    for (int i = 0; i < Array.Count; ++i) {
        auto* current = Array.Items[i];
        if (current->IntensityCount == count) {
            current->RefCount = std::bit_cast<std::int32_t>(std::uint32_t(current->RefCount) + 1u);
            return current;
        }
    }
    void* storage = YRMemory::Allocate(sizeof(AlphaLightingRemapClass));
    auto* item = storage ? new (storage) AlphaLightingRemapClass(count) : nullptr;
    Array.AddItem(item);
    // The original dereferences failed allocation after its attempted append.
    ++item->RefCount;
    return item;
}
void YRPP_STDCALL AlphaLightingRemapClass::Release(AlphaLightingRemapClass* item) {
    if (!item) return;
    item->RefCount = std::bit_cast<std::int32_t>(std::uint32_t(item->RefCount) - 1u);
    if (!item->RefCount) {
        Array.Remove(item);
        YRMemory::Deallocate(item);
    }
}
