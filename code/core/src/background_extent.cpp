#include "background_invalidation.hpp"

namespace game {
namespace {
bool extent_ready = false;
}
void reset_background_invalidation_extent() noexcept {
    extent_ready = false;
    set_background_invalidation_extent(-1, -1);
}
void prepare_background_invalidation_extent() noexcept {
    if (extent_ready) return;
    // A dirty rectangle names where something changed; held background items
    // are recorded per cell and composited every frame, so only cells whose
    // own drawing changed need recording again, not every cell whose art
    // overlaps the rectangle. Those cells lie under the rectangle at their
    // raised positions. Raised cells and bridge decks draw above their
    // unraised position; the original background passes visit at most 20
    // half-rows past an area, from 3 rows below it (TacticalClass::
    // DrawTileShadows, 0x6D3040), which bounds that distance: allow 24 half-rows
    // of 15 pixels. One more cell on every side covers a drawing that reads its
    // neighbours. The notification audit reports any cell this misses.
    set_background_invalidation_extent(60, 30 + 24 * 15);
    extent_ready = true;
}
}
