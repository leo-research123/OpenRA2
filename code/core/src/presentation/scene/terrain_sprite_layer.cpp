#include "presentation/scene/terrain_sprite_layer.hpp"
#include <algorithm>
#include <tuple>

namespace game {
namespace {
std::int64_t floor_div(std::int64_t n, std::int64_t d) noexcept { return n / d - (n % d < 0 ? 1 : 0); }
std::uint64_t pack(std::uint32_t layer, std::uint32_t group, std::uint32_t state, std::uint32_t overflow) noexcept {
    return std::uint64_t(layer & 0xFF) | (std::uint64_t(group & 0xFF) << 8) | (std::uint64_t(overflow & 0xFFFF) << 16) |
           (std::uint64_t(state) << 32);
}
}

void TerrainSpriteLayer::reset(const CellStruct* cells, std::size_t count, int row_origin_y) {
    instances_.clear();
    order_.clear();
    lookup_.clear();
    created_.clear();
    items_.clear();
    serial_ = 0;
    cell_order_valid_ = false;
    cell_runs_.clear();
    row_origin_y_ = row_origin_y;
    rank_.assign(std::size_t(Span) * Span, 0xFFFF);
    row_count_.fill(0);
    // Cells in layout order: by row, then X. Duplicates and cells outside the index range are ignored.
    std::vector<std::uint32_t> keys;
    keys.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const int x = cells[i].X, y = cells[i].Y;
        if (cell_index(x, y) == NoCell) continue;
        keys.push_back((std::uint32_t(x + y) << 16) | std::uint32_t(x));
    }
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    cells_ = keys.size();
    for (const auto key : keys) {
        const int row = int(key >> 16), x = int(key & 0xFFFF), y = row - x;
        rank_[std::size_t(y) * Span + x] = row_count_[row]++;
    }
    first_fwd_.assign(RowCount + 1, 0);
    first_rev_.assign(RowCount + 1, 0);
    for (int m = 0; m < RowCount; ++m) {
        first_fwd_[m + 1] = first_fwd_[m] + row_count_[m];
        first_rev_[m + 1] = first_rev_[m] + row_count_[RowCount - 1 - m];
    }
}

std::uint32_t TerrainSpriteLayer::ordinal(std::uint32_t cell, bool reverse) const noexcept {
    const std::uint32_t x = cell % Span, y = cell / Span;
    return first_of(reverse)[memory_row(x + y, reverse)] + rank_[cell];
}

std::uint32_t TerrainSpriteLayer::instance_for(std::uint32_t layer, std::uint32_t group, std::uint32_t state,
                                               std::uint32_t overflow) {
    const auto key = pack(layer, group, state, overflow);
    const auto found = lookup_.find(key);
    if (found != lookup_.end()) return found->second;
    Instance instance;
    instance.info = {std::uint32_t(instances_.size()), layer, group, state, overflow, 0};
    instance.reverse = reversed(layer);
    instance.overflow = overflow != 0;
    instance.per_cell = instance.overflow ? 1 : slots_per_cell(layer, group);
    instance.info.slots = instance.overflow ? OverflowSlots : std::uint32_t(cells_) * instance.per_cell;
    instance.owner.assign(instance.info.slots, 0);
    if (!instance.overflow) instance.dirty_flag.assign(RowCount, 0);
    const auto id = instance.info.id;
    instances_.push_back(std::move(instance));
    lookup_.emplace(key, id);
    created_.push_back(id);
    // Drawing order: layer, group, state, then the main instance before its overflow instances.
    const auto before = [&](std::uint32_t a, std::uint32_t b) {
        const auto& x = instances_[a].info;
        const auto& y = instances_[b].info;
        return std::tie(x.layer, x.group, x.state, x.overflow) < std::tie(y.layer, y.group, y.state, y.overflow);
    };
    order_.insert(std::upper_bound(order_.begin(), order_.end(), id, before), id);
    return id;
}

bool TerrainSpriteLayer::place(const ItemKey& key, std::uint32_t anchor, const BackgroundRecordInfo* records,
                               std::uint32_t count, std::vector<BackgroundSlotAddress>& addresses,
                               std::vector<BackgroundSlotAddress>& releases) {
    addresses.clear();
    releases.clear();
    if (count > MaxRecords || (count && !records)) return false;
    auto held = items_.find(key);
    if (held == items_.end() && count == 0) return true;
    const std::uint32_t serial = held != items_.end() ? held->second.serial : ++serial_;
    const bool anchored = anchor != NoCell && anchor < rank_.size() && rank_[anchor] != 0xFFFF;
    const std::uint32_t row = anchored ? anchor % Span + anchor / Span : 0;
    const auto claimed = [&](std::uint32_t instance, std::uint32_t slot) {
        for (const auto& a : addresses)
            if (a.instance == instance && a.slot == slot) return true;
        return false;
    };
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto& record = records[i];
        const auto group = group_of(record.kind);
        const auto main = instance_for(key.layer, group, record.state, 0);
        BackgroundSlotAddress at;
        if (anchored) {
            auto& instance = instances_[main];
            const auto base = ordinal(anchor, instance.reverse) * instance.per_cell;
            for (std::uint32_t k = 0; k < instance.per_cell; ++k) {
                const auto slot = base + k;
                if ((instance.owner[slot] == 0 || instance.owner[slot] == serial) && !claimed(main, slot)) {
                    at = {main, slot};
                    break;
                }
            }
        }
        for (std::uint32_t chain = 0; at.instance == BackgroundInvalidInstance; ++chain) {
            const auto id = instance_for(key.layer, group, record.state, chain + 1);
            auto& instance = instances_[id];
            for (std::uint32_t slot = 0; slot < instance.owner.size(); ++slot)
                if ((instance.owner[slot] == 0 || instance.owner[slot] == serial) && !claimed(id, slot)) {
                    at = {id, slot};
                    break;
                }
        }
        auto& instance = instances_[at.instance];
        own(instance, at.slot, serial);
        if (!instance.overflow) {
            const int base_y = row_origin_y_ + RowHeight * int(row);
            instance.above = std::max<std::int32_t>(instance.above, base_y - record.bounds.Y);
            instance.below = std::max<std::int32_t>(instance.below, record.bounds.Y + record.bounds.Height - base_y);
        }
        addresses.push_back(at);
    }
    // The cached cell order depends on the slots every item holds (their sequence included) and on the draw orders
    // those slots occupy. An item that is placed again on exactly the slots it holds changes neither.
    bool layout_changed = true;
    if (held != items_.end()) {
        const auto& old_slots = held->second.slots;
        const auto same = [](const auto& x, const auto& y) { return x.instance == y.instance && x.slot == y.slot; };
        layout_changed = count == 0 || old_slots.size() != addresses.size() ||
                         !std::equal(old_slots.begin(), old_slots.end(), addresses.begin(), same);
        for (const auto& old : held->second.slots)
            if (!claimed(old.instance, old.slot)) {
                own(instances_[old.instance], old.slot, 0);
                releases.push_back(old);
            }
        if (count == 0)
            items_.erase(held);
        else
            held->second.slots = addresses;
    } else {
        items_.emplace(key, Held{serial, addresses});
    }
    // Items of another layer never take part in the order of the cached layer.
    if (layout_changed && key.layer == cell_order_layer_) cell_order_valid_ = false;
    return true;
}

bool TerrainSpriteLayer::holds(const ItemKey& key) const { return items_.find(key) != items_.end(); }

std::vector<TerrainSpriteLayer::ItemKey> TerrainSpriteLayer::items(std::uint32_t layer) const {
    std::vector<ItemKey> out;
    for (const auto& [key, held] : items_)
        if (key.layer == layer) out.push_back(key);
    return out;
}

std::uint32_t TerrainSpriteLayer::row_of_slot(const Instance& instance, std::uint32_t slot) const {
    const auto& first = first_of(instance.reverse);
    const auto cell = slot / instance.per_cell;
    return std::uint32_t(std::upper_bound(first.begin(), first.end(), cell) - first.begin()) - 1;
}

void TerrainSpriteLayer::own(Instance& instance, std::uint32_t slot, std::uint32_t serial) {
    auto& owner = instance.owner[slot];
    if ((owner == 0) != (serial == 0)) instance.moved.push_back(slot);
    owner = serial;
}

std::uint32_t TerrainSpriteLayer::update_order(Instance& instance) {
    auto& moved = instance.moved;
    auto& order = instance.order;
    if (moved.empty()) return ~0u;
    std::uint32_t from = ~0u;
    if (moved.size() > 64) {
        // Many changes (a full recording): scan the owners once.
        std::vector<std::uint32_t> fresh;
        fresh.reserve(order.size() + moved.size());
        for (std::uint32_t slot = 0; slot < instance.owner.size(); ++slot)
            if (instance.owner[slot]) fresh.push_back(slot);
        const auto common = std::min(order.size(), fresh.size());
        const auto split = std::mismatch(order.begin(), order.begin() + common, fresh.begin()).first - order.begin();
        if (std::size_t(split) != common || order.size() != fresh.size()) from = std::uint32_t(split);
        order.swap(fresh);
    } else {
        for (const auto slot : moved) {
            const auto at = std::lower_bound(order.begin(), order.end(), slot);
            const bool present = at != order.end() && *at == slot, wanted = instance.owner[slot] != 0;
            if (present == wanted) continue;
            from = std::min(from, std::uint32_t(at - order.begin()));
            if (wanted)
                order.insert(at, slot);
            else
                order.erase(at);
        }
    }
    moved.clear();
    return from;
}

void TerrainSpriteLayer::mark(Instance& instance, std::uint32_t slot) {
    if (instance.overflow) {
        instance.dirty_first = std::min(instance.dirty_first, slot);
        instance.dirty_last = std::max(instance.dirty_last, slot);
    } else {
        const auto row = row_of_slot(instance, slot);
        if (!instance.dirty_flag[row]) {
            instance.dirty_flag[row] = 1;
            instance.dirty_rows.push_back(row);
        }
    }
}

void TerrainSpriteLayer::mark_dirty(const BackgroundSlotAddress* slots, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i)
        if (slots[i].instance < instances_.size() && slots[i].slot < instances_[slots[i].instance].info.slots)
            mark(instances_[slots[i].instance], slots[i].slot);
}

void TerrainSpriteLayer::take_created(std::vector<InstanceInfo>& out) {
    out.clear();
    for (const auto id : created_)
        out.push_back(instances_[id].info);
    created_.clear();
}

void TerrainSpriteLayer::take_dirty(std::vector<BackgroundSlotRange>& out, std::vector<BackgroundOrderUpdate>& orders) {
    out.clear();
    orders.clear();
    for (auto& instance : instances_) {
        const auto from = update_order(instance);
        if (from != ~0u && instance.info.layer == cell_order_layer_) cell_order_valid_ = false;
        if (from != ~0u)
            orders.push_back(
                {instance.info.id, from, std::uint32_t(instance.order.size()), instance.order.data() + from});
        if (instance.overflow) {
            if (instance.dirty_first != ~0u)
                out.push_back({instance.info.id, instance.dirty_first, instance.dirty_last - instance.dirty_first + 1});
            instance.dirty_first = ~0u;
            instance.dirty_last = 0;
            continue;
        }
        if (instance.dirty_rows.empty()) continue;
        std::sort(instance.dirty_rows.begin(), instance.dirty_rows.end());
        const auto& first = first_of(instance.reverse);
        for (const auto row : instance.dirty_rows) {
            instance.dirty_flag[row] = 0;
            const auto begin = first[row] * instance.per_cell, end = first[row + 1] * instance.per_cell;
            if (begin == end) continue;
            if (!out.empty() && out.back().instance == instance.info.id && out.back().first + out.back().count == begin)
                out.back().count += end - begin;
            else
                out.push_back({instance.info.id, begin, end - begin});
        }
        instance.dirty_rows.clear();
    }
}

void TerrainSpriteLayer::ranges(int top, int bottom, int margin, std::vector<BackgroundSlotRange>& out) const {
    out.clear();
    for (const auto id : order_) {
        const auto& instance = instances_[id];
        const auto& order = instance.order;
        if (instance.overflow) {
            out.push_back({id, 0, std::uint32_t(order.size())});
            continue;
        }
        // Row r holds records between its baseline minus `above` and baseline plus `below`; keep the rows whose
        // span meets [top - margin, bottom + margin).
        const std::int64_t y0 = row_origin_y_;
        const auto low = floor_div(std::int64_t(top) - margin - instance.below - y0, RowHeight);
        const auto high = floor_div(std::int64_t(bottom) + margin + instance.above - y0, RowHeight);
        if (high < 0 || low > RowCount - 1 || high < low) {
            out.push_back({id, 0, 0});
            continue;
        }
        const auto row_low = std::uint32_t(std::max<std::int64_t>(low, 0));
        const auto row_high = std::uint32_t(std::min<std::int64_t>(high, RowCount - 1));
        const auto m_low = instance.reverse ? RowCount - 1 - row_high : row_low;
        const auto m_high = instance.reverse ? RowCount - 1 - row_low : row_high;
        const auto& first = first_of(instance.reverse);
        // The occupied slots of those rows: a range of the draw order.
        const auto begin = std::lower_bound(order.begin(), order.end(), first[m_low] * instance.per_cell);
        const auto end = std::lower_bound(begin, order.end(), first[m_high + 1] * instance.per_cell);
        out.push_back({id, std::uint32_t(begin - order.begin()), std::uint32_t(end - begin)});
    }
}

void TerrainSpriteLayer::rebuild_cell_runs(std::uint32_t layer) const {
    const bool reverse = reversed(layer);
    auto& cells = cell_scratch_;
    cells.clear();
    for (const auto& [key, held] : items_)
        if (key.layer == layer && key.cell != NoCell) cells.emplace_back(ordinal(key.cell, reverse), &held);
    // Order the items by cell ordinal. Ordinals are dense and unique per item in a valid layout, so placing each
    // item at its ordinal replaces the sort; anything else (an ordinal outside the layout, a tie) sorts.
    auto& table = cell_table_;
    table.assign(cells_, nullptr);
    bool direct = true;
    for (const auto& [rank, held] : cells) {
        if (rank >= table.size() || table[rank]) {
            direct = false;
            break;
        }
        table[rank] = held;
    }
    if (direct) {
        std::size_t next = 0;
        for (std::uint32_t rank = 0; rank < table.size() && next < cells.size(); ++rank)
            if (table[rank]) cells[next++] = {rank, table[rank]};
    } else {
        std::sort(cells.begin(), cells.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    }
    auto& runs = cell_runs_;
    runs.clear();
    // The slots of an instance mostly rise from one item to the next: try the entry after the previous hit first.
    auto& hint = run_hint_;
    hint.assign(instances_.size(), 0);
    for (const auto& [rank, held] : cells)
        for (const auto& address : held->slots) {
            const auto& slots = instances_[address.instance].order;
            auto& guess = hint[address.instance];
            std::uint32_t position;
            if (guess < slots.size() && slots[guess] == address.slot) {
                position = guess;
            } else {
                const auto at = std::lower_bound(slots.begin(), slots.end(), address.slot);
                if (at == slots.end() || *at != address.slot) continue;
                position = std::uint32_t(at - slots.begin());
            }
            guess = position + 1;
            if (!runs.empty() && runs.back().instance == address.instance &&
                runs.back().first + runs.back().count == position)
                ++runs.back().count;
            else
                runs.push_back({address.instance, position, 1});
        }
    cell_order_layer_ = layer;
    cell_order_valid_ = true;
}

void TerrainSpriteLayer::order_cell_ranges(std::uint32_t layer, std::vector<BackgroundSlotRange>& ranges) const {
    const auto first = std::find_if(ranges.begin(), ranges.end(), [&](const auto& r) {
        return instances_[r.instance].info.layer == layer;
    });
    if (first == ranges.end()) return;
    if (!cell_order_valid_ || cell_order_layer_ != layer) rebuild_cell_runs(layer);
    const auto& runs = cell_runs_;
    auto& ordered = ordered_scratch_;
    ordered.clear();
    ordered.reserve(ranges.size() + runs.size());
    ordered.insert(ordered.end(), ranges.begin(), first);
    for (const auto& run : runs) {
        const auto range = std::find_if(first, ranges.end(), [&](const auto& r) { return r.instance == run.instance; });
        if (range == ranges.end()) continue;
        const auto begin = std::max(run.first, range->first);
        const auto end = std::min(run.first + run.count, range->first + range->count);
        ordered.push_back({run.instance, begin, end > begin ? end - begin : 0});
    }
    for (auto at = first; at != ranges.end(); ++at)
        if (instances_[at->instance].info.layer != layer) ordered.push_back(*at);
    ranges.swap(ordered);
}

TerrainSpriteLayer::Statistics TerrainSpriteLayer::statistics() const {
    Statistics s;
    s.instances = instances_.size();
    s.items = items_.size();
    for (const auto& instance : instances_) {
        s.overflow_instances += instance.overflow;
        s.slots += instance.info.slots;
        for (const auto owner : instance.owner)
            s.used_slots += owner != 0;
    }
    return s;
}

std::uint32_t TerrainSpriteLayer::slot_of(std::uint32_t instance, std::uint32_t cell, std::uint32_t k) const {
    if (instance >= instances_.size() || cell == NoCell || cell >= rank_.size() || rank_[cell] == 0xFFFF) return ~0u;
    const auto& i = instances_[instance];
    return ordinal(cell, i.reverse) * i.per_cell + k;
}

std::uint32_t TerrainSpriteLayer::draw_position(std::uint32_t instance) const {
    return std::uint32_t(std::find(order_.begin(), order_.end(), instance) - order_.begin());
}

void TerrainSpriteLayer::extent(std::uint32_t instance, int& above, int& below) const {
    above = instances_[instance].above;
    below = instances_[instance].below;
}
}
