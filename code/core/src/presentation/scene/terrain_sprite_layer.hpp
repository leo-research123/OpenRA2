#pragma once

#include "type_drawing_context.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace game {
// The resident background as OpenRA lays out its TerrainSpriteLayer: one instance per drawing group and
// pipeline state, a fixed number of slots per map cell, and the slot of a cell is its place in the original
// drawing order. A change rewrites only the slots of the item that changed and marks their rows dirty; an
// instance draws the range of rows the viewport can touch. The component is independent of any host: it
// decides addresses, dirty rows and draw ranges, and the host stores records by slot (type_drawing.hpp).
//
// Draw order: every instance also keeps its occupied slots in ascending order, and a draw range is a range of
// that order, not of slots. Most instances are sparse (overlays and terrain objects fill a few percent of the
// cells) and a row spans the whole map width, so drawing every slot of the visible rows would submit mostly
// empty slots; the order skips them. It changes only when a slot is taken or freed.
//
// Items are the original per-cell and per-object background items. A cell item (layers 1, 2, 3 and 5) owns
// the slots of its own cell; a terrain object (layer 4) owns slots at the cell it is anchored to. Cells
// are ordered by row (X + Y), then X. Layers 2, 3 and 4 draw rows from the last to the first.
//
// Overflow: a record that finds no free slot in its cell (more records of one state than the instance has
// slots per cell, an anchor outside the map, two objects in one cell) takes a slot of an overflow instance
// of the same state. Those are small, unordered pools that draw whole.
class TerrainSpriteLayer {
public:
    static constexpr int Span = 512;  // cell index = y * Span + x
    static constexpr int RowCount = 2 * Span - 1;
    static constexpr int RowHeight = 15;
    static constexpr std::uint32_t OverflowSlots = 256;
    static constexpr std::uint32_t MaxRecords = 64;  // per item
    static constexpr std::uint32_t NoCell = 0xFFFFFFFFu;

    struct ItemKey {
        std::uint32_t layer = 0;
        std::uint32_t cell = NoCell;  // cell_index() of a cell item; NoCell for a terrain object
        std::uint64_t object = 0;     // identity of a terrain object, 0 for a cell item
        bool operator==(const ItemKey&) const = default;
    };
    // What a host creates for one instance before it receives place operations that name it.
    struct InstanceInfo {
        std::uint32_t id = 0, layer = 0, group = 0, state = 0, overflow = 0, slots = 0;
    };

    static std::uint32_t cell_index(int x, int y) noexcept {
        return x >= 0 && y >= 0 && x < Span && y < Span ? std::uint32_t(y) * Span + std::uint32_t(x) : NoCell;
    }
    // Slots per cell of an instance: two for the terrain group of layer 1 (a TMP image and its extra image),
    // otherwise one.
    static std::uint32_t slots_per_cell(std::uint32_t layer, std::uint32_t group) noexcept {
        return layer == 1 && group == 0 ? 2 : 1;
    }
    static bool reversed(std::uint32_t layer) noexcept { return layer == 2 || layer == 3 || layer == 4; }
    static std::uint32_t group_of(std::uint32_t kind) noexcept {
        return kind == std::uint32_t(BackgroundRecordKind::shape) ? 1u : 0u;
    }

    // Forgets everything and lays out the given cells. row_origin_y is the pixel row of the cells with
    // X + Y == 0; row r starts RowHeight * r pixels below it.
    void reset(const CellStruct* cells, std::size_t count, int row_origin_y);
    std::size_t cell_count() const noexcept { return cells_; }

    // Chooses the slots of the records an item recorded now, at the cell `anchor` (a cell item's own cell, a
    // terrain object's current cell). addresses receives one per record, releases the
    // slots the item held before and no longer holds. The slots are owned from here on; a host that fails to
    // write them leaves the layer unusable until reset. Returns false when the item has more than MaxRecords
    // records. Instances created by the call are listed by take_created().
    bool place(const ItemKey&, std::uint32_t anchor, const BackgroundRecordInfo* records, std::uint32_t count,
               std::vector<BackgroundSlotAddress>& addresses, std::vector<BackgroundSlotAddress>& releases);
    bool holds(const ItemKey&) const;
    // Every item of a layer that holds slots (a copy, safe to place or remove while iterating).
    std::vector<ItemKey> items(std::uint32_t layer) const;

    // Marks the rows of these slots dirty; the caller does so when the host reported a change.
    void mark_dirty(const BackgroundSlotAddress*, std::size_t count);
    void take_created(std::vector<InstanceInfo>&);
    // The dirty slots as ranges, in slot order within each instance, and the draw orders that changed since
    // the last call (orders point into the layer until the next place or reset); clears the marks.
    void take_dirty(std::vector<BackgroundSlotRange>& dirty, std::vector<BackgroundOrderUpdate>& orders);

    // For every instance in drawing order, the range of its draw order whose records can touch pixel rows
    // [top, bottom) extended by margin in both directions. Empty ranges are included. Draw orders reflect the
    // last take_dirty.
    void ranges(int top, int bottom, int margin, std::vector<BackgroundSlotRange>& out) const;

    // CellClass::DrawIt submits the TMP and then its smudge before visiting the
    // next cell. Keep that order across pipeline groups and states. The cell order of a layer is cached and
    // rebuilt only after the slots of that layer changed: an item that is placed again on the slots it already
    // holds (a cell that was only re-recorded) leaves the cache valid.
    void order_cell_ranges(std::uint32_t layer, std::vector<BackgroundSlotRange>& ranges) const;

    // Observability for statistics and tests.
    struct Statistics {
        std::size_t instances = 0, overflow_instances = 0, items = 0, slots = 0, used_slots = 0;
    };
    // An instance's draw order as of the last take_dirty: its occupied slots, ascending.
    const std::vector<std::uint32_t>& order(std::uint32_t instance) const { return instances_[instance].order; }
    Statistics statistics() const;
    std::size_t instance_count() const noexcept { return instances_.size(); }
    const InstanceInfo& instance_info(std::uint32_t id) const { return instances_[id].info; }
    // The slot a cell takes in a main instance, or ~0u for a cell outside the layout.
    std::uint32_t slot_of(std::uint32_t instance, std::uint32_t cell, std::uint32_t k = 0) const;
    std::uint32_t draw_position(std::uint32_t instance) const;  // index in drawing order
    // The pixel extent above and below the row baseline the records of an instance reach so far.
    void extent(std::uint32_t instance, int& above, int& below) const;

private:
    struct Instance {
        InstanceInfo info;
        bool reverse = false, overflow = false;
        std::uint32_t per_cell = 1;
        std::vector<std::uint32_t> owner;  // slot -> item serial, 0 when free
        std::int32_t above = 0, below = 0;
        std::vector<std::uint32_t> dirty_rows;
        std::vector<std::uint8_t> dirty_flag;  // by memory row
        std::uint32_t dirty_first = ~0u, dirty_last = 0;
        std::vector<std::uint32_t> order;  // occupied slots, ascending
        std::vector<std::uint32_t> moved;  // slots taken or freed since the last take_dirty
    };
    struct Held {
        std::uint32_t serial = 0;
        std::vector<BackgroundSlotAddress> slots;
    };
    struct ItemHash {
        std::size_t operator()(const ItemKey& k) const noexcept {
            std::uint64_t h = k.object * 0x9E3779B97F4A7C15ull ^ (std::uint64_t(k.layer) << 40) ^ k.cell;
            h ^= h >> 31;
            h *= 0xBF58476D1CE4E5B9ull;
            return std::size_t(h ^ (h >> 29));
        }
    };
    std::uint32_t instance_for(std::uint32_t layer, std::uint32_t group, std::uint32_t state, std::uint32_t overflow);
    std::uint32_t memory_row(std::uint32_t row, bool reverse) const noexcept {
        return reverse ? std::uint32_t(RowCount - 1) - row : row;
    }
    const std::vector<std::uint32_t>& first_of(bool reverse) const noexcept {
        return reverse ? first_rev_ : first_fwd_;
    }
    std::uint32_t ordinal(std::uint32_t cell, bool reverse) const noexcept;
    std::uint32_t row_of_slot(const Instance&, std::uint32_t slot) const;
    void mark(Instance&, std::uint32_t slot);
    void own(Instance&, std::uint32_t slot, std::uint32_t serial);
    // Applies the moved slots to the order; returns the first position that changed, or ~0u.
    static std::uint32_t update_order(Instance&);

    std::size_t cells_ = 0;
    int row_origin_y_ = 0;
    std::vector<std::uint16_t> rank_;  // by cell index; 0xFFFF outside the layout
    std::array<std::uint16_t, RowCount> row_count_{};
    std::vector<std::uint32_t> first_fwd_, first_rev_;  // cells before each memory row, RowCount + 1 entries

    std::vector<Instance> instances_;
    std::vector<std::uint32_t> order_;  // instance ids in drawing order
    std::unordered_map<std::uint64_t, std::uint32_t> lookup_;
    std::vector<std::uint32_t> created_;
    std::unordered_map<ItemKey, Held, ItemHash> items_;
    std::uint32_t serial_ = 0;
    void rebuild_cell_runs(std::uint32_t layer) const;
    // The cell order of one layer (see order_cell_ranges): runs of positions in the draw orders of its instances.
    mutable bool cell_order_valid_ = false;
    mutable std::uint32_t cell_order_layer_ = 0;
    mutable std::vector<BackgroundSlotRange> cell_runs_;
    // Reused by the rebuild and by order_cell_ranges so that a call allocates nothing once warm.
    mutable std::vector<std::pair<std::uint32_t, const Held*>> cell_scratch_;
    mutable std::vector<const Held*> cell_table_;
    mutable std::vector<std::uint32_t> run_hint_;
    mutable std::vector<BackgroundSlotRange> ordered_scratch_;
};
}
