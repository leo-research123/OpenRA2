// Per-house sight count grid; see visibility_grid.hpp. Replaces the per-step full-circle reveal of the
// original paired sight update (YR 0x005673A0 / 0x005678E0) with net count changes.
#include "visibility_grid.hpp"
#include "yrpp/CellSpread.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <new>

namespace game {
namespace {
// 309 entries match original initializer 0x005638D0 byte-for-byte.
constexpr CellStruct occlusion[]{
    {0, 0},   {-1, 1},  {0, 1},  {1, 1},   {1, 0},   {-1, 0},  {1, -1}, {0, -1},  {-1, -1}, {1, 1},   {0, 1},  {-1, 1},
    {1, 1},   {-1, 1},  {1, 0},  {-1, 0},  {1, -1},  {-1, -1}, {1, -1}, {0, -1},  {-1, -1}, {0, 1},   {0, 1},  {0, 1},
    {1, 1},   {-1, 1},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, -1},  {-1, -1}, {0, -1}, {0, -1},
    {0, -1},  {0, 1},   {0, 1},  {0, 1},   {1, 1},   {1, 1},   {-1, 1}, {-1, 1},  {1, 1},   {-1, 1},  {1, 0},  {-1, 0},
    {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, -1},  {-1, -1}, {1, -1}, {1, -1},  {-1, -1}, {-1, -1}, {0, -1}, {0, -1},
    {0, -1},  {0, 1},   {0, 1},  {0, 1},   {1, 1},   {1, 1},   {-1, 1}, {-1, 1},  {1, 1},   {-1, 1},  {1, 1},  {-1, 1},
    {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, -1}, {-1, -1}, {1, -1},  {-1, -1}, {1, -1}, {1, -1},
    {-1, -1}, {-1, -1}, {0, -1}, {0, -1},  {0, -1},  {0, 1},   {0, 1},  {0, 1},   {1, 1},   {0, 1},   {0, 1},  {-1, 1},
    {1, 1},   {-1, 1},  {1, 1},  {-1, 1},  {1, 0},   {1, 0},   {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},
    {1, 0},   {-1, 0},  {1, -1}, {-1, -1}, {1, -1},  {-1, -1}, {1, -1}, {0, -1},  {0, -1},  {-1, -1}, {0, -1}, {0, -1},
    {0, -1},  {0, 1},   {0, 1},  {0, 1},   {1, 1},   {0, 1},   {0, 1},  {-1, 1},  {1, 1},   {1, 1},   {-1, 1}, {-1, 1},
    {1, 1},   {-1, 1},  {1, 1},  {-1, 1},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},
    {1, 0},   {-1, 0},  {1, -1}, {-1, -1}, {1, -1},  {-1, -1}, {1, -1}, {1, -1},  {-1, -1}, {-1, -1}, {1, -1}, {0, -1},
    {0, -1},  {-1, -1}, {0, -1}, {0, -1},  {0, -1},  {0, 1},   {0, 1},  {0, 1},   {0, 1},   {0, 1},   {0, 1},  {0, 1},
    {1, 1},   {1, 1},   {-1, 1}, {-1, 1},  {1, 1},   {-1, 1},  {1, 1},  {-1, 1},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},
    {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, -1}, {-1, -1},
    {1, -1},  {-1, -1}, {1, -1}, {1, -1},  {-1, -1}, {-1, -1}, {0, -1}, {0, -1},  {0, -1},  {0, -1},  {0, -1}, {0, -1},
    {0, -1},  {0, 1},   {0, 1},  {0, 1},   {0, 1},   {0, 1},   {0, 1},  {0, 1},   {1, 1},   {1, 1},   {-1, 1}, {-1, 1},
    {1, 1},   {-1, 1},  {1, 1},  {-1, 1},  {1, 1},   {-1, 1},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},
    {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, -1},  {-1, -1}, {1, -1}, {-1, -1},
    {1, -1},  {-1, -1}, {1, -1}, {1, -1},  {-1, -1}, {-1, -1}, {0, -1}, {0, -1},  {0, -1},  {0, -1},  {0, -1}, {0, -1},
    {0, -1},  {0, 1},   {0, 1},  {0, 1},   {0, 1},   {0, 1},   {0, 1},  {0, 1},   {1, 1},   {1, 1},   {-1, 1}, {-1, 1},
    {1, 1},   {1, 1},   {-1, 1}, {-1, 1},  {1, 1},   {-1, 1},  {1, 1},  {-1, 1},  {1, 1},   {-1, 1},  {1, 0},  {-1, 0},
    {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},  {1, 0},   {-1, 0},  {1, 0},  {-1, 0},
    {1, -1},  {-1, -1}, {1, -1}, {-1, -1}, {1, -1},  {-1, -1}, {1, -1}, {1, -1},  {-1, -1}, {-1, -1}, {1, -1}, {1, -1},
    {-1, -1}, {-1, -1}, {0, -1}, {0, -1},  {0, -1},  {0, -1},  {0, -1}, {0, -1},  {0, -1},
};

constexpr int Span = 512, CellCount = Span * Span, MaxPlanes = 256, DirtyCapacity = 16384;
constexpr int Window = 21, Centre = 10;
constexpr int BlockShift = 3, BlocksPerRow = Span >> BlockShift;

struct Tables {
    std::uint32_t rows[11][Window];  // geometric mask per radius and dy + 10, bit dx + 10
    short index[Window][Window];     // spiral index per [dy + 10][dx + 10], -1 outside the radius-10 table
};
Tables build_tables() {
    Tables t{};
    for (auto& row : t.index)
        for (auto& value : row)
            value = -1;
    const int total = int(CellSpread::NumCells(10));
    for (int i = 0; i < total; ++i) {
        const auto offset = CellSpread::GetCell(std::size_t(i));
        if (std::abs(offset.X) > Centre || std::abs(offset.Y) > Centre) std::abort();
        auto& slot = t.index[offset.Y + Centre][offset.X + Centre];
        // The original radius-10 table lists each offset once; a repeat would break set membership.
        if (slot != -1) std::abort();
        slot = short(i);
    }
    for (int radius = 0; radius <= 10; ++radius) {
        const int count = int(CellSpread::NumCells(unsigned(radius)));
        for (int i = 0; i < count; ++i) {
            const auto offset = CellSpread::GetCell(std::size_t(i));
            // Original per-cell tests: |dx| <= radius and floor(sqrt(dx^2 + dy^2)) <= radius.
            const int d2 = offset.X * offset.X + offset.Y * offset.Y;
            if (std::abs(offset.X) > radius || d2 >= (radius + 1) * (radius + 1)) continue;
            t.rows[radius][offset.Y + Centre] |= 1u << (offset.X + Centre);
        }
    }
    return t;
}
const Tables& tables() {
    static const Tables built = build_tables();
    return built;
}

// One sight circle as row masks: bit b of row k is cell (x0 + b, y0 + k).
struct Rows {
    int x0 = 0, y0 = 0;
    std::uint32_t m[Window]{};
};

struct State {
    std::uint16_t* planes[MaxPlanes]{};
    std::uint32_t* stamp = nullptr;
    std::uint32_t epoch = 0;
    int* dirty = nullptr;
    int dirty_count = 0;
    bool all_dirty = false;
    signed char* block_level = nullptr;  // max cell level per 8x8 block
    bool block_valid = false;
    struct Pending {
        bool active = false, applies = false;
        HouseClass* owner = nullptr;
        SightSet set{};
    } pending;
};
int scope_depth = 0;
State state;
VisibilityStats stats{};

std::uint16_t* plane_for(int index) {
    if (index < 0 || index >= MaxPlanes) return nullptr;
    auto*& plane = state.planes[index];
    if (!plane) {
        plane = new (std::nothrow) std::uint16_t[CellCount]();
        // A map without its sight planes cannot keep a consistent shroud; stop rather than drift.
        if (!plane) std::abort();
    }
    return plane;
}

void cache_push(int index) {
    if (state.all_dirty) return;
    if (!state.dirty) {
        state.dirty = new (std::nothrow) int[DirtyCapacity];
        if (!state.dirty) {
            state.all_dirty = true;
            return;
        }
    }
    if (state.dirty_count == DirtyCapacity) {
        state.all_dirty = true;
        return;
    }
    state.dirty[state.dirty_count++] = index;
}

void build_blocks() {
    auto& map = MapClass::Instance;
    if (!state.block_level) {
        state.block_level = new (std::nothrow) signed char[BlocksPerRow * BlocksPerRow];
        if (!state.block_level) std::abort();
    }
    // Slots outside the diamond answer with the shared invalid cell, so it bounds every block too.
    const signed char invalid = static_cast<signed char>(MapClass::InvalidCell.Level);
    std::fill_n(state.block_level, BlocksPerRow * BlocksPerRow, invalid);
    if (map.Cells.Items)
        for (int i = 0; i < map.Cells.Capacity && i < CellCount; ++i)
            if (const auto* cell = map.Cells.Items[i]) {
                auto& block = state.block_level[((i / Span) >> BlockShift) * BlocksPerRow + ((i % Span) >> BlockShift)];
                block = std::max(block, static_cast<signed char>(cell->Level));
            }
    state.block_valid = true;
}

// True when no terrain lookup of this set can exceed the RevealByHeight limit.
bool terrain_flat(const SightSet& s) {
    const int lo_x = s.cx - s.radius - s.hx - 1, hi_x = s.cx + s.radius - s.hx + 1;
    const int lo_y = s.cy - s.radius - s.hy - 1, hi_y = s.cy + s.radius - s.hy + 1;
    if (lo_x < 0 || lo_y < 0 || hi_x >= Span || hi_y >= Span) return false;
    if (!state.block_valid) build_blocks();
    const int limit = s.level + 3;
    for (int by = lo_y >> BlockShift; by <= hi_y >> BlockShift; ++by)
        for (int bx = lo_x >> BlockShift; bx <= hi_x >> BlockShift; ++bx)
            if (state.block_level[by * BlocksPerRow + bx] > limit) return false;
    return true;
}

void build_rows(const SightSet& s, Rows& rows) {
    rows = Rows{};
    rows.x0 = s.cx - Centre;
    rows.y0 = s.cy - Centre;
    if (s.radius <= 0) return;
    const auto& t = tables();
    const auto& rect = MapClass::Instance.MapRect;
    const int width = rect.Width, height = rect.Height;
    for (int k = 0; k < Window; ++k) {
        const int dy = k - Centre;
        if (std::abs(dy) > s.radius) continue;
        std::uint32_t mask = t.rows[s.radius][k];
        if (!mask) continue;
        // sight_in_map as an x interval for this row.
        const int y = s.cy + dy;
        const int lo = std::max(width - y + 1, y - width + 1), hi = std::min(width + y - 1, width + 2 * height - y);
        const int first = std::max(0, lo - rows.x0), last = std::min(Window - 1, hi - rows.x0);
        if (first > last) continue;
        mask &= (((1u << (last + 1)) - 1u) & ~((1u << first) - 1u));
        rows.m[k] = mask;
    }
    ++stats.sets;
    if (!s.by_height || terrain_flat(s)) return;
    ++stats.rough_sets;
    const CellStruct* occlusion_table = occlusion;
    const auto& map = MapClass::Instance;
    // Same lookup as MapClass::GetCellAt, including its flattened aliases and the shared invalid cell.
    const int invalid_level = MapClass::InvalidCell.Level;
    for (int k = 0; k < Window; ++k) {
        std::uint32_t bits = rows.m[k];
        while (bits) {
            const int b = std::countr_zero(bits);
            bits &= bits - 1;
            const int i = t.index[k][b];
            const int rx = short(rows.x0 + b - s.hx + occlusion_table[i].X);
            const int ry = short(rows.y0 + k - s.hy + occlusion_table[i].Y);
            const int index = ry * Span + rx;
            const CellClass* cell =
                index >= 0 && index < MapClass::MaxCells && index < map.Cells.Capacity && map.Cells.Items
                    ? map.Cells.Items[index]
                    : nullptr;
            const int level = cell ? int(cell->Level) : invalid_level;
            if (level > s.level + 3) rows.m[k] &= ~(1u << b);
        }
    }
}

struct Crossing {
    short x, y;
    int key;
    bool crossing;  // the local view count of the cell changed between zero and non-zero
};
constexpr int CrossingCapacity = Window * Window;

// Insertion sort: the lists are short and sorting must not allocate.
void sort_by_key(Crossing* items, int count) {
    for (int i = 1; i < count; ++i) {
        const Crossing value = items[i];
        int j = i;
        while (j > 0 && items[j - 1].key > value.key) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = value;
    }
}

struct View {
    HouseClass* current = nullptr;
    bool special = false;
    bool planes_ready = false;
    const std::uint16_t* others[MaxPlanes]{};
    int other_count = 0;
};

void gather_view_planes(View& view, const HouseClass* owner) {
    view.planes_ready = true;
    for (auto* house : HouseClass::Array) {
        if (!house || house == owner) continue;
        const int index = house->ArrayIndex;
        if (index < 0 || index >= MaxPlanes || !state.planes[index]) continue;
        if (sight_owner(house) == view.current) view.others[view.other_count++] = state.planes[index];
    }
}

bool settled(const CellClass& cell) {
    return (cell.Flags & CellFlags::EdgeRevealed) != CellFlags{} &&
           (cell.AltFlags & AltCellFlags::Clear) == AltCellFlags::Clear;
}

// One sight source starts (up) or stops (down) covering a cell of the local view. The reference counter always
// follows the original per-cell reveal. The rest of that reveal is idempotent for a settled cell, so it only runs
// when the cell enters or leaves the view unsettled, or while every frame cache is stale (bulk shroud rewrite).
void view_change(const View& view, const Crossing& c, bool up) {
    ++(up ? stats.crossings_up : stats.crossings_down);
    auto& map = MapClass::Instance;
    auto* cell = map.Cells.Items[c.y * Span + c.x];
    if (!cell) return;
    if (!c.crossing && !state.all_dirty && !view.special && settled(*cell)) {
        // The common case, a settled cell that another source keeps in view: the counter arithmetic of
        // CellClass::ReduceShroudCounter / IncreaseShroudCounter for a cell that already has the Clear flags.
        cell->Flags &= ~CellFlags::IsPlot;
        int counter = cell->ShroudCounter;
        if (up) {
            if (counter == 1) counter = 0;
            counter = std::bit_cast<int>(unsigned(counter) - 1u);
            cell->ShroudCounter = counter;
            if (counter <= 0) cell->Flags &= ~CellFlags::FlagToShroud;
        } else {
            const int previous = counter;
            if (counter == -1) counter = 0;
            counter =
                std::min(std::bit_cast<int>(unsigned(counter) + 1u), std::bit_cast<int>(cell->GapsCoveringThisCell));
            cell->ShroudCounter = counter;
            if (previous <= 0 && counter > 0) cell->Flags |= CellFlags::FlagToShroud;
        }
        return;
    }
    CellStruct at{c.x, c.y};
    cell->Flags &= ~CellFlags::IsPlot;
    if (view.special && sight_reserved_cell(c.x, c.y)) return;
    if (state.all_dirty || (c.crossing && !settled(*cell))) {
        RadarClass::Instance.RevealFogShroud(&at, view.current, !up);
        return;
    }
    if (up)
        cell->ReduceShroudCounter();
    else
        cell->IncreaseShroudCounter();
    // Objects standing in a cell that enters the view are discovered, as the original does when the cell's
    // frame cache changes.
    if (up && c.crossing) MapClass::RevealCheck(cell, view.current, false);
}

// Cell newly covered by a source that already sees through another source: counters only.
void view_cross(const View& view, int x, int y, bool up) { view_change(view, {short(x), short(y), 0, true}, up); }

// Adds `to` and removes `from` from the plane; cells in both are untouched.
void apply_rows(HouseClass* owner, std::uint16_t* plane, bool applies, const Rows& from, const Rows& to,
                const SightSet& to_set, const SightSet& from_set) {
    View view;
    if (applies) {
        view.current = HouseClass::CurrentPlayer;
        view.special = sight_special_multiplayer();
    }
    Crossing ups[CrossingCapacity], downs[CrossingCapacity];
    int up_count = 0, down_count = 0;
    const auto& t = tables();
    const int y_first = std::min(from.y0, to.y0), y_last = std::max(from.y0, to.y0) + Window - 1;
    const int x_base = std::min(from.x0, to.x0);
    const int from_shift = from.x0 - x_base, to_shift = to.x0 - x_base;
    // Windows further apart than the 64-bit domain (or 21 rows) cannot overlap; treat them as separate sets.
    const bool disjoint = std::max(from_shift, to_shift) > 42 || std::abs(from.y0 - to.y0) >= Window;
    const auto row_of = [](const Rows& rows, int y) -> std::uint32_t {
        const int k = y - rows.y0;
        return k >= 0 && k < Window ? rows.m[k] : 0u;
    };
    const auto visit = [&](std::uint64_t bits, int y, int base, bool add) {
        while (bits) {
            const int b = std::countr_zero(bits);
            bits &= bits - 1;
            const int x = base + b;
            const int index = y * Span + x;
            const unsigned before = plane[index];
            if (add) {
                if (before != 0xFFFFu) plane[index] = std::uint16_t(before + 1);
            } else if (before) {
                plane[index] = std::uint16_t(before - 1);
            }
            if (!applies) continue;
            const bool crossing = add ? before == 0 : before == 1;
            bool covered = false;
            if (crossing) {
                if (!view.planes_ready) gather_view_planes(view, owner);
                for (int j = 0; j < view.other_count && !covered; ++j)
                    covered = view.others[j][index] != 0;
            }
            const auto& set = add ? to_set : from_set;
            const int dx = x - set.cx, dy = y - set.cy;
            const int key =
                std::abs(dx) <= Centre && std::abs(dy) <= Centre ? t.index[dy + Centre][dx + Centre] : 0x7FFF;
            if (add)
                ups[up_count++] = {short(x), short(y), key, crossing && !covered};
            else
                downs[down_count++] = {short(x), short(y), key, crossing && !covered};
        }
    };
    if (disjoint) {
        for (int k = 0; k < Window; ++k)
            visit(from.m[k], from.y0 + k, from.x0, false);
        for (int k = 0; k < Window; ++k)
            visit(to.m[k], to.y0 + k, to.x0, true);
    } else {
        for (int y = y_first; y <= y_last; ++y) {
            const std::uint64_t a = std::uint64_t(row_of(from, y)) << from_shift;
            const std::uint64_t b = std::uint64_t(row_of(to, y)) << to_shift;
            if (a == b) continue;
            visit(a & ~b, y, x_base, false);
            visit(b & ~a, y, x_base, true);
        }
    }
    // The original visits a circle in spiral order, releases before acquisitions.
    sort_by_key(downs, down_count);
    sort_by_key(ups, up_count);
    for (int i = 0; i < down_count; ++i)
        view_cross(view, downs[i].x, downs[i].y, false);
    for (int i = 0; i < up_count; ++i)
        view_cross(view, ups[i].x, ups[i].y, true);
}

void commit_pending() {
    auto& p = state.pending;
    if (!p.active) return;
    p.active = false;
    auto* plane = plane_for(p.owner->ArrayIndex);
    if (!plane) return;
    Rows from, none;
    build_rows(p.set, from);
    none.x0 = from.x0;
    none.y0 = from.y0;
    apply_rows(p.owner, plane, p.applies, from, none, p.set, p.set);
}

// Same recompute as the original reveal does for a visited cell: both frame layers, with the
// sticky centre bits, and the visible-cell notification when either layer changed.
void refresh_cell(CellClass& cell) {
    auto* tactical = TacticalClass::Instance;
    const auto& at = cell.MapCoords;
    bool changed = false;
    for (int layer = 0; layer < 2; ++layer) {
        const bool fog = layer != 0;
        const char frame = tactical->GetOcclusion(at, fog);
        auto& old = fog ? cell.Foggedness : cell.Visibility;
        if (old != frame) {
            old = frame;
            changed = true;
            if (!fog) cell.VisibilityChanged = true;
        }
        if (frame == -1) {
            if (fog)
                cell.Flags |= CellFlags::CenterRevealed;
            else
                cell.AltFlags |= AltCellFlags::NoFog;
        }
    }
    if (changed) tactical->RegisterCellAsVisible(&cell);
}

void refresh_caches() {
    auto& map = MapClass::Instance;
    if (!TacticalClass::Instance || !map.Cells.Items) {
        state.dirty_count = 0;
        state.all_dirty = false;
        return;
    }
    if (state.all_dirty) {
        for (int i = 0; i < map.Cells.Capacity && i < CellCount; ++i)
            if (auto* cell = map.Cells.Items[i]) refresh_cell(*cell);
        state.dirty_count = 0;
        state.all_dirty = false;
        return;
    }
    if (!state.dirty_count) return;
    if (!state.stamp) {
        state.stamp = new (std::nothrow) std::uint32_t[CellCount]();
        if (!state.stamp) {
            state.all_dirty = true;
            refresh_caches();
            return;
        }
    }
    if (++state.epoch == 0) {
        std::fill_n(state.stamp, CellCount, 0u);
        state.epoch = 1;
    }
    for (int n = 0; n < state.dirty_count; ++n) {
        const int centre = state.dirty[n];
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                const int index = centre + dy * Span + dx;
                if (index < 0 || index >= CellCount || state.stamp[index] == state.epoch) continue;
                state.stamp[index] = state.epoch;
                if (auto* cell = map.Cells.Items[index]) refresh_cell(*cell);
            }
    }
    state.dirty_count = 0;
}
}  // namespace

const CellStruct* sight_occlusion_offsets() noexcept { return occlusion; }

HouseClass* sight_owner(HouseClass* house) noexcept {
    auto* current = HouseClass::CurrentPlayer;
    if (house && current && house != current) {
        if (house->RadarVisibleTo.data & (1u << (unsigned(current->Type->ArrayIndex2) & 31))) house = current;
        if (house != current && house->IsAlliedWith(current) && RulesClass::Instance->AllyReveal) house = current;
    }
    return house;
}

bool sight_special_multiplayer() noexcept {
    const auto* current = &SessionClass::Instance;
    return current && (current->GameMode == GameMode::LAN || current->GameMode == GameMode::Internet) &&
           current->MPGameMode && !current->MPGameMode->vt_entry_04();
}

bool sight_reserved_cell(int x, int y) noexcept {
    const auto& rect = MapClass::Instance.MapRect;
    const int width = rect.Width, height = rect.Height;
    return (x == 7 && y == width + 5) || (x == 13 && y == width + 11) || (x == height + 13 && y == width + height - 15);
}

bool sight_in_map(int x, int y) noexcept {
    const auto& rect = MapClass::Instance.MapRect;
    const int sum = x + y, width = rect.Width;
    return sum > width && x - y < width && y - x < width && sum <= width + 2 * rect.Height;
}

void visibility_sight_apply(HouseClass* owner, const SightSet& set, bool release) noexcept {
    if (!owner || set.radius <= 0) return;
    auto& p = state.pending;
    const bool applies = HouseClass::CurrentPlayer && sight_owner(owner) == HouseClass::CurrentPlayer;
    if (release) {
        commit_pending();
        if (scope_depth == 0) {
            auto* plane = plane_for(owner->ArrayIndex);
            if (!plane) return;
            Rows from, none;
            build_rows(set, from);
            none.x0 = from.x0;
            none.y0 = from.y0;
            apply_rows(owner, plane, applies, from, none, set, set);
            return;
        }
        p.active = true;
        p.applies = applies;
        p.owner = owner;
        p.set = set;
        return;
    }
    auto* plane = plane_for(owner->ArrayIndex);
    if (!plane) return;
    Rows to, from;
    build_rows(set, to);
    SightSet from_set = set;
    if (p.active && p.owner == owner && p.applies == applies) {
        p.active = false;
        ++stats.netted;
        build_rows(p.set, from);
        from_set = p.set;
    } else {
        commit_pending();
        from.x0 = to.x0;
        from.y0 = to.y0;
    }
    apply_rows(owner, plane, applies, from, to, set, from_set);
}

void visibility_scope_enter() noexcept { ++scope_depth; }

void visibility_scope_leave() noexcept {
    if (scope_depth > 0 && --scope_depth == 0) commit_pending();
}

void visibility_flush() noexcept { commit_pending(); }

void visibility_reconcile() noexcept {
    commit_pending();
    auto& map = MapClass::Instance;
    if (!HouseClass::CurrentPlayer || !map.Cells.Items) return;
    View view;
    view.current = HouseClass::CurrentPlayer;
    view.special = sight_special_multiplayer();
    gather_view_planes(view, nullptr);
    if (!view.other_count) return;
    for (int index = 0; index < map.Cells.Capacity && index < CellCount; ++index) {
        auto* cell = map.Cells.Items[index];
        if (!cell) continue;
        const bool is_settled = settled(*cell);
        if (is_settled && cell->ShroudCounter < 0) continue;
        bool covered = false;
        for (int j = 0; j < view.other_count && !covered; ++j)
            covered = view.others[j][index] != 0;
        if (!covered) continue;
        if (is_settled)
            cell->ReduceShroudCounter();
        else
            view_cross(view, index % Span, index / Span, true);
    }
}

void visibility_frame_end() noexcept {
    commit_pending();
    refresh_caches();
}

void visibility_cache_dirty(CellStruct cell) noexcept {
    if (cell.X >= 0 && cell.Y >= 0 && cell.X < Span && cell.Y < Span) cache_push(cell.Y * Span + cell.X);
}

void visibility_cache_all_dirty() noexcept { state.all_dirty = true; }

void visibility_terrain_changed() noexcept { state.block_valid = false; }

void visibility_reset(bool release_storage) noexcept {
    state.pending = State::Pending{};
    scope_depth = 0;
    state.dirty_count = 0;
    state.all_dirty = false;
    state.block_valid = false;
    for (auto*& plane : state.planes) {
        if (!plane) continue;
        if (release_storage) {
            delete[] plane;
            plane = nullptr;
        } else {
            std::fill_n(plane, CellCount, std::uint16_t{});
        }
    }
    if (release_storage) {
        delete[] state.stamp;
        state.stamp = nullptr;
        delete[] state.dirty;
        state.dirty = nullptr;
        delete[] state.block_level;
        state.block_level = nullptr;
    }
}

std::uint64_t visibility_grid_hash() noexcept {
    commit_pending();
    std::uint64_t h = 1469598103934665603ull;
    const auto mix = [&h](std::uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            h ^= (v >> (i * 8)) & 0xFF;
            h *= 1099511628211ull;
        }
    };
    for (int p = 0; p < MaxPlanes; ++p) {
        if (!state.planes[p]) continue;
        mix(std::uint64_t(p));
        for (int i = 0; i < CellCount; ++i)
            if (const unsigned v = state.planes[p][i]) {
                mix(std::uint64_t(i));
                mix(v);
            }
    }
    return h;
}

VisibilityStats visibility_stats() noexcept { return stats; }

unsigned visibility_count(int house_index, int x, int y) noexcept {
    commit_pending();
    if (house_index < 0 || house_index >= MaxPlanes || !state.planes[house_index] || x < 0 || y < 0 || x >= Span ||
        y >= Span)
        return 0;
    return state.planes[house_index][y * Span + x];
}

int visibility_set_cells(const SightSet& set, CellStruct* out, int capacity) noexcept {
    Rows rows;
    build_rows(set, rows);
    int count = 0;
    for (int k = 0; k < Window; ++k) {
        std::uint32_t bits = rows.m[k];
        while (bits) {
            const int b = std::countr_zero(bits);
            bits &= bits - 1;
            if (count < capacity) out[count] = {short(rows.x0 + b), short(rows.y0 + k)};
            ++count;
        }
    }
    return count;
}
}  // namespace game
