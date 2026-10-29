// YR 0x00545000: terrain light converter reset, run by the theater phase of
// 0x00545150 before any cell borrows a converter.
#include "yrpp/ConvertClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/MapClass.h"
#include "yrpp/Memory.h"
#include <new>

#if !defined(RA2_YRPP_GAME)
bool LightConvertClass::InitTerrainConverters() noexcept {
    // The original deletes from the front and shifts the remainder down.
    while (Array.Count > 0) {
        auto* item = Array.Items[0];
        Array.RemoveItem(0);
        if (item) {
            item->~LightConvertClass();
            YRMemory::Deallocate(item);
        }
    }
    // The original stores a failed allocation as a null Array[0]; every later
    // reader dereferences it, so a failure leaves the registry empty instead.
    auto* neutral = static_cast<LightConvertClass*>(YRMemory::Allocate(sizeof(LightConvertClass)));
    if (!neutral) return false;
    try {
        // Native hosts have no Primary surface; converters use 16-bit pixels.
        new (neutral) LightConvertClass(&FileSystem::ISOx_PAL, &FileSystem::TEMPERAT_PAL, 2, 1000, 1000, 1000, false,
                                        nullptr, 53);
    } catch (...) {
        YRMemory::Deallocate(neutral);
        return false;
    }
    if (!Array.AddItem(neutral)) {
        neutral->~LightConvertClass();
        YRMemory::Deallocate(neutral);
        return false;
    }
    auto& invalid = MapClass::InvalidCell;
    invalid.LightConvert = nullptr;
    invalid.InitLightConvert(nullptr, 0x10000, 0, 1000, 1000, 1000);
    // Cells forget their borrowed converter without a reference update.
    auto& map = MapClass::Instance;
    map.CellIteratorReset();
    while (auto* cell = map.CellIteratorNext())
        cell->LightConvert = nullptr;
    return true;
}
#endif
