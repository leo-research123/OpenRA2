// Existing YRpp tile catalog and cells, calibrated to 546DA0. Loading-screen
// pumping belongs to the host; ownership and per-variant usage remain original.
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MapClass.h"

bool IsometricTileTypeClass::LoadMapImages(bool all,bool random_map) noexcept {
    try {
        if (!all) {
            auto& map=MapClass::Instance;
            map.CellIteratorReset();
            while (const auto* cell=map.CellIteratorNext()) {
                IsometricTileTypeClass* tile=nullptr; int variant=0;
                if (!cell->GetTerrainTile(tile,variant)) return false;
                while (variant-- && tile) tile=tile->NextVariant;
                if (!tile) return false;
                ++tile->unk_308; // original DWORD usage counter wraps
            }
        }
        for (int i=0;i<Array.Count;++i) for (auto* tile=Array[i];tile;tile=tile->NextVariant) {
            if (!tile->unk_2F4 || !tile->FileName[0]) continue;
            if (all || tile->unk_308 || (random_map && tile->RequiredByRMG)) {
                if (!tile->GetImage()) return false;
            } else if (tile->Image) {
                // Original retains radar colors/dimensions while dropping the
                // image; a later ReadTMP replaces that cache before publishing.
                if (tile->ImageAllocated) YRMemory::Deallocate(tile->Image);
                tile->Image=nullptr; tile->ImageAllocated=false;
            }
        }
        return true;
    } catch (...) { return false; }
}
