#pragma once
#include "yrpp/GeneralStructures.h"
#include <cstdint>
class FootClass;
class PathFinderData;
namespace game {
inline constexpr int path_snapshot_cells = 4096;
inline constexpr unsigned path_snapshot_node_budget = 2048;
struct PathSnapshotCounters {
    std::uint64_t profile = 0, extent = 0, zone = 0, terrain = 0, destination = 0;
};
void read_path_snapshot_counters(PathSnapshotCounters&) noexcept;
// Internal, pointer-free worker input. Only the simulation thread captures it.
struct PathSnapshot {
    CellStruct origin{}, start{}, destination{};
    int width = 0, height = 0, level = 0;
    unsigned costs[path_snapshot_cells]{};
};
struct PathSnapshotResult {
    int directions[24]{};
    int length = 0;
    unsigned expanded = 0;
    bool found = false;
};
bool capture_path_snapshot(FootClass&, CellStruct, int offset, int mode, PathSnapshot&) noexcept;
void solve_path_snapshot(const PathSnapshot&, PathSnapshotResult&) noexcept;
// A worker only receives snapshots and disjoint result slots. Completion order
// never controls application order or the request budget.
void solve_path_snapshot_batch(const PathSnapshot*, PathSnapshotResult*, unsigned count, unsigned workers) noexcept;
void activate_path_snapshot(FootClass*, const PathSnapshot*, const PathSnapshotResult*) noexcept;
#if defined(RA2_YRPP_GAME) || defined(RA2_IMAGE_GAME)
inline bool consume_path_snapshot(FootClass&, CellStruct, CellStruct, int*, PathFinderData*&) noexcept { return false; }
#else
bool consume_path_snapshot(FootClass&, CellStruct start, CellStruct end, int* directions,
                           PathFinderData*& result) noexcept;
#endif
std::uint64_t path_snapshots_consumed() noexcept;
}
