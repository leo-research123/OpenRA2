#pragma once
// Per-house sight count grid.
//
// Every house owns one uint16 plane with one entry per map cell: the number of its sight sources
// that currently cover the cell. A moving unit applies -1 to its old sight set and +1 to its new
// one; overlapping cells cancel and only cells whose count crosses zero reach the local player's
// view (cell flags, shroud counter, radar, discovery, background invalidation). Draw-frame caches
// of changed cells and their neighbours are refreshed once per logic frame.
//
// Everything here is integer-only, single-threaded and processed in a fixed order, so planes are
// bit-identical on every client that runs the same commands. Internal to the core.
#include "yrpp/GeneralStructures.h"
#include <cstdint>

class HouseClass;

namespace game {
// Geometry of one sight circle in map cells (the inputs of the original Sight_From scan).
struct SightSet {
    short cx, cy;    // projected centre cell
    short hx, hy;    // RevealByHeight raise offset
    int level;       // Z / LevelHeight
    int radius;      // 1..10; 0 means the set is empty
    bool by_height;  // apply the RevealByHeight terrain test
};
// Original occlusion offsets (0x005638D0), indexed like CellSpread::GetCell.
const CellStruct* sight_occlusion_offsets() noexcept;
// Houses whose sight the local player sees: itself, radar-shared and allied (AllyReveal) houses.
HouseClass* sight_owner(HouseClass* house) noexcept;
bool sight_special_multiplayer() noexcept;
bool sight_reserved_cell(int x, int y) noexcept;
bool sight_in_map(int x, int y) noexcept;

// Applies one acquisition (release == false) or release of `set` to the owner's plane. Inside a SightMoveScope a
// release is held back and netted against the next acquisition, which is how a unit's paired sight update avoids
// touching the cells both circles share; outside a scope it takes effect at once.
void visibility_sight_apply(HouseClass* owner, const SightSet& set, bool release) noexcept;
void visibility_scope_enter() noexcept;
void visibility_scope_leave() noexcept;
// Wraps the release and re-acquisition of one source (cell arrival of a moving object).
class SightMoveScope {
public:
    SightMoveScope() noexcept { visibility_scope_enter(); }
    ~SightMoveScope() { visibility_scope_leave(); }
    SightMoveScope(const SightMoveScope&) = delete;
    SightMoveScope& operator=(const SightMoveScope&) = delete;
};
// Commits a held-back release. Required before anything reads or rewrites cell sight flags.
void visibility_flush() noexcept;
// End of a logic frame: commits pending work and refreshes the draw-frame caches of dirty cells.
void visibility_frame_end() noexcept;
// After a bulk shroud rewrite: reveals every unsettled cell that some sight source of the local view still
// covers (sources the bulk operation did not withdraw and restore, such as allied units).
void visibility_reconcile() noexcept;
// A cell's occlusion inputs changed; its draw-frame cache and its neighbours' are refreshed at frame end.
void visibility_cache_dirty(CellStruct cell) noexcept;
// Marks every cell's draw-frame cache stale (bulk shroud rewrites).
void visibility_cache_all_dirty() noexcept;
// Cell terrain levels changed; the flat-terrain summary is rebuilt on next use.
void visibility_terrain_changed() noexcept;
// Zeroes every plane (cells reconstructed) or frees them (cell storage released).
void visibility_reset(bool release_storage) noexcept;
// Hash over all planes in house and cell order, for determinism checks.
std::uint64_t visibility_grid_hash() noexcept;
// Raw count of one house plane at a cell; 0 when the plane does not exist.
unsigned visibility_count(int house_index, int x, int y) noexcept;
// Counters since map load: applied sets, sets needing the per-cell terrain test, view crossings up and down,
// pending releases netted into an acquisition. For diagnostics only.
struct VisibilityStats {
    std::uint64_t sets, rough_sets, crossings_up, crossings_down, netted;
};
VisibilityStats visibility_stats() noexcept;
// Cells of a set in row order, for verification against the planes. Returns the cell count.
int visibility_set_cells(const SightSet& set, CellStruct* out, int capacity) noexcept;
}  // namespace game
