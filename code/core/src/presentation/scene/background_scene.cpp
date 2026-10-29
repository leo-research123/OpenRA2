#include "presentation/recorder.hpp"
#include "background_invalidation.hpp"
#include "alpha_notifications.hpp"
#include "convert_revision.hpp"
#include "tactical_drawing.hpp"
#include "presentation/builders/cell.hpp"
#include "presentation/builders/alpha.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include "overlay_drawing.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/Surface.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/LightConvertClassHelpers.hpp"
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/FileSystem.h"
#include "debug/debug_state.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <vector>

namespace game {
namespace {
// Center each dirty item in a camera-independent recording view. Supported
// source frames are at most 4096 pixels; the bridge rejects any clipped item.
// No Surface or viewport-sized pixel storage is allocated for this view.
constexpr RectangleStruct RecordingBounds{0, 0, 8192, 8192};
bool complete(DrawingStatus s) { return s == DrawingStatus::drawn || s == DrawingStatus::skipped; }
BackgroundSceneTint scene_tint(int red, int green, int blue, int shades) noexcept {
    BackgroundSceneTint tint{red, green, blue, shades};
    tint.factor_red = light_palette_factor(red);
    tint.factor_green = light_palette_factor(green);
    tint.factor_blue = light_palette_factor(blue);
    return tint;
}
// The terrain palette inputs CellClass::DrawIt (tint[0]) and world sprites
// (tint[1]) derive from the cell, as in CellClassDrawing.cpp and
// generic terminal requests.
BackgroundSceneLight scene_light(const CellClass& cell, int quality) noexcept {
    BackgroundSceneLight light;
    light.cell = cell.MapCoords;
    light.normal = std::uint16_t(cell.Intensity_Normal);
    light.terrain = std::uint16_t(cell.Intensity_Terrain);
    int red = cell.Color2_Red, green = cell.Color2_Green, blue = cell.Color2_Blue;
    const int shades = LightConvertClass::PrepareCellTint(red, green, blue, quality);
    light.tint[1] = scene_tint(red, green, blue, shades);
    const bool untinted = cell.Color2_Red == 1000 && cell.Color2_Green == 1000 && cell.Color2_Blue == 1000;
    light.tint[0] = untinted ? scene_tint(1000, 1000, 1000, 53) : light.tint[1];
    // A global tint replaces both conventions with the converter's table inputs.
    int shades_tinted;
    if (game::cell_converter_tint(cell, red, green, blue, shades_tinted))
        light.tint[0] = light.tint[1] = scene_tint(red, green, blue, shades_tinted);
    return light;
}
struct Recording {
    PresentationFrame& frame;
    const BackgroundInvalidationView& changes;
    const std::vector<CellStruct>& stale;
    BackgroundSceneRequest request;
    TerrainSpriteLayer& layer;
    bool full = false;
    unsigned cells = 0, objects = 0;
    std::array<BackgroundRecordInfo, TerrainSpriteLayer::MaxRecords> infos{};
    std::vector<BackgroundSlotAddress> addresses, releases;
    std::vector<TerrainSpriteLayer::InstanceInfo> created;
    DrawingStatus send(BackgroundSceneOperation op) {
        request.operation = op;
        return submit_background_scene(frame.drawing->types, request);
    }
    // The host writes the slots the layer chose for this item (place). The item's records are pending in
    // the host after item_end; an item that recorded nothing has no pending record.
    void place(const TerrainSpriteLayer::ItemKey& key, CellStruct anchor, std::uint32_t count) {
        if (!count && !layer.holds(key)) return;
        if (!layer.place(key, TerrainSpriteLayer::cell_index(anchor.X, anchor.Y), infos.data(), count, addresses,
                         releases)) {
            record_tactical_drawing(frame, DrawingStatus::unsupported);
            return;
        }
        layer.take_created(created);
        for (const auto& instance : created) {
            request.instance = instance.id;
            request.instance_slots = instance.slots;
            request.instance_state = instance.state;
            record_tactical_drawing(frame, send(BackgroundSceneOperation::instance));
        }
        bool changed = false;
        request.slots = addresses.data();
        request.slot_count = std::uint32_t(addresses.size());
        request.releases = releases.data();
        request.release_count = std::uint32_t(releases.size());
        request.changed = &changed;
        record_tactical_drawing(frame, send(BackgroundSceneOperation::place));
        request.slots = request.releases = nullptr;
        request.slot_count = request.release_count = 0;
        request.changed = nullptr;
        if (!complete(frame.status) || !changed) return;
        layer.mark_dirty(addresses.data(), addresses.size());
        layer.mark_dirty(releases.data(), releases.size());
    }
    // Ends the item the host has been recording and takes its record descriptions.
    std::uint32_t finish() {
        std::uint32_t count = 0;
        request.infos = infos.data();
        request.info_capacity = std::uint32_t(infos.size());
        request.info_count = &count;
        record_tactical_drawing(frame, send(BackgroundSceneOperation::item_end));
        request.infos = nullptr;
        request.info_capacity = 0;
        request.info_count = nullptr;
        return count;
    }
    PresentationContext* context = nullptr;
    void item(CellStruct at, unsigned layer_number, std::uint64_t object, void (*draw)(void*), void* argument) {
        if (!complete(frame.status)) return;
        auto& tactical = *TacticalClass::Instance;
        const auto flat = TacticalClass::CoordsToScreen({at.X * 256, at.Y * 256, 0});
        tactical.TacticalPos = {flat.X - 4096, flat.Y - 4096};
        request.cell = at;
        request.layer = layer_number;
        request.object = object;
        request.origin = tactical.TacticalPos;
        record_tactical_drawing(frame, send(BackgroundSceneOperation::item_begin));
        if (!complete(frame.status)) return;
        take_background_item_culled(frame);
        draw(argument);
        submit_tactical_background(*context);
        if (take_background_item_culled(frame)) record_tactical_drawing(frame, DrawingStatus::unsupported);
        frame.pending_background.clear();
        const auto count = finish();
        if (!complete(frame.status)) return;
        place({layer_number, object ? TerrainSpriteLayer::NoCell : TerrainSpriteLayer::cell_index(at.X, at.Y), object},
              at, count);
    }
    // Forgets a terrain object that left the layer.
    void erase(std::uintptr_t object) {
        request.cell = {};
        request.layer = 4;
        request.object = object;
        record_tactical_drawing(frame, send(BackgroundSceneOperation::item_begin));
        if (!complete(frame.status)) return;
        finish();
        if (complete(frame.status)) place({4, TerrainSpriteLayer::NoCell, object}, {}, 0);
    }
    void cell(CellClass& cell, unsigned mask) {
        ++cells;
        const auto at = cell.MapCoords;
        struct Call {
            Recording& self;
            CellClass& cell;
            unsigned layer;
        } call{*this, cell, 0};
        for (unsigned layer : {1u, 2u, 3u, 5u}) {
            if (!(mask & (1u << (layer - 1)))) continue;
            call.layer = layer;
            item(
                at, layer, 0,
                [](void* p) {
                    auto& c = *static_cast<Call*>(p);
                    const Point2D point{4066, 4096};
                    auto& ctx = *c.self.context;
                    if (c.layer == 1)
                        record_tactical_drawing(
                            c.self.frame, draw_cell_terrain(c.cell, *c.self.frame.drawing, point, RecordingBounds));
                    else if (c.layer == 5)
                        build_cell_shadow_cast(ctx, c.cell, point, RecordingBounds);
                    else if (c.cell.OverlayTypeIndex != -1) {
                        // Recording is independent of the original per-view bridge redraw stamp.
                        struct RestoreStamp {
                            CellClass& cell;
                            RectangleStruct rect;
                            unsigned frame;
                            char epoch;
                            ~RestoreStamp() {
                                cell.InViewportRect = rect;
                                cell.RedrawFrame = frame;
                                cell.unknown_118 = epoch;
                            }
                        } restore{c.cell, c.cell.InViewportRect, c.cell.RedrawFrame, c.cell.unknown_118};
                        c.cell.InViewportRect = {};
                        if (c.layer == 2)
                            build_cell_overlay(ctx, c.cell, point, RecordingBounds);
                        else
                            build_cell_overlay_shadow(ctx, c.cell, point, RecordingBounds);
                        if (const auto* drawing = ctx.overlay) record_tactical_drawing(c.self.frame, drawing->status);
                    }
                },
                &call);
        }
    }
    void record() {
        auto& map = MapClass::Instance;
        auto& w = *frame.world->impl;
        std::unordered_set<unsigned> terrainCells;
        bool terrain = full || changes.object_count;
        if (full) {
            for (int i = 0; i < map.Cells.Capacity && complete(frame.status); ++i)
                if (auto* c = map.Cells.Items[i]; c && map.CoordinatesLegal(c->MapCoords)) cell(*c, 0x1F);
        } else {
            // Light-only changes were relit by the host; stale cells it could not
            // relight are recorded again with every layer.
            const auto visit = [&](CellStruct at, unsigned mask) {
                if (mask & 8) {
                    terrain = true;
                    terrainCells.insert(unsigned(at.Y) * 512 + at.X);
                }
                if (!(mask & 0x17)) return;
                if (auto* c = map.TryGetCellAt(at); c && map.CoordinatesLegal(at)) cell(*c, mask);
            };
            std::unordered_set<unsigned> staleCells;
            for (const auto& at : stale)
                staleCells.insert(unsigned(at.Y) * 512 + at.X);
            for (unsigned i = 0; i < changes.cell_count && complete(frame.status); ++i) {
                const auto& dirty = changes.cells[i];
                const auto index = unsigned(dirty.cell.Y) * 512 + dirty.cell.X;
                if (staleCells.erase(index)) {
                    visit(dirty.cell, 0x1F);
                    continue;
                }
                visit(dirty.cell, dirty.layers & 0x1F);
            }
            for (const auto& at : stale)
                if (staleCells.count(unsigned(at.Y) * 512 + at.X) && complete(frame.status)) visit(at, 0x1F);
        }
        if (!terrain) return;
        std::unordered_set<std::uintptr_t> dirtyObjects;
        for (unsigned i = 0; i < changes.object_count; ++i)
            dirtyObjects.insert(changes.objects[i]);
        if (full) w.presentation.background_objects.clear();
        std::unordered_set<std::uintptr_t> live;
        auto& objectLayer = MapClass::ObjectsInLayers[int(Layer::Ground)];
        // Resolve comparison-only identities against the live original layer. Never
        // dereference a removed terrain pointer. A terrain object owns the slots of the
        // cell it is anchored to; the original equal-depth order between objects of one
        // row is not kept (cells of a row are ordered by X).
        for (int i = 0; i < objectLayer.Count && complete(frame.status); ++i) {
            auto* object = objectLayer[i];
            if (object->WhatAmI() != AbstractType::Terrain) continue;
            auto* t = static_cast<TerrainClass*>(object);
            if (t->IsDead() || !t->Type || t->Type->IsAnimated || t->IsCrumbling) continue;
            const auto id = reinterpret_cast<std::uintptr_t>(object);
            live.insert(id);
            const bool inserted = w.presentation.background_objects.insert(id).second;
            CellStruct at;
            object->GetMapCoords(&at);
            if (!full && !inserted && !dirtyObjects.count(id) && !terrainCells.count(unsigned(at.Y) * 512 + at.X))
                continue;
            ++objects;
            struct Terrain {
                Recording& self;
                ObjectClass& object;
            } terrain_call{*this, *object};
            item(
                at, 4, id,
                [](void* p) {
                    auto& c = *static_cast<Terrain*>(p);
                    record_tactical_drawing(c.self.frame,
                                            draw_tactical_object(*c.self.context, c.object, true, RecordingBounds, false));
                },
                &terrain_call);
        }
        for (auto it = w.presentation.background_objects.begin(); it != w.presentation.background_objects.end();) {
            if (live.count(*it) || !complete(frame.status)) {
                ++it;
                continue;
            }
            erase(*it);
            it = w.presentation.background_objects.erase(it);
        }
    }
};
}
namespace {
std::uint64_t alpha_shape_revision = 1;
// CellClass::DrawShroudAndFog's SHROUD.SHP frame of an occlusion state.
int shroud_frame(int state) noexcept { return state == -2 ? 15 : state == -1 ? 0 : state; }
// A failed transaction leaves no partial scene: the host aborted it, the next
// successful attempt records the whole map, and this frame's caller draws the
// background immediately. A world that failed once keeps drawing immediately,
// so one unsupported request cannot cost a full recording every frame.
DrawingStatus fail_background_scene(MapWorld& world, DrawingStatus status) noexcept {
    if (!world.impl->presentation.background_scene_failed)
        std::fprintf(stderr, "BACKGROUND_SCENE_DISABLED status=%s\n", drawing_status_name(status));
    world.impl->presentation.background_scene_failed = true;
    invalidate_background_all();
    return DrawingStatus::unavailable;
}
}
DrawingStatus draw_background_scene(PresentationContext& outer_context, const BackgroundInvalidationView& changes) noexcept {
    auto* outer = outer_context.frame;
    if (!outer || !outer->drawing || !outer->world || !supports_drawing(outer->drawing->types, DrawingKind::background_scene))
        return DrawingStatus::unavailable;
    if (outer->world->impl->presentation.background_scene_failed) return DrawingStatus::unavailable;
    try {
        BackgroundSceneRequest request;
        request.target = outer->drawing->types.target;
        request.world = outer->world->impl->world_id;
        request.lighting_quality = outer->drawing->lighting_quality;
        auto send = [&](BackgroundSceneOperation op) {
            request.operation = op;
            return submit_background_scene(outer->drawing->types, request);
        };
        const auto started = send(BackgroundSceneOperation::begin);
        if (started != DrawingStatus::unavailable && !complete(started))
            return fail_background_scene(*outer->world, started);
        const bool full = started == DrawingStatus::unavailable || changes.all;
        auto& map = MapClass::Instance;
        auto& layer = outer->world->impl->presentation.background_layer;
        if (full) {
            const auto status = send(BackgroundSceneOperation::reset);
            if (!complete(status)) return fail_background_scene(*outer->world, status);
            // The slot layout covers the map cells of this recording; items fill it as they are recorded.
            static std::vector<CellStruct> layout;
            layout.clear();
            for (int i = 0; i < map.Cells.Capacity; ++i)
                if (auto* c = map.Cells.Items[i]; c && map.CoordinatesLegal(c->MapCoords))
                    layout.push_back(c->MapCoords);
            layer.reset(layout.data(), layout.size(), TacticalClass::CoordsToScreen({0, 0, 0}).Y);
        }
        // Lighting first: the host relights held items and infers the light
        // sources of items recorded below from these values.
        // Reused across frames: a global change sends every cell. The core is the
        // only caller and does not re-enter.
        static std::vector<BackgroundSceneLight> lights;
        static std::vector<CellStruct> stale;
        lights.clear();
        stale.clear();
        const auto quality = outer->drawing->lighting_quality;
        if (full || changes.global_lighting) {
            for (int i = 0; i < map.Cells.Capacity; ++i)
                if (auto* c = map.Cells.Items[i]; c && map.CoordinatesLegal(c->MapCoords))
                    lights.push_back(scene_light(*c, quality));
        } else
            for (unsigned i = 0; i < changes.cell_count; ++i) {
                const auto& dirty = changes.cells[i];
                if (auto* c = map.TryGetCellAt(dirty.cell); c && map.CoordinatesLegal(dirty.cell))
                    lights.push_back(scene_light(*c, quality));
            }
        if (!lights.empty()) {
            std::uint32_t staleCount = 0;
            stale.resize(lights.size());
            request.lights = lights.data();
            request.light_count = std::uint32_t(lights.size());
            request.stale = stale.data();
            request.stale_capacity = std::uint32_t(stale.size());
            request.stale_count = &staleCount;
            const auto status = send(BackgroundSceneOperation::lighting);
            request.lights = nullptr;
            request.light_count = request.stale_capacity = 0;
            request.stale = nullptr;
            request.stale_count = nullptr;
            if (!complete(status)) return fail_background_scene(*outer->world, status);
            stale.resize(std::min<std::size_t>(staleCount, stale.size()));
            if (full) stale.clear();
        }
        // Converter tables are referenced, not copied, by held items.
        const ConvertClass* const* converters = nullptr;
        std::uint32_t converter_count = 0;
        take_changed_converters(converters, converter_count);
        for (std::uint32_t i = 0; i < converter_count; ++i) {
            request.converter = converters[i];
            request.revision = converter_revision(*converters[i]);
            const auto status = send(BackgroundSceneOperation::converter);
            if (!complete(status)) return fail_background_scene(*outer->world, status);
        }
        request.converter = nullptr;
        request.revision = 0;
        // Shroud frames: all of them after a reset, otherwise the cells whose
        // occlusion draw_world_shroud refreshed this frame.
        auto& world = *outer->world->impl;
        if (world.presentation.occlusion_ready && world_shroud_available(*outer) && (full || !world.presentation.shroud_changes.empty())) {
            if (full) {
                world.presentation.shroud_changes.clear();
                for (int i = 0; i < map.Cells.Capacity; ++i)
                    if (auto* c = map.Cells.Items[i]; c && map.CoordinatesLegal(c->MapCoords)) {
                        const auto at = c->MapCoords;
                        world.presentation.shroud_changes.push_back(
                            {at, world.presentation.occlusion_hidden
                                     ? -1
                                     : shroud_frame(world.presentation.occlusion_shroud[std::size_t(at.Y) * 512 + at.X])});
                    }
            }
            request.shroud = world.presentation.shroud_changes.data();
            request.shroud_count = std::uint32_t(world.presentation.shroud_changes.size());
            const auto status = send(BackgroundSceneOperation::shroud);
            request.shroud = nullptr;
            request.shroud_count = 0;
            world.presentation.shroud_changes.clear();
            if (!complete(status)) return fail_background_scene(*outer->world, status);
        }
        // AlphaShapes: the whole set whenever AlphaShapeClass::Array or the bounds
        // origin changed, positioned as AlphaShapeClass::DrawAll draws them.
        const Point2D alphaOrigin{outer->bounds.X, outer->bounds.Y};
        if (world_shroud_available(*outer) && (full || !world.presentation.alpha_sent || world.presentation.alpha_revision != alpha_shape_revision ||
                                         world.presentation.alpha_origin != alphaOrigin)) {
            static std::vector<BackgroundAlphaShape> alphas;
            alphas.clear();
            for (auto* shape : AlphaShapeClass::Array) {
                if (auto* image = alpha_presentation_image(&world.presentation, *shape))
                    alphas.push_back({shape, image, {shape->Rect.X + alphaOrigin.X, shape->Rect.Y + alphaOrigin.Y}});
            }
            request.alphas = alphas.data();
            request.alpha_count = std::uint32_t(alphas.size());
            const auto status = send(BackgroundSceneOperation::alpha);
            request.alphas = nullptr;
            request.alpha_count = 0;
            if (!complete(status)) return fail_background_scene(*outer->world, status);
            world.presentation.alpha_sent = true;
            world.presentation.alpha_revision = alpha_shape_revision;
            world.presentation.alpha_origin = alphaOrigin;
        }
        if (std::getenv("RA2_MAP_PROFILE") && (!lights.empty() || converter_count))
            std::fprintf(stderr, "BACKGROUND_LIGHT cells=%zu stale=%zu converters=%u\n", lights.size(), stale.size(),
                         converter_count);
        bool work = full || changes.object_count || !stale.empty();
        for (unsigned i = 0; !work && i < changes.cell_count; ++i)
            work = (changes.cells[i].layers & 0x1F) != 0;
        DrawingStatus status = DrawingStatus::skipped;
        if (work) {
            const auto started = std::chrono::steady_clock::now();
            auto& tactical = *TacticalClass::Instance;
            const auto camera = tactical.TacticalPos;
            const auto bounds = TacticalClass::ViewBounds;
            const auto surface = DSurface::ViewBounds;
            struct Restore {
                TacticalClass& tactical;
                Point2D camera;
                RectangleStruct bounds, surface;
                ~Restore() {
                    tactical.TacticalPos = camera;
                    TacticalClass::ViewBounds = bounds;
                    DSurface::ViewBounds = surface;
                }
            } restore{tactical, camera, bounds, surface};
            TacticalClass::ViewBounds = RecordingBounds;
            DSurface::ViewBounds = RecordingBounds;
            auto drawing = *outer->drawing;
            drawing.types.overlay_offset = {};
            drawing.types.buffer_offset_y = 0;
            MapDrawStatistics stats;
            auto frame = *outer;
            frame.drawing = &drawing;
            frame.bounds = RecordingBounds;
            frame.statistics = &stats;
            frame.include_terrain = false;
            frame.recording_background = true;
            frame.status = DrawingStatus::skipped;
            Recording recording{frame, changes, stale, request, layer, full};
            status = with_tactical_frame(
                frame,
                [](void* p, PresentationContext& ctx) {
                    auto& r = *static_cast<Recording*>(p);
                    r.frame.status = with_tactical_render_resources(
                        r.frame, false,
                        [](void* q, PresentationContext& scoped) {
                            auto& r = *static_cast<Recording*>(q);
                            r.context = &scoped;
                            r.record();
                        },
                        p, ctx);
                },
                &recording, outer_context);
            if (std::getenv("RA2_MAP_PROFILE"))
                std::fprintf(stderr, "BACKGROUND_RECORD full=%d cells=%u objects=%u record_us=%lld\n", full,
                             recording.cells, recording.objects,
                             static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(
                                                        std::chrono::steady_clock::now() - started)
                                                        .count()));
        }
        if (!complete(status)) {
            send(BackgroundSceneOperation::abort);
            return fail_background_scene(*outer->world, status);
        }
        // The slots and draw orders this transaction changed, then the ranges the viewport touches.
        static std::vector<BackgroundSlotRange> dirty, visible, window;
        static std::vector<BackgroundOrderUpdate> orders;
        layer.take_dirty(dirty, orders);
        request.dirty = dirty.data();
        request.dirty_count = std::uint32_t(dirty.size());
        request.orders = orders.data();
        request.order_count = std::uint32_t(orders.size());
        status = send(BackgroundSceneOperation::commit);
        request.dirty = nullptr;
        request.dirty_count = 0;
        request.orders = nullptr;
        request.order_count = 0;
        if (!complete(status)) {
            send(BackgroundSceneOperation::abort);
            return fail_background_scene(*outer->world, status);
        }
        request.origin = TacticalClass::Instance->TacticalPos;
        request.clip = outer->bounds;
        const int top = request.origin.Y + request.clip.Y, bottom = top + request.clip.Height;
        layer.ranges(top, bottom, 0, visible);
        layer.ranges(top, bottom, BackgroundWindowMargin, window);
        // A smudge without Z testing may overlap the following TMP. Grouping all
        // TMPs before all smudges changes those pixels even when each draw is exact.
        layer.order_cell_ranges(1, visible);
        layer.order_cell_ranges(1, window);
        request.visible = visible.data();
        request.window = window.data();
        request.draw_count = std::uint32_t(visible.size());
        status = send(BackgroundSceneOperation::draw);
        request.visible = request.window = nullptr;
        request.draw_count = 0;
        if (!complete(status)) {
            send(BackgroundSceneOperation::abort);
            return fail_background_scene(*outer->world, status);
        }
        return status;
    } catch (...) {
        return fail_background_scene(*outer->world, DrawingStatus::backend_failure);
    }
}
}
namespace game {
void alpha_shapes_changed(const AlphaShapeClass* expired) noexcept {
    ++alpha_shape_revision;
    if (expired) if (auto* world = current_map_world()) world->impl->presentation.alpha_images.erase(expired);
}
bool world_shroud_available(const PresentationFrame& frame_ref) noexcept {
    static const bool enabled = [] {
        const char* v = std::getenv("RA2_MAP_WORLD_SHROUD");
        return !v || std::strcmp(v, "0") != 0;
    }();
    const auto* frame = &frame_ref;
    return enabled && frame->drawing && frame->world && frame->draw_world &&
           supports_drawing(frame->drawing->types, DrawingKind::background_scene) && supports_drawing(frame->drawing->types, DrawingKind::lighting_shape_batch) &&
           !frame->world->impl->presentation.background_scene_failed;
}
DrawingStatus draw_world_shroud(PresentationContext& ctx, TacticalClass& tactical,
                                const BackgroundInvalidationView& changes, const RectangleStruct& bounds) noexcept {
    auto* frame = ctx.frame;
    if (!frame || !frame->world) return DrawingStatus::unavailable;
    auto& w = *frame->world->impl;
    auto& map = MapClass::Instance;
    constexpr int Span = 512;
    const bool batched = begin_lighting_batch(ctx);
    try {
        const bool hidden = debug_fog_hidden();


        const auto refresh = [&](CellClass& cell) {
            const auto at = cell.MapCoords;
            const auto index = std::size_t(at.Y) * Span + at.X;
            const auto shroud = char(tactical.GetOcclusion(at, false)), fog = char(tactical.GetOcclusion(at, true));
            w.presentation.occlusion_shroud[index] = std::int8_t(shroud);
            w.presentation.occlusion_fog[index] = std::int8_t(fog);
            w.presentation.shroud_changes.push_back({at, hidden ? -1 : shroud_frame(shroud)});
        };
        if (!w.presentation.occlusion_ready || changes.all || hidden != w.presentation.occlusion_hidden) {
            w.presentation.occlusion_shroud.assign(std::size_t(Span) * Span, 0);
            w.presentation.occlusion_fog.assign(std::size_t(Span) * Span, 0);
            w.presentation.shroud_changes.clear();
            for (int i = 0; i < map.Cells.Capacity; ++i)
                if (auto* c = map.Cells.Items[i]; c && map.CoordinatesLegal(c->MapCoords)) refresh(*c);
            w.presentation.occlusion_ready = true;
            w.presentation.occlusion_hidden = hidden;
        } else {
            for (unsigned i = 0; i < changes.cell_count; ++i) {
                const auto& dirty = changes.cells[i];
                if (!(dirty.layers & BackgroundShroud) || !map.CoordinatesLegal(dirty.cell)) continue;
                auto* cell = map.TryGetCellAt(dirty.cell);
                if (!cell) continue;
                refresh(*cell);

            }
        }

        // The scene holds the AlphaShapes (BackgroundSceneOperation::alpha).
        if (!batched) {
            auto alpha = inherit_scopes(ctx);
            build_alpha_shapes(alpha, bounds);
            return DrawingStatus::unavailable;
        }
        const auto flat = TacticalClass::CoordsToScreen({0, 0, 0});
        lighting_batch_world_shroud(ctx, static_cast<SHPStruct*>(FileSystem::LoadFile("SHROUD.SHP", true)),
                                    {flat.X - tactical.TacticalPos.X - 30, bounds.Y + flat.Y - tactical.TacticalPos.Y},
                                    tactical.TacticalPos);
        const auto* current = ctx.frame;
        if (!current || !complete(current->status)) {
            cancel_lighting_batch(ctx);
            return current ? current->status : DrawingStatus::unavailable;
        }
        return end_lighting_batch(ctx, bounds);
    } catch (...) {
        if (batched) cancel_lighting_batch(ctx);
        return DrawingStatus::backend_failure;
    }
}
}
