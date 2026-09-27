// EA f1f0d42b REDALERT/MAP.CPP cell ownership, adapted to YRpp 9402d7da.
// Copyright 2020 Electronic Arts Inc.; GPL-3.0 + third_party/ea/LICENSE.TXT terms.
// YR 565090, 565AA0..565C10, 5662AC..56645C and 567110 calibrate the fresh map.
#include "yrpp/MapClass.h"
#include "yrpp/AStarClass.h"
#include "map_hash.hpp"
#include "map_runtime.hpp"
#include <algorithm>
#include <cstdlib>

MapClass::MapClass()
    : GScreenClass(), unknown_10{}, unknown_pointer_14{}, MovementZones{}, somecount_4C{},
      ZoneConnections{}, LevelAndPassability{}, ValidMapCellCount{}, LevelAndPassabilityStruct2pointer_70{},
      SubzoneTrackingCounts{}, unknown_80{},
      SubzoneTracking{}, CellStructs1{}, MapRect{}, VisibleRect{},
      CellIterator_NextX{}, CellIterator_NextY{}, CellIterator_CurrentY{}, CellIterator_NextCell{},
      ZoneIterator_X{}, ZoneIterator_Y{}, MapCoordBounds{}, TotalValue{},
      Cells{}, MaxLevel{}, MaxWidth{}, MaxHeight{},
      MaxNumCells{}, Crates{}, Redraws{}, TaggedCells{} { MaxLevel = 13; }
MapClass::~MapClass() {
    ReleaseCellStorage();
    game::destroy_map_hash(unknown_pointer_14);
    for (auto* table : unknown_80) game::destroy_map_hash(table);
}

void MapClass::ReleaseCellStorage() noexcept {
    // Break bridge dependencies first, matching 565B00's two passes.
    if (Cells.Items) {
        for (int index = 0; index < Cells.Capacity; ++index) {
            if (auto* cell = Cells.Items[index]) {
                cell->BridgeOwnerCell = nullptr;
                cell->unknown_30 = 0;
            }
        }
        for (int index = 0; index < Cells.Capacity; ++index) {
            auto* cell = Cells.Items[index];
            Cells.Items[index] = nullptr;
            GameDelete(cell);
        }
    }
    Cells.Clear(); Cells.IsInitialized = true;
    YRMemory::Deallocate(LevelAndPassability); LevelAndPassability = nullptr;
    YRMemory::Deallocate(LevelAndPassabilityStruct2pointer_70); LevelAndPassabilityStruct2pointer_70 = nullptr;
    ValidMapCellCount = 0;
    CellIterator_NextCell = nullptr;
    CellIterator_NextX = CellIterator_NextY = CellIterator_CurrentY = 0;
    MapRect = {}; VisibleRect = {}; MapCoordBounds = {};
    TotalValue = 0;
    ZoneConnections.Clear(); TaggedCells.Clear(); CellStructs1.Clear();
    for (auto& tracking : SubzoneTracking) tracking.Clear();
    for (auto& zones : MovementZones) { YRMemory::Deallocate(zones); zones = nullptr; }
    somecount_4C = 0;
    std::fill_n(SubzoneTrackingCounts, 3, 0);
    if (unknown_pointer_14) for (int i = 0; i < unknown_pointer_14->BucketCount; ++i) unknown_pointer_14->Buckets[i].Clear();
    for (auto* hash : unknown_80) if (hash) for (int i = 0; i < hash->BucketCount; ++i) hash->Buckets[i].Clear();
}
bool MapClass::AllocateCellStorage() noexcept {
    if (Cells.Items && Cells.Capacity == MaxCells) return true;
    ReleaseCellStorage();
    try {
        if (!Cells.SetCapacity(MaxCells)) return false;
        std::fill_n(Cells.Items, MaxCells, nullptr);
        MaxWidth = MaxHeight = 512; MaxNumCells = MaxCells;
        return true;
    } catch (...) { ReleaseCellStorage(); return false; }
}
void MapClass::AllocateCells() {
    // 565AA0 is the One_Time allocation after MaxNumCells is configured.
    // Its caller must not discard live Cells by reconstructing their table.
    for (int i = 0; i < Cells.Capacity; ++i) if (Cells.Items[i]) std::abort();
    Cells.Clear(); Cells.IsInitialized = true;
    if (MaxNumCells < 0 || !Cells.SetCapacity(MaxNumCells)) std::abort();
    if (MaxNumCells) std::fill_n(Cells.Items, MaxNumCells, nullptr);
}
void MapClass::DestructCells() {
    // 565B00 retains the table, geometry and navigation storage. World shutdown
    // has different ownership semantics and uses ReleaseCellStorage instead.
    for (int i = 0; i < Cells.Capacity; ++i) if (auto* cell = Cells.Items[i]) {
        cell->BridgeOwnerCell = nullptr; cell->unknown_30 = 0;
    }
    for (int i = 0; i < Cells.Capacity; ++i) {
        auto* cell = Cells.Items[i]; Cells.Items[i] = nullptr; GameDelete(cell);
    }
    if (auto release = game::map_runtime().release_cell_lighting) release();
    if (Cells.Capacity < MaxCells) {
        if (!Cells.SetCapacity(MaxCells)) std::abort();
        std::fill_n(Cells.Items, Cells.Capacity, nullptr);
    }
}
void MapClass::ConstructCells() {
    TotalValue = 0;
    // 565BC0 reconstructs previously cleared cell slots, without a second
    // destructor pass. Callers must already have released per-cell resources.
    for (int index = 0; index < Cells.Capacity && index < MaxCells; ++index) {
        if (auto* cell = Cells.Items[index]) {
            ::new (cell) CellClass();
        }
    }
}

bool MapClass::CreateEmptyCells(const RectangleStruct& bounds, char level) noexcept {
    // The fresh-map path is independent of game objects, pathfinding services
    // and the live-world resize branch of CreateEmptyMap. It owns real Cells.
    ReleaseCellStorage();
    const int width = bounds.Width, height = bounds.Height;
    if (width < 2 || height < 1 || width > 511 || height > 510 || width + height > 512 ||
        static_cast<signed char>(level) < 0 || level > MaxLevel) return false;
    if (!AllocateCellStorage()) return false;
    try {
        MapRect = {0, 0, width, height};
        MapCoordBounds = {1, 1, width + height - 1, width + height - 1};
        // Original traversal allocates valid diamond cells in row-major slot
        // order. Coordinates outside the diamond remain null in the full table.
        for (int y = 0; y < 512; ++y) for (int x = 0; x < 512; ++x) {
            if (x + y <= width || x - y >= width || y - x >= width || x + y > width + 2 * height) continue;
            auto* cell = CellClass::Create();
            if (!cell) { ReleaseCellStorage(); return false; }
            cell->MapCoords = {static_cast<short>(x), static_cast<short>(y)};
            cell->Level = level;
            Cells.Items[y * 512 + x] = cell;
        }
        // This field names the square passability buffer capacity in 567110,
        // not the number of live diamond cells.
        ValidMapCellCount = (width + height + 1) * (width + height + 1);
        LevelAndPassability = static_cast<CellLevelPassabilityStruct*>(YRMemory::Allocate(
            sizeof(CellLevelPassabilityStruct) * ValidMapCellCount));
        LevelAndPassabilityStruct2pointer_70 = static_cast<LevelAndPassabilityStruct2*>(YRMemory::Allocate(
            sizeof(LevelAndPassabilityStruct2) * ValidMapCellCount));
        if (!LevelAndPassability || !LevelAndPassabilityStruct2pointer_70) {
            ReleaseCellStorage(); return false;
        }
        std::fill_n(LevelAndPassability, ValidMapCellCount, CellLevelPassabilityStruct{7, 0, 0});
        std::fill_n(LevelAndPassabilityStruct2pointer_70, ValidMapCellCount, LevelAndPassabilityStruct2{});
        for (int i = 0; i < 3; ++i) SubzoneTracking[i].CapacityIncrement = width * height / (1 << (2*i));
        InvalidCell.~CellClass(); ::new (&InvalidCell) CellClass();
        CellIteratorReset();
        AStarClass::Instance.UpdateMapDimensions(MapRect);
        Redraws = 1;
        return true;
    } catch (...) { ReleaseCellStorage(); return false; }
}
