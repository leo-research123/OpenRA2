#pragma once
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
namespace game {
// The first test of TacticalClass::GetOcclusion (YR 0x6D8700): a cell whose own shroud (or fog) bit is clear
// is classified from its own flags alone and the eight neighbors are not read. Returns false when the neighbors
// decide, then `result` is untouched. Shared by GetOcclusion and the per-view shroud cycle, which already
// holds the cell and calls the query for the cells that need their neighbors.
inline bool occlusion_without_neighbors(const CellClass& cell, bool fog, char& result) noexcept {
    const unsigned flags = fog ? static_cast<unsigned>(cell.Flags) : static_cast<unsigned>(cell.AltFlags);
    const unsigned mask = fog ? 2u : 8u;
    if (flags & mask) return false;
    result = (flags & (fog ? 3u : 0x18u)) ? char(-1) : char(-2);
    return true;
}
// MapClass::TryGetCellAt without the calls (index Y * 512 + X as in MapClass::GetCellIndex): the entry of the
// cell table, or null. The occlusion query reads nine cells per cell.
inline CellClass* try_table_cell(const MapClass& map, const CellStruct& at) noexcept {
    const int index = at.Y * 512 + at.X;
    if (map.Cells.Items && index >= 0 && index < MapClass::MaxCells && index < map.Cells.Capacity)
        return map.Cells.Items[index];
    return nullptr;
}
// MapClass::GetCellAt: the original call decides only when the table has no cell, so the invalid-cell result and
// its side effect are unchanged.
inline CellClass* table_cell(const MapClass& map, const CellStruct& at) {
    if (auto* cell = try_table_cell(map, at)) return cell;
    return map.GetCellAt(at);
}
}
