#include "tactical_spatial.hpp"
#include "presentation/visibility.hpp"
#include "point_grid.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include <array>
#include <bit>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace game {
namespace {
constexpr auto invalid = ~std::uint32_t{};
struct SpatialSlot {
    ObjectClass* object = nullptr;
    std::vector<std::uint32_t> entries;
    std::vector<std::uint32_t> attachments;
    std::uint32_t parent = invalid;
    std::uint64_t building_order = 0;
    CoordStruct position{};
    Point2D building_position{};
    std::uint32_t visible_offset = 0;
    bool dirty = false, building = false, visible = false;
};
struct SpatialEntry {
    std::uint32_t slot = invalid;
    int layer = 0, index = 0;
    std::uint32_t visible_offset = invalid;
};
struct SpatialState {
    std::unordered_map<ObjectClass*, std::uint32_t> identities;
    std::vector<SpatialSlot> slots;
    std::vector<SpatialEntry> entries;
    std::vector<std::uint32_t> free_slots, free_entries, dirty, visible, building_candidates;
    std::vector<std::uint32_t> position_candidates;
    TacticalWorldBounds candidate_bounds{};
    std::uint64_t candidate_revision = 0;
    std::array<std::vector<std::uint32_t>, 5> layers;
    std::array<std::vector<std::uint32_t>, 5> visible_entries;
    std::array<std::vector<std::uint64_t>, 5> candidate_bits;
    PointGrid positions, buildings{8}, anchors{5};
    std::uint64_t revision = 1, building_sequence = 0;
    std::uint32_t duplicated_objects = 0;
    std::uint64_t generation = 1;
    std::uint64_t selectable_revision = 1;
    bool reconcile = false, failed = false, anchors_valid = false;
    int anchor_count = 0;
    std::uint32_t identity(ObjectClass* object) {
        if (auto found = identities.find(object); found != identities.end()) return found->second;
        const auto id = free_slots.empty() ? std::uint32_t(slots.size()) : free_slots.back();
        if (free_slots.empty())
            slots.emplace_back();
        else
            free_slots.pop_back();
        auto& slot = slots[id];
        slot.object = object;
        slot.dirty = false;
        slot.building = false;
        slot.visible = false;
        identities.emplace(object, id);
        return id;
    }
    void mark(std::uint32_t id) {
        auto& slot = slots[id];
        if (!slot.dirty) {
            dirty.push_back(id);
            slot.dirty = true;
            ++generation;
            for (auto child : slot.attachments)
                mark(child);
        }
    }
    void attach(std::uint32_t child, ObjectClass* owner) {
        const auto parent = owner ? identity(owner) : invalid;
        auto& slot = slots[child];
        if (slot.parent == parent) return;
        if (slot.parent != invalid) std::erase(slots[slot.parent].attachments, child);
        slot.parent = parent;
        if (parent != invalid) slots[parent].attachments.push_back(child);
        mark(child);
    }
    void set_visible(std::uint32_t entry, bool visible) {
        auto& e = entries[entry];
        auto& list = visible_entries[e.layer];
        if (visible && e.visible_offset == invalid) {
            e.visible_offset = std::uint32_t(list.size());
            list.push_back(entry);
        } else if (!visible && e.visible_offset != invalid) {
            const auto moved = list.back();
            list[e.visible_offset] = moved;
            entries[moved].visible_offset = e.visible_offset;
            list.pop_back();
            e.visible_offset = invalid;
        }
    }
    void insert(int layer, int index, ObjectClass* object) {
        const auto slot = identity(object);
        const auto entry = free_entries.empty() ? std::uint32_t(entries.size()) : free_entries.back();
        if (free_entries.empty())
            entries.emplace_back();
        else
            free_entries.pop_back();
        entries[entry] = {slot, layer, index};
        if (slots[slot].entries.size() == 1) ++duplicated_objects;
        slots[slot].entries.push_back(entry);
        if (object && object->WhatAmI() == AbstractType::Anim)
            attach(slot, static_cast<AnimClass*>(object)->OwnerObject);
        if (object && visible_objects().contains(object) && !slots[slot].visible) {
            slots[slot].visible_offset = std::uint32_t(visible.size());
            visible.push_back(slot);
            slots[slot].visible = true;
        }
        if (slots[slot].visible) set_visible(entry, true);
        auto& order = layers[layer];
        order.insert(order.begin() + index, entry);
        for (int i = index + 1; i < int(order.size()); ++i)
            entries[order[i]].index = i;
        mark(slot);
        ++revision;
    }
    void remove(int layer, int index) {
        auto& order = layers[layer];
        const auto entry = order[index];
        auto& e = entries[entry];
        set_visible(entry, false);
        if (e.slot != invalid) {
            auto& list = slots[e.slot].entries;
            if (list.size() == 2) --duplicated_objects;
            std::erase(list, entry);
            if (list.empty()) positions.erase(e.slot);
        }
        e.slot = invalid;
        free_entries.push_back(entry);
        order.erase(order.begin() + index);
        for (int i = index; i < int(order.size()); ++i)
            entries[order[i]].index = i;
        ++revision;
        ++generation;
    }
};
std::unique_ptr<SpatialState> state_owner;
SpatialState* state = nullptr;
int layer_number(const LayerClass* layer) noexcept {
    for (int n = 0; n < 5; ++n)
        if (layer == &MapClass::ObjectsInLayers[n]) return n;
    return -1;
}
template <class F>
void mutate(F&& operation) noexcept {
    if (!state || state->failed) return;
    try {
        operation(*state);
    } catch (...) {
        state->failed = true;
    }
}
void add_building(SpatialState& s, ObjectClass* object) {
    const auto id = s.identity(object);
    auto& slot = s.slots[id];
    if (!slot.building) {
        slot.building = true;
        slot.building_order = ++s.building_sequence;
    }
    s.mark(id);
}
struct QueryBuffers {
    std::vector<std::uint32_t> candidates, scratch;
    std::vector<std::uint64_t> membership;
};
thread_local std::vector<QueryBuffers> query_buffers;
thread_local std::size_t query_depth = 0;
bool verify_spatial() {
    static const bool enabled = [] {
        const auto* value = std::getenv("RA2_VERIFY_SPATIAL");
        return value && value[0] && std::strcmp(value, "0") != 0;
    }();
    return enabled;
}
}
std::uint64_t tactical_spatial_layer_revision() noexcept { return state ? state->revision : 0; }
bool tactical_spatial_unique_entries() noexcept { return state && !state->failed && !state->duplicated_objects; }
bool tactical_spatial_active() noexcept { return bool(state); }
std::uint64_t tactical_spatial_generation() noexcept { return state ? state->generation : 0; }
void start_tactical_spatial() {
    state_owner = std::make_unique<SpatialState>();
    state = state_owner.get();
    state->reconcile = true;
    flush_tactical_spatial();
}
void stop_tactical_spatial() noexcept {
    state = nullptr;
    state_owner.reset();
}
void tactical_spatial_reconcile() noexcept {
    if (state) {
        state->reconcile = true;
        ++state->generation;
    }
}
void tactical_spatial_dirty(ObjectClass* object) noexcept {
    mutate([&](auto& s) {
        if (auto found = s.identities.find(object); found != s.identities.end()) s.mark(found->second);
    });
}
void tactical_spatial_dirty(const ObjectClass* object) noexcept { tactical_spatial_dirty(const_cast<ObjectClass*>(object)); }
void tactical_spatial_building(ObjectClass* object) noexcept {
    mutate([&](auto& s) { add_building(s, object); });
}
void tactical_spatial_forget(ObjectClass* object) noexcept {
    visible_objects().forget(object);
    hidden_anims().forget(object);
    forget_unit_waterline(object);
    redraw_requests().forget(object);
    mutate([&](auto& s) {
        const auto found = s.identities.find(object);
        if (found == s.identities.end()) return;
        const auto id = found->second;
        auto& slot = s.slots[id];
        if (slot.parent != invalid) std::erase(s.slots[slot.parent].attachments, id);
        slot.parent = invalid;
        for (auto child : slot.attachments) {
            s.slots[child].parent = invalid;
            s.mark(child);
        }
        slot.attachments.clear();
        for (auto entry : slot.entries) {
            s.set_visible(entry, false);
            s.entries[entry].slot = invalid;
        }
        if (slot.entries.size() > 1) --s.duplicated_objects;
        slot.entries.clear();
        slot.object = nullptr;
        slot.dirty = false;
        s.positions.erase(id);
        s.buildings.erase(id);
        s.identities.erase(found);
        std::erase(s.dirty, id);
        if (slot.visible) {
            const auto moved = s.visible.back();
            s.visible[slot.visible_offset] = moved;
            s.slots[moved].visible_offset = slot.visible_offset;
            s.visible.pop_back();
        }
        slot.visible = false;
        s.free_slots.push_back(id);
        ++s.revision;
        ++s.generation;
        s.anchors_valid = false;
    });
}
void tactical_spatial_insert(LayerClass* layer, int index, ObjectClass* object) noexcept {
    const int n = layer_number(layer);
    if (n < 0) return;
    mutate([&](auto& s) {
        if (!s.reconcile) s.insert(n, index, object);
    });
}
void tactical_spatial_remove(LayerClass* layer, int index) noexcept {
    const int n = layer_number(layer);
    if (n < 0) return;
    mutate([&](auto& s) {
        if (!s.reconcile) s.remove(n, index);
    });
}
void tactical_spatial_swap(LayerClass* layer, int first, int second) noexcept {
    const int n = layer_number(layer);
    if (n < 0) return;
    mutate([&](auto& s) {
        if (s.reconcile) return;
        auto& order = s.layers[n];
        std::swap(order[first], order[second]);
        s.entries[order[first]].index = first;
        s.entries[order[second]].index = second;
        ++s.revision;
        ++s.generation;
    });
}
void flush_tactical_spatial() {
    if (!state) return;
    auto& s = *state;
    if (s.failed) throw std::runtime_error("Tactical spatial index allocation failed");
    if (s.reconcile) {
        state_owner = std::make_unique<SpatialState>();
        state = state_owner.get();
        auto& fresh = *state;
        for (int layer = 0; layer < 5; ++layer) {
            auto& objects = MapClass::ObjectsInLayers[layer];
            for (int i = 0; i < objects.Count; ++i)
                fresh.insert(layer, i, objects[i]);
        }
        for (auto* building : BuildingClass::Array)
            add_building(fresh, building);
        flush_tactical_spatial();
        return;
    }
    if (s.dirty.empty()) return;
    bool changed = false;
    for (auto id : s.dirty) {
        auto& slot = s.slots[id];
        slot.dirty = false;
        if (!slot.object) continue;
        if (!slot.entries.empty()) {
            const auto at =
                slot.object->WhatAmI() == AbstractType::Anim ? slot.object->GetCoords() : slot.object->Location;
            changed |= at.X != slot.position.X || at.Y != slot.position.Y;
            slot.position = at;
            s.positions.set(id, at.X, at.Y);
        }
        if (slot.building) {
            const auto at = TacticalClass::CoordsToScreen(slot.object->Location);
            changed |= at != slot.building_position;
            slot.building_position = at;
            s.buildings.set(id, at.X, at.Y);
        }
    }
    if (changed) ++s.revision;
    if (!s.dirty.empty()) s.anchors_valid = false;
    s.dirty.clear();
}
TacticalWorldBounds tactical_object_bounds(TacticalClass& tactical, const RectangleStruct& bounds) {
    const auto wrap = [](unsigned value) { return std::bit_cast<int>(value); };
    const auto camera = tactical.TacticalPos;
    const auto top = tactical.ApplyMatrix_Pixel(
        {wrap(unsigned(camera.X) + unsigned(bounds.Width / 2)), wrap(unsigned(camera.Y) - unsigned(bounds.Height))});
    const auto right = tactical.ApplyMatrix_Pixel({wrap(unsigned(camera.X) + 2u * unsigned(bounds.Width)),
                                                   wrap(unsigned(camera.Y) + unsigned(bounds.Height / 2))});
    const auto left = tactical.ApplyMatrix_Pixel(
        {wrap(unsigned(camera.X) - unsigned(bounds.Width)), wrap(unsigned(camera.Y) + unsigned(bounds.Height / 2))});
    return {top.X, top.Y, wrap(unsigned(top.X) + 2u * (unsigned(right.X) - unsigned(top.X))),
            wrap(unsigned(top.Y) + 2u * (unsigned(left.Y) - unsigned(top.Y)))};
}
TacticalLayerQuery::TacticalLayerQuery() {
    if (query_buffers.size() == query_depth) query_buffers.emplace_back();
    auto& buffers = query_buffers[query_depth++];
    candidates.swap(buffers.candidates);
    scratch.swap(buffers.scratch);
    membership.swap(buffers.membership);
}
TacticalLayerQuery::~TacticalLayerQuery() {
    auto& buffers = query_buffers[--query_depth];
    candidates.swap(buffers.candidates);
    scratch.swap(buffers.scratch);
    membership.swap(buffers.membership);
}
void TacticalLayerQuery::begin(int layer_, const TacticalWorldBounds& bounds_, bool include_visible_) {
    layer = layer_;
    bounds = bounds_;
    include_visible = include_visible_;
    next_index = 0;
    revision = 0;
    cursor = 0;
}
void TacticalLayerQuery::refresh() {
    auto& s = *state;
    candidates.clear();
    const auto previous = s.candidate_bounds;
    if (s.candidate_revision != s.revision || previous.left != bounds.left || previous.top != bounds.top ||
        previous.right != bounds.right || previous.bottom != bounds.bottom) {
        s.positions.query(bounds.left, bounds.top, bounds.right, bounds.bottom, s.position_candidates);
        for (int n = 0; n < 5; ++n)
            s.candidate_bits[n].assign((s.layers[n].size() + 63) / 64, 0);
        for (auto id : s.position_candidates)
            for (auto entry : s.slots[id].entries) {
                const auto& e = s.entries[entry];
                s.candidate_bits[e.layer][unsigned(e.index) / 64] |= std::uint64_t{1} << (e.index % 64);
            }
        s.candidate_revision = s.revision;
        s.candidate_bounds = bounds;
    }
    membership = s.candidate_bits[layer];
    if (include_visible) {
        // Only objects that were visible need the original clearing side effect
        // outside this frame's rectangle. Keep it at their original list index.
        for (auto entry : s.visible_entries[layer]) {
            const auto& e = s.entries[entry];
            membership[unsigned(e.index) / 64] |= std::uint64_t{1} << (e.index % 64);
        }
    }
    // Original ordinals are already dense. Extract set bits in ascending order
    // instead of sorting a temporary list, and deduplicate at the same time.
    const std::size_t first_word = unsigned(next_index) / 64;
    const auto word_bits = [&](std::size_t word) {
        auto bits = membership[word];
        if (word == first_word) bits &= ~std::uint64_t{} << (next_index % 64);
        return bits;
    };
    std::size_t total = 0;
    for (std::size_t word = first_word; word < membership.size(); ++word) total += std::popcount(word_bits(word));
    candidates.resize(total);
    auto* out = candidates.data();
    for (std::size_t word = first_word; word < membership.size(); ++word) {
        auto bits = word_bits(word);
        while (bits) {
            *out++ = std::uint32_t(word * 64 + std::countr_zero(bits));
            bits &= bits - 1;
        }
    }
    cursor = 0;
    revision = s.revision;
    if (verify_spatial()) {
        scratch.clear();
        const auto& original = MapClass::ObjectsInLayers[layer];
        for (int i = next_index; i < original.Count; ++i) {
            auto* object = original[i];
            if (!object) continue;
            const auto at = object->WhatAmI() == AbstractType::Anim ? object->GetCoords() : object->Location;
            if ((at.X >= bounds.left && at.X < bounds.right && at.Y >= bounds.top && at.Y < bounds.bottom) ||
                (include_visible && visible_objects().contains(object)))
                scratch.push_back(std::uint32_t(i));
        }
        if (scratch.size() != candidates.size())
            throw std::runtime_error("Spatial candidate count differs from original layer");
        for (std::size_t i = 0; i < scratch.size(); ++i)
            if (scratch[i] != candidates[i])
                throw std::runtime_error("Spatial candidate order differs from original layer");
    }
}
bool TacticalLayerQuery::next(ObjectClass*& output, int& original_index) {
    output = nullptr;
    if (!state) {
        auto& objects = MapClass::ObjectsInLayers[layer];
        while (next_index < objects.Count) {
            original_index = next_index++;
            if ((output = objects[original_index])) return true;
        }
        return false;
    }
    if (state->failed || state->reconcile || !state->dirty.empty()) flush_tactical_spatial();
    auto& s = *state;
    if (revision != s.revision) refresh();
    while (cursor < candidates.size()) {
        original_index = int(candidates[cursor++]);
        next_index = original_index + 1;
        if ((output = MapClass::ObjectsInLayers[layer][original_index])) return true;
    }
    return false;
}

void tactical_spatial_visible(ObjectClass* object) noexcept {
    mutate([&](auto& s) {
        const auto found = s.identities.find(object);
        if (found == s.identities.end()) return;
        auto& slot = s.slots[found->second];
        const bool in_view = visible_objects().contains(object);
        if (in_view && !slot.visible) {
            slot.visible_offset = std::uint32_t(s.visible.size());
            s.visible.push_back(found->second);
            slot.visible = true;
            for (auto entry : slot.entries)
                s.set_visible(entry, true);
        } else if (!in_view && slot.visible) {
            const auto moved = s.visible.back();
            s.visible[slot.visible_offset] = moved;
            s.slots[moved].visible_offset = slot.visible_offset;
            s.visible.pop_back();
            slot.visible = false;
            for (auto entry : slot.entries)
                s.set_visible(entry, false);
        }
    });
}
void collect_tactical_buildings(const RectangleStruct& bounds, std::vector<ObjectClass*>& output) {
    output.clear();
    flush_tactical_spatial();
    if (!state) {
        for (auto* b : BuildingClass::Array)
            output.push_back(b);
        return;
    }
    auto& s = *state;
    const auto camera = TacticalClass::Instance->TacticalPos;
    s.buildings.query(std::int64_t(bounds.X) + camera.X, std::int64_t(bounds.Y) + camera.Y,
                      std::int64_t(bounds.X) + camera.X + bounds.Width + 1,
                      std::int64_t(bounds.Y) + camera.Y + bounds.Height + 1, s.building_candidates);
    std::sort(s.building_candidates.begin(), s.building_candidates.end(),
              [&](auto a, auto b) { return s.slots[a].building_order < s.slots[b].building_order; });
    for (auto id : s.building_candidates)
        output.push_back(s.slots[id].object);
}
void invalidate_selectable_index() noexcept {
    if (state) {
        state->anchors_valid = false;
        ++state->selectable_revision;
    }
}
std::uint64_t selectable_index_revision() noexcept { return state ? state->selectable_revision : 0; }
void collect_visible_tactical_buildings(std::vector<ObjectClass*>& output) {
    output.clear();
    if (!state) {
        for (auto* b : BuildingClass::Array)
            output.push_back(b);
        return;
    }
    auto& s = *state;
    s.building_candidates.clear();
    for (auto id : s.visible) {
        const auto& slot = s.slots[id];
        if (slot.building && slot.object && visible_objects().contains(slot.object)) s.building_candidates.push_back(id);
    }
    std::sort(s.building_candidates.begin(), s.building_candidates.end(),
              [&](auto a, auto b) { return s.slots[a].building_order < s.slots[b].building_order; });
    for (auto id : s.building_candidates)
        output.push_back(s.slots[id].object);
}
bool query_selectable_indices(const RectangleStruct& bounds, std::vector<std::uint32_t>& result) {
    if (!state) return false;
    auto& s = *state;
    auto& tactical = *TacticalClass::Instance;
    if (!s.anchors_valid || s.anchor_count != tactical.SelectableCount) {
        for (int i = 0; i < tactical.SelectableCount; ++i) {
            const auto& e = TacticalClass::SelectableObjects[i];
            if (e.Object)
                s.anchors.set(std::uint32_t(i), e.X, e.Y);
            else
                s.anchors.erase(std::uint32_t(i));
        }
        for (int i = tactical.SelectableCount; i < s.anchor_count; ++i)
            s.anchors.erase(std::uint32_t(i));
        s.anchors_valid = true;
        s.anchor_count = tactical.SelectableCount;
    }
    s.anchors.query(std::int64_t(bounds.X) + tactical.TacticalPos.X, std::int64_t(bounds.Y) + tactical.TacticalPos.Y,
                    std::int64_t(bounds.X) + tactical.TacticalPos.X + bounds.Width,
                    std::int64_t(bounds.Y) + tactical.TacticalPos.Y + bounds.Height, result);
    std::sort(result.begin(), result.end());
    return true;
}
void verify_tactical_selectables(TacticalClass& tactical) {
    if (!state || !verify_spatial()) return;
    static std::uint64_t frames = 0, entries = 0;
    std::vector<TacticalSelectableStruct> reference(TacticalClass::SelectableObjects,
                                                    TacticalClass::SelectableObjects + tactical.SelectableCount);
    rebuild_tactical_selectables(tactical);
    if (reference.size() != std::size_t(tactical.SelectableCount)) {
        std::fprintf(stderr, "[spatial-reference] count original=%zu query=%d\n", reference.size(),
                     tactical.SelectableCount);
        const auto kinds = [](const TacticalSelectableStruct* entries, std::size_t count) {
            int buildings = 0, others = 0;
            for (std::size_t i = 0; i < count; ++i)
                (entries[i].Object && entries[i].Object->WhatAmI() == AbstractType::Building ? buildings : others)++;
            return std::pair<int, int>{buildings, others};
        };
        const auto before = kinds(reference.data(), reference.size());
        const auto after = kinds(TacticalClass::SelectableObjects, std::size_t(tactical.SelectableCount));
        std::fprintf(stderr, "[spatial-reference] buildings/others original=%d/%d query=%d/%d\n", before.first,
                     before.second, after.first, after.second);
        throw std::runtime_error("Independent selectable count differs from original drawing");
    }
    for (std::size_t i = 0; i < reference.size(); ++i) {
        const auto& expected = reference[i];
        const auto& actual = TacticalClass::SelectableObjects[i];
        if (expected.Object != actual.Object || expected.X != actual.X || expected.Y != actual.Y) {
            std::fprintf(stderr, "[spatial-reference] entry=%zu original=%p,%d,%d query=%p,%d,%d\n", i,
                         static_cast<void*>(expected.Object), expected.X, expected.Y, static_cast<void*>(actual.Object),
                         actual.X, actual.Y);
            throw std::runtime_error("Independent selectable anchor/order differs from original drawing");
        }
    }
    ++frames;
    entries += reference.size();
    if (frames == 1 || frames % 600 == 0)
        std::fprintf(stderr, "[spatial-reference] frames=%llu entries=%llu failures=0\n",
                     static_cast<unsigned long long>(frames), static_cast<unsigned long long>(entries));
}
}
