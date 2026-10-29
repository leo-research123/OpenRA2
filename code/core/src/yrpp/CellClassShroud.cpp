// YR sight reference-count additions to OpenTS code/cell.cpp.
// 0x00487630 / 0x00487690 / 0x004876F0. Preserve wrapping int arithmetic.
#include "background_invalidation.hpp"
#include "visibility_grid.hpp"
#include "yrpp/CellClass.h"
#include <algorithm>
#include <bit>
void CellClass::ReduceShroudCounter() {
    if (ShroudCounter == 1) ShroudCounter = 0;
    ShroudCounter = std::bit_cast<int>(unsigned(ShroudCounter) - 1u);
    if ((AltFlags & AltCellFlags::Clear) == AltCellFlags::Clear) {
        if (ShroudCounter <= 0) Flags &= ~CellFlags::FlagToShroud;
    } else if (ShroudCounter <= 0) {
        AltFlags |= AltCellFlags::Clear;
        // Notify the host only when occlusion inputs change, including offscreen cells.
        game::invalidate_background_shroud(MapCoords);
        game::visibility_cache_dirty(MapCoords);
    }
}
void CellClass::IncreaseShroudCounter() {
    const int previous = ShroudCounter;
    if (ShroudCounter == -1) ShroudCounter = 0;
    ShroudCounter =
        std::min(std::bit_cast<int>(unsigned(ShroudCounter) + 1u), std::bit_cast<int>(GapsCoveringThisCell));
    if (previous <= 0 && ShroudCounter > 0) Flags |= CellFlags::FlagToShroud;
}
void CellClass::Unshroud() {
    const auto previous = AltFlags;
    AltFlags |= AltCellFlags::Clear;
    if (ShroudCounter > 0) Flags |= CellFlags::FlagToShroud;
    if (AltFlags != previous) {
        game::invalidate_background_shroud(MapCoords);
        game::visibility_cache_dirty(MapCoords);
    }
}
