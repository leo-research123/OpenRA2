#include "background_invalidation.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Unsorted.h"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace game {
namespace {
constexpr std::uint32_t All = std::uint32_t(BackgroundLayerMask::all);
struct Field {
    const char* name;
    std::uint32_t layers;
};
constexpr Field CellFields[] = {{"IsoTileTypeIndex", All},
                                {"Height", All},
                                {"Level", All},
                                {"SlopeIndex", All},
                                {"OverlayTypeIndex", 0x6},
                                {"OverlayData", 0x6},
                                {"SmudgeTypeIndex", 0x1},
                                {"SmudgeData", 0x1},
                                {"Flags", All | BackgroundShroud},
                                {"AltFlags", All | BackgroundShroud},
                                {"Rubble", 0x6},
                                {"BridgeOwnerCell", All},
                                {"LightConvert", BackgroundLighting},
                                {"Intensity_Normal", BackgroundLighting},
                                {"Intensity_Terrain", BackgroundLighting},
                                {"Color1_Blue", BackgroundLighting},
                                {"Color2_Red", BackgroundLighting},
                                {"Color2_Green", BackgroundLighting},
                                {"Color2_Blue", BackgroundLighting},
                                {"ShroudOcclusion", BackgroundShroud},
                                {"FogOcclusion", BackgroundShroud},
                                {"WallOwnerIndex", 0x6}};
using CellRecord = std::array<std::uint64_t, std::size(CellFields)>;
std::uint64_t pointer(const void* p) noexcept { return reinterpret_cast<std::uintptr_t>(p); }
CellRecord record(const CellClass& c) noexcept {
    // Height is the original TMP sub-tile byte at 0x11A; there is no separate
    // IsoTileTypeSubIndex member. Recompute occlusion without writing drawing
    // caches. Counter-only changes need no notification until coverage changes.
    return {std::uint32_t(c.IsoTileTypeIndex),
            std::uint8_t(c.Height),
            std::uint8_t(c.Level),
            c.SlopeIndex,
            std::uint32_t(c.OverlayTypeIndex),
            c.OverlayData,
            std::uint32_t(c.SmudgeTypeIndex),
            c.SmudgeData,
            std::uint32_t(c.Flags),
            std::uint32_t(c.AltFlags),
            pointer(c.Rubble),
            pointer(c.BridgeOwnerCell),
            pointer(c.LightConvert),
            c.Intensity_Normal,
            c.Intensity_Terrain,
            c.Color1_Blue,
            c.Color2_Red,
            c.Color2_Green,
            c.Color2_Blue,
            TacticalClass::Instance ? std::uint8_t(TacticalClass::Instance->GetOcclusion(c.MapCoords, false)) : 0u,
            TacticalClass::Instance ? std::uint8_t(TacticalClass::Instance->GetOcclusion(c.MapCoords, true)) : 0u,
            std::uint32_t(c.WallOwnerIndex)};
}
constexpr const char* ObjectFields[] = {"Type",       "Health",         "Location.X",      "Location.Y",  "Location.Z",
                                        "IsOnMap",    "InLimbo",        "Animation.Value", "IsCrumbling", "IsBurning",
                                        "IsAnimated", "Animation.Rate", "IsAlive"};
using ObjectRecord = std::array<std::uint64_t, std::size(ObjectFields)>;
ObjectRecord record(const TerrainClass& t) noexcept {
    return {pointer(t.Type),
            std::uint32_t(t.Health),
            std::uint32_t(t.Location.X),
            std::uint32_t(t.Location.Y),
            std::uint32_t(t.Location.Z),
            t.IsOnMap,
            t.InLimbo,
            std::uint32_t(t.Animation.Value),
            t.IsCrumbling,
            t.IsBurning,
            t.Type && t.Type->IsAnimated,
            std::uint32_t(t.Animation.Rate),
            t.IsAlive};
}
struct Snapshot {
    CellRecord fields{};
    bool present = false;
    std::uint64_t geometry = 0, lighting = 0;
};
struct Audit {
    std::vector<Snapshot> cells;
    std::unordered_map<std::uintptr_t, ObjectRecord> objects;
    std::vector<std::uint32_t> dirty;
    bool initialized = false;
};
}
void audit_background_invalidations(const BackgroundInvalidationView* consumed) noexcept {
    static const bool enabled = [] {
        const auto* v = std::getenv("RA2_BG_LAYER_AUDIT");
        return v && std::strcmp(v, "1") == 0;
    }();
    if (!enabled) return;
    BackgroundInvalidationView changes;
    if (consumed)
        changes = *consumed;
    else
        take_background_invalidations(changes);
    try {
        static Audit audit;
        const auto capacity = std::size_t(MapClass::MaxCells);
        if (audit.cells.empty()) {
            audit.cells.resize(capacity);
            audit.dirty.resize(capacity);
        }
        std::fill(audit.dirty.begin(), audit.dirty.end(), 0);
        for (std::uint32_t i = 0; i < changes.cell_count; ++i) {
            const auto& c = changes.cells[i];
            audit.dirty[std::size_t(c.cell.Y) * 512 + c.cell.X] |= c.layers;
        }
        const int frame = Unsorted::CurrentFrame;
        unsigned missed_cells = 0, missed_objects = 0;
        const auto& map = MapClass::Instance;
        for (std::size_t i = 0; i < capacity; ++i) {
            const auto* cell = map.Cells.Items && i < std::size_t(map.Cells.Capacity) ? map.Cells.Items[i] : nullptr;
            auto& previous = audit.cells[i];
            const auto current = cell ? record(*cell) : CellRecord{};
            std::uint64_t geometry = 0, lighting = 0;
            if (cell) background_cell_records(*cell, geometry, lighting);
            std::uint32_t covered = changes.all ? 0x7F : audit.dirty[i];
            if (changes.global_lighting) covered |= BackgroundLighting;
            std::uint32_t missed = 0;
            std::array<bool, std::size(CellFields)> fields{};
            if (audit.initialized) {
                if (previous.present != bool(cell))
                    missed = All & ~covered;
                else if (cell &&
                         (geometry != previous.geometry || lighting != previous.lighting || current != previous.fields))
                    for (std::size_t f = 0; f < current.size(); ++f) {
                        auto layers = CellFields[f].layers;
                        const auto changed = previous.fields[f] ^ current[f];
                        // FlagToShroud (0x20) schedules a later transition; it is not an occlusion input.
                        // TacticalClass::GetOcclusion reads CenterRevealed (Flags 0x1) and NoFog (AltFlags
                        // 0x10) only while the cell's EdgeRevealed (0x2) / Mapped (0x8) bit is clear, and a
                        // neighbour contributes only that mapped bit. A changed occlusion value itself is
                        // reported through ShroudOcclusion / FogOcclusion, for the cell and its neighbours.
                        const auto stays = previous.fields[f] & current[f];
                        if (f == 8) {
                            const auto inputs = (changed & 0x40000Au) | (stays & 0x2u ? 0u : changed & 0x1u);
                            layers = (changed & ~0x40002Bu ? All : 0u) | (inputs ? BackgroundShroud : 0u);
                        }
                        if (f == 9) {
                            const auto inputs = (changed & 0x8u) | (stays & 0x8u ? 0u : changed & 0x10u);
                            layers = (changed & ~0x18u ? All : 0u) | (inputs ? BackgroundShroud : 0u);
                        }
                        fields[f] = changed && (layers & ~covered);
                        if (fields[f]) missed |= layers & ~covered;
                    }
            }
            if (missed) {
                ++missed_cells;
                std::fprintf(stderr, "BG_AUDIT frame=%d cell=%zu,%zu fields=", frame, i % 512, i / 512);
                bool first = true;
                if (previous.present != bool(cell)) {
                    std::fputs("membership", stderr);
                    first = false;
                }
                for (std::size_t f = 0; f < fields.size(); ++f)
                    if (fields[f]) {
                        std::fprintf(stderr, "%s%s", first ? "" : ",", CellFields[f].name);
                        first = false;
                    }
                std::fprintf(stderr, " layers=0x%X\n", missed);
            }
            previous = {current, bool(cell), geometry, lighting};
        }
        std::unordered_map<std::uintptr_t, ObjectRecord> current_objects;
        const auto notified = [&](std::uintptr_t id) {
            if (changes.all) return true;
            for (std::uint32_t i = 0; i < changes.object_count; ++i)
                if (changes.objects[i] == id) return true;
            return false;
        };
        for (const auto* terrain : TerrainClass::Array) {
            if (!terrain) continue;
            const auto id = reinterpret_cast<std::uintptr_t>(terrain);
            const auto current = record(*terrain);
            current_objects.emplace(id, current);
            const auto previous = audit.objects.find(id);
            if (!audit.initialized || notified(id)) continue;
            if (previous != audit.objects.end() && previous->second == current) continue;
            ++missed_objects;
            std::fprintf(stderr, "BG_AUDIT frame=%d object=0x%llX fields=", frame, static_cast<unsigned long long>(id));
            if (previous == audit.objects.end())
                std::fputs("added", stderr);
            else {
                bool first = true;
                for (std::size_t f = 0; f < current.size(); ++f)
                    if (previous->second[f] != current[f]) {
                        std::fprintf(stderr, "%s%s", first ? "" : ",", ObjectFields[f]);
                        first = false;
                    }
            }
            std::fputs(" layers=0x8\n", stderr);
        }
        for (const auto& old : audit.objects)
            if (current_objects.find(old.first) == current_objects.end() && !notified(old.first)) {
                ++missed_objects;
                std::fprintf(stderr, "BG_AUDIT frame=%d object=0x%llX fields=removed layers=0x8\n", frame,
                             static_cast<unsigned long long>(old.first));
            }
        audit.objects.swap(current_objects);
        audit.initialized = true;
        std::fprintf(stderr,
                     "BG_AUDIT_SUMMARY frame=%d missed_cells=%u missed_objects=%u invalidated_cells=%u "
                     "invalidated_objects=%u full_rebuild=%d global_lighting=%d\n",
                     frame, missed_cells, missed_objects, changes.cell_count, changes.object_count, changes.all,
                     changes.global_lighting);
    } catch (...) {
        // An incomplete audit is never reported as a successful zero-miss frame.
        std::fprintf(stderr, "BG_AUDIT_ERROR frame=%d reason=snapshot_failure\n", Unsorted::CurrentFrame);
    }
}
}
