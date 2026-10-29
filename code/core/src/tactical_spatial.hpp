#pragma once
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/GeneralStructures.h"
#include <cstdint>
#include <vector>
class ObjectClass;
class LayerClass;
class TacticalClass;
namespace game {
#if defined(RA2_YRPP_GAME) || defined(RA2_IMAGE_GAME)
inline std::uint64_t tactical_spatial_layer_revision() noexcept { return 0; }
inline bool tactical_spatial_unique_entries() noexcept { return false; }
#else
std::uint64_t tactical_spatial_layer_revision() noexcept;
bool tactical_spatial_unique_entries() noexcept;
#endif
// Private caches of the original registries, never authoritative game objects.
// Allocation failure aborts the host operation; it must not silently omit an object.
void start_tactical_spatial();
void stop_tactical_spatial() noexcept;
bool tactical_spatial_active() noexcept;
std::uint64_t tactical_spatial_generation() noexcept;
#if defined(RA2_YRPP_GAME) || defined(RA2_IMAGE_GAME)
// Original image/probe builds keep the EXE's registries and traversal.
inline void tactical_spatial_dirty(ObjectClass*) noexcept {}
inline void tactical_spatial_dirty(const ObjectClass*) noexcept {}
inline void tactical_spatial_forget(ObjectClass*) noexcept {}
inline void tactical_spatial_building(ObjectClass*) noexcept {}
inline void tactical_spatial_insert(LayerClass*, int, ObjectClass*) noexcept {}
inline void tactical_spatial_remove(LayerClass*, int) noexcept {}
inline void tactical_spatial_swap(LayerClass*, int, int) noexcept {}
inline void tactical_spatial_reconcile() noexcept {}
#else
void tactical_spatial_dirty(ObjectClass*) noexcept;
// For callers that only hold a const object: the index is keyed by identity, nothing is written to the object.
void tactical_spatial_dirty(const ObjectClass*) noexcept;
void tactical_spatial_forget(ObjectClass*) noexcept;
void tactical_spatial_building(ObjectClass*) noexcept;
void tactical_spatial_insert(LayerClass*, int, ObjectClass*) noexcept;
void tactical_spatial_remove(LayerClass*, int) noexcept;
void tactical_spatial_swap(LayerClass*, int, int) noexcept;
void tactical_spatial_reconcile() noexcept;
#endif
void flush_tactical_spatial();
struct TacticalWorldBounds {
    int left = 0, top = 0, right = 0, bottom = 0;
};
TacticalWorldBounds tactical_object_bounds(TacticalClass&, const RectangleStruct&);
// Reused per nested invocation. next() restores original layer order and
// refreshes candidates after live-list changes, preserving shifted successors.
class TacticalLayerQuery {
    std::vector<std::uint32_t> candidates, scratch;
    std::vector<std::uint64_t> membership;
    TacticalWorldBounds bounds;
    std::uint64_t revision = 0;
    int layer = 0, next_index = 0;
    std::size_t cursor = 0;
    bool include_visible = false;
    void refresh();

public:
    TacticalLayerQuery();
    ~TacticalLayerQuery();
    TacticalLayerQuery(const TacticalLayerQuery&) = delete;
    TacticalLayerQuery& operator=(const TacticalLayerQuery&) = delete;
    void begin(int, const TacticalWorldBounds&, bool);
    bool next(ObjectClass*& object, int& original_index);
};
void tactical_spatial_visible(ObjectClass*) noexcept;
void collect_tactical_buildings(const RectangleStruct&, std::vector<ObjectClass*>&);
void collect_visible_tactical_buildings(std::vector<ObjectClass*>&);
void rebuild_tactical_selectables(TacticalClass&);
void verify_tactical_selectables(TacticalClass&);
// The selectable table remains the original fixed-capacity ordered table.
// These helpers accelerate queries without changing ties, capacity or callbacks.
void invalidate_selectable_index() noexcept;
std::uint64_t selectable_index_revision() noexcept;
bool query_selectable_indices(const RectangleStruct&, std::vector<std::uint32_t>&);
}
