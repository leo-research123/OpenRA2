// Host resource lookup, request storage and submission. Scene traversal and
// rendering decisions live in yrpp/TacticalClassRender.cpp.
#include "map_world_internal.hpp"
#include "convert_revision.hpp"
#include "techno_drawing.hpp"
#include "type_drawing.hpp"
#include "presentation/recorder.hpp"
#include "map_view.hpp"
#include "tactical_drawing.hpp"
#include "presentation/builders/cell.hpp"
#include "presentation/context.hpp"
#include "background_invalidation.hpp"
#include "yrpp/HouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/CellClass.h"
#include "yrpp/LightConvertClassHelpers.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/ColorScheme.h"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <bit>
#include <stdexcept>
namespace game {
namespace {
bool complete(DrawingStatus s) { return s == DrawingStatus::drawn || s == DrawingStatus::skipped; }
}
DrawingStatus choose_world_palette(MapWorld& world, const WorldPaletteRequest& request,
                                   WorldPaletteChoice& choice) noexcept {
    choice = {};
    try {
        auto& w = *world.impl;
        switch (request.source) {
            case WorldPaletteSource::selection:
                choice.palette = &w.presentation.selection_palette;
                break;
            case WorldPaletteSource::animation:
                choice.palette = &w.presentation.anim_palette;
                break;
            case WorldPaletteSource::cell:
                choice.palette = &FileSystem::ISOx_PAL;
                break;
            case WorldPaletteSource::color_scheme: {
                // 0x46B226 / 0x4683B5 indexes ColorScheme directly, including schemes with
                // no matching House. Preserve its original converter and shade count.
                const auto* scheme = ColorScheme::Array.GetItemOrDefault(request.scheme_index);
                choice.converter = scheme ? scheme->LightConvert : nullptr;
                break;
            }
            case WorldPaletteSource::techno_house: {
                if (!request.object) return DrawingStatus::invalid_argument;
                const auto& object = *request.object;
                auto* owner = request.house ? request.house : object.Owner;
                // Resource-free native maps can draw before a color registry is loaded.
                if (!ColorScheme::Array.Count) {
                    choice.palette = world_palette(world, w.presentation.unit_palette, owner);
                    break;
                }
                // 0x00705F87 / 0x007060C4: retain the selected original converter, including
                // custom palettes, shade count and disguise-house overrides.
                if (!(object.WhatAmI() == AbstractType::Unit && static_cast<const UnitClass&>(object).TerrainPalette) &&
                    object.IsClearlyVisibleTo(HouseClass::CurrentPlayer))
                    {
                        auto* palettes = request.appearance_type ? request.appearance_type->Palette : nullptr;
                        choice.converter = palettes ? palettes->Items[object.Owner->ColorSchemeIndex]->LightConvert
                            : request.appearance_type ? ColorScheme::Array.Items[object.Owner->ColorSchemeIndex]->LightConvert
                            : object.GetRemapColour();
                    }
                else if (owner)
                    if (auto* scheme = ColorScheme::Array.GetItemOrDefault(owner->ColorSchemeIndex))
                        choice.converter = scheme->LightConvert;
                break;
            }
            case WorldPaletteSource::techno_unit:
                if (!request.object) return DrawingStatus::invalid_argument;
                choice.palette = world_palette(world, w.presentation.unit_palette, request.house ? request.house : request.object->Owner);
                break;
        }
        return choice.converter || choice.palette ? DrawingStatus::drawn : DrawingStatus::unavailable;
    } catch (...) {
        choice = {};
        return DrawingStatus::backend_failure;
    }
}
SHPStruct* world_sprite_data(SHPStruct* image) {
    // GetData alone uses one shared scratch buffer (0x0069E580). Resolving the
    // next reference evicts it. Drawing needs the reference-owned allocation,
    // as in the original GetPixels path (0x0069E740), including failed-load null.
    if (!image) return nullptr;
    if (auto* reference = image->AsReference()) {
        reference->Load();
        return reference->Data;
    }
    return image;
}
namespace {
std::array<std::uint32_t, 15> voxel_state(const BuildingVoxelPart& part) {
    std::array<std::uint32_t, 15> state{};
    const auto& local = part.local;
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned col = 0; col < 4; ++col)
            state[row * 4 + col] = std::bit_cast<std::uint32_t>(local.row[row][col]);
    state[12] = part.frame;
    state[13] = unsigned(part.use_buffer) | (unsigned(part.shadow) << 1) | (unsigned(part.half_shadow) << 2) |
                (unsigned(part.camera_applied) << 3) | (unsigned(part.apply_hva) << 4) |
                (unsigned(part.shadow_layer) << 8);
    state[14] = unsigned(part.voxel_index);
    return state;
}
}
namespace {
void prepare_voxel_reference(const BuildingVoxelPart& part, const VoxelPalette& palette, VoxelSurface& surface) {
    static const bool verify = std::getenv("RA2_VERIFY_VOXELS") != nullptr;
    if (!verify || !surface.gpu) return;
    VoxelSurface reference;
    if (render_building_voxel(part, palette, reference) != DrawingStatus::drawn || reference.width != surface.width ||
        reference.height != surface.height || reference.offset.X != surface.offset.X ||
        reference.offset.Y != surface.offset.Y)
        throw std::runtime_error("CPU/GPU voxel setup differential failed");
    surface.gpu->reference_pixels = std::move(reference.pixels);
}
}
DrawingStatus cache_world_voxel(MapWorld& world, const TechnoVoxelRequest& r, std::shared_ptr<VoxelSurface>& voxel,
                                std::shared_ptr<const VoxelImageHandle> mask) noexcept {
    BuildingVoxelPart part;
    part.resource = r.resource;
    part.local = r.local_matrix ? *r.local_matrix : r.matrix;
    part.frame = unsigned(r.frame);
    part.cache_key = r.cache_key;
    part.shadow = r.shadow;
    part.shadow_layer = r.shadow_layer;
    part.half_shadow = r.half_shadow;
    part.use_buffer = r.use_buffer;
    part.camera_applied = !r.local_matrix;
    part.shadow_exclusion = r.shadow_exclusion;
    part.shadow_offset = r.shadow_offset;
    part.shadow_cache = r.shadow_cache;
    part.shadow_force = r.shadow_force;
    return cache_world_voxel(world, part, voxel, std::move(mask));
}
DrawingStatus cache_world_voxel(MapWorld& world, const BuildingVoxelPart& part, std::shared_ptr<VoxelSurface>& voxel,
                                std::shared_ptr<const VoxelImageHandle> mask) noexcept {
    try {
        auto& w = *world.impl;
        if (part.shadow) {
            voxel.reset();
            // 0x706BD0 caches the masked cold result in the caller's shadow index.
            // A hit must not resample a surface that UnitClass has already cleared.
            // Masked shadows cannot share the geometry-only cache with other units.
            const bool cacheable = part.cache_key != -1 && part.shadow_cache;
            const auto key = std::make_pair(part.shadow_cache, part.cache_key);
            if (cacheable) {
                auto found = w.presentation.shadow_voxel_surfaces.find(key);
                if (found != w.presentation.shadow_voxel_surfaces.end()) {
                    voxel = found->second;
                    return DrawingStatus::drawn;
                }
                if (!part.shadow_force) return DrawingStatus::skipped;
            }
            auto surface = std::make_shared<VoxelSurface>();
            const auto status = w.presentation.gpu_voxels
                                    ? prepare_building_voxel(part, w.presentation.voxel_palette, w.presentation.gpu_voxel_assets, *surface)
                                    : render_building_voxel(part, w.presentation.voxel_palette, *surface);
            if (status != DrawingStatus::drawn) {
                if (status == DrawingStatus::unavailable)
                    ++w.presentation.missing_voxel_parts;
                else if (status != DrawingStatus::skipped)
                    ++w.presentation.invalid_voxel_parts;
                return status == DrawingStatus::backend_failure ? status : DrawingStatus::skipped;
            }
            prepare_voxel_reference(part, w.presentation.voxel_palette, *surface);
            if (surface->gpu && mask)
                surface->gpu->mask = w.presentation.voxel_composites.get(*mask, {0, 0, mask->width, mask->height});
            voxel = std::move(surface);
            if (cacheable) w.presentation.shadow_voxel_surfaces.emplace(key, voxel);
            return DrawingStatus::drawn;
        }
        const auto original_key = std::make_pair(part.resource, part.cache_key);
        if (part.cache_key != -1 && !part.shadow) {
            auto found = w.presentation.unit_voxel_surfaces.find(original_key);
            if (found != w.presentation.unit_voxel_surfaces.end()) voxel = found->second;
        }
        if (!voxel && w.presentation.gpu_voxels) {
            auto surface = std::make_shared<VoxelSurface>();
            const auto status = prepare_building_voxel(part, w.presentation.voxel_palette, w.presentation.gpu_voxel_assets, *surface);
            if (status != DrawingStatus::drawn) {
                if (status == DrawingStatus::unavailable)
                    ++w.presentation.missing_voxel_parts;
                else if (status != DrawingStatus::skipped)
                    ++w.presentation.invalid_voxel_parts;
                return status == DrawingStatus::backend_failure ? status : DrawingStatus::skipped;
            }
            prepare_voxel_reference(part, w.presentation.voxel_palette, *surface);
            voxel = std::move(surface);
            if (part.cache_key != -1) w.presentation.unit_voxel_surfaces.emplace(original_key, voxel);
        }
        if (!voxel) {
            const auto key = std::make_pair(part.resource, voxel_state(part));
            auto found = w.presentation.voxel_surfaces.find(key);
            if (found == w.presentation.voxel_surfaces.end()) {
                auto surface = std::make_shared<VoxelSurface>();
                const auto status = render_building_voxel(part, w.presentation.voxel_palette, *surface);
                if (status != DrawingStatus::drawn) {
                    if (status == DrawingStatus::unavailable)
                        ++w.presentation.missing_voxel_parts;
                    else if (status != DrawingStatus::skipped)
                        ++w.presentation.invalid_voxel_parts;
                    return DrawingStatus::skipped;
                }
                if (w.presentation.voxel_surfaces.size() >= 512) w.presentation.voxel_surfaces.clear();
                found = w.presentation.voxel_surfaces.emplace(key, std::move(surface)).first;
            }
            voxel = found->second;
            if (part.cache_key != -1 && !part.shadow) w.presentation.unit_voxel_surfaces.emplace(original_key, voxel);
        }
        return DrawingStatus::drawn;
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
const BytePalette* world_palette(MapWorld& world, const BytePalette& source, HouseClass* owner) {
    if (!owner) return &source;
    auto& cache = world.impl->presentation.remaps;
    const auto hsv = world_house_hsv(*owner);
    const auto color = (std::uint32_t(hsv.R) << 16) | (std::uint32_t(hsv.G) << 8) | hsv.B;
    const auto key = std::make_pair(&source, color);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second.get();
    auto palette = std::make_unique<BytePalette>();
    ColorScheme::BuildPalette(hsv, source, *palette);
    auto* out = palette.get();
    cache.emplace(key, std::move(palette));
    return out;
}
namespace {
DrawingStatus resolve_sprite_palette(const MapDrawingContext& c, const BytePalette* palette,
                                     const ConvertClass* converter, CellClass* cell, bool cell_tint, bool color_scheme,
                                     const DrawingPaletteHandle*& pal) noexcept {
    if (converter) {
        if (!c.converter_palette) return DrawingStatus::unavailable;
        return c.converter_palette(c.types.backend_context, *converter, converter_revision(*converter), pal);
    }
    if (!palette) return DrawingStatus::unavailable;
    if (color_scheme) {
        if (!c.color_scheme_palette) return DrawingStatus::unavailable;
        return c.color_scheme_palette(c.types.backend_context, *palette, 53, pal);
    }
    if (cell_tint) {
        if (!c.terrain_palette) return DrawingStatus::unavailable;
        int r = 1000, g = 1000, b = 1000, shades = 53;
        if (cell && !game::cell_converter_tint(*cell, r, g, b, shades)) {
            r = cell->Color2_Red;
            g = cell->Color2_Green;
            b = cell->Color2_Blue;
            shades = LightConvertClass::PrepareCellTint(r, g, b, c.lighting_quality);
        }
        return c.terrain_palette(c.types.backend_context, *palette, r, g, b, shades, pal);
    }
    if (!c.shape_palette) return DrawingStatus::unavailable;
    return c.shape_palette(c.types.backend_context, *palette, 53, pal);
}
}
namespace {
struct RequestSourceScope {
    PresentationFrame& frame;
    const WorldDrawingSource* previous;
    RequestSourceScope(PresentationFrame& f, const WorldDrawingSource& source) : frame(f), previous(f.source) { frame.source = &source; }
    ~RequestSourceScope() { frame.source = previous; }
};
DrawingStatus submit_pending_world(PresentationFrame& frame, const PendingWorldDrawing& packet) {
    RequestSourceScope source(frame, packet.source);
    ++frame.statistics->visited;
    const auto status = packet.is_indexed ? submit_type_indexed(frame.drawing->types, packet.indexed)
                                          : submit_type_shape(frame.drawing->types, packet.shape);
    if (status == DrawingStatus::drawn) ++frame.statistics->drawn;
    else if (status == DrawingStatus::skipped) ++frame.statistics->skipped;
    else return record_drawing_failure(status, packet.is_indexed ? "Voxel submit" : "World SHP submit", packet.source.owner,
                                       packet.is_indexed ? nullptr : &packet.shape);
    return status;
}
}
DrawingStatus flush_world_background(PresentationFrame& frame_ref) noexcept {
    auto* frame = &frame_ref;
    if (!frame->drawing || !frame->statistics) return DrawingStatus::unavailable;
    try {
        frame->background_requests = unsigned(frame->pending_background.size());
        auto result = DrawingStatus::skipped;
        for (const auto& packet : frame->pending_background) {
            const auto status = submit_pending_world(*frame, packet);
            if (!drawing_completed(status)) return status;
            if (status == DrawingStatus::drawn) result = status;
        }
        frame->pending_background.clear();
        return result;
    } catch (...) { return DrawingStatus::backend_failure; }
}
DrawingStatus submit_world_shape(PresentationFrame& frame_ref, const ShapeDrawingRequest& request, ObjectClass* owner,
                                 CellClass* cell, const ConvertClass* converter, bool cell_tint,
                                 bool color_scheme) noexcept {
    try {
        auto* frame = &frame_ref;
        if (!frame->drawing || !frame->statistics) return DrawingStatus::unavailable;
        // Retain the original host's conservative centered-frame rejection while
        // consuming the request before its palette or source can change.
        auto* shape = world_sprite_data(request.image);
        if (!shape || request.frame < 0 || request.frame >= shape->Frames) return DrawingStatus::skipped;
        const auto b = shape->GetFrameBounds(request.frame);
        const auto& clip = TacticalClass::ViewBounds;
        const auto point = request.position;
        if (point.X - shape->Width / 2 + b.X + b.Width < 0 || point.Y - shape->Height / 2 + b.Y + b.Height < 0 ||
            point.X - shape->Width / 2 + b.X >= clip.Width || point.Y - shape->Height / 2 + b.Y >= clip.Height) {
            if (frame->recording_background) cull_background_item(*frame);
            return DrawingStatus::skipped;
        }
        const auto& c = *frame->drawing;
        auto r = request;
        r.target = c.types.target;
        r.clip = frame->bounds;
        const auto resolved =
            resolve_sprite_palette(c, converter ? nullptr : reinterpret_cast<const BytePalette*>(request.palette),
                                   converter, cell, cell_tint, color_scheme, r.palette);
        if (!complete(resolved)) return resolved;
        r.depth_mode = ShapeDepthMode::legacy;
        r.blend_mode = ShapeBlendMode::palette;
        PendingWorldDrawing packet;
        packet.shape = r;
        packet.source = {owner, cell, converter ? nullptr : reinterpret_cast<const BytePalette*>(request.palette),
                         converter, cell_tint, color_scheme};
        if (!frame->submit_body && !frame->recording_background) {
            // Requests retained for the background flush carry the scanline depth of their position and cell
            // level (formerly the depth of the retained sprite), whatever the producer set. Both backends
            // ignore absolute_depth in legacy depth mode, so only the recorded request keeps this value.
            packet.shape.absolute_depth = std::clamp(
                0x8000 - (request.position.Y + (cell ? 15 * static_cast<signed char>(cell->Level) : 0) + 15), 0, 65535);
            frame->pending_background.push_back(std::move(packet));
            return DrawingStatus::drawn;
        }
        return submit_pending_world(*frame, packet);
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus submit_world_indexed(PresentationFrame& frame_ref, const IndexedDrawingRequest& request,
                                   ObjectClass* owner, CellClass* cell, const ConvertClass* converter,
                                   bool cell_tint, bool color_scheme, bool use_clip,
                                   std::shared_ptr<VoxelSurface> storage) noexcept {
    try {
        auto* frame = &frame_ref;
        if (!frame->drawing || !frame->statistics) return DrawingStatus::unavailable;
        const auto& c = *frame->drawing;
        const auto& bounds = frame->bounds;
        auto r = request;
        r.target = c.types.target;
        r.position.X += bounds.X;
        r.position.Y += bounds.Y;
        r.clip = bounds;
        if (use_clip) {
            const int left = std::max(bounds.X, request.clip.X + bounds.X),
                      top = std::max(bounds.Y, request.clip.Y + bounds.Y);
            const int right = std::min(bounds.X + bounds.Width, request.clip.X + request.clip.Width + bounds.X);
            const int bottom = std::min(bounds.Y + bounds.Height, request.clip.Y + request.clip.Height + bounds.Y);
            r.clip = {left, top, std::max(0, right - left), std::max(0, bottom - top)};
        }
        const auto resolved =
            resolve_sprite_palette(c, converter ? nullptr : reinterpret_cast<const BytePalette*>(request.palette),
                                   converter, cell, cell_tint, color_scheme, r.palette);
        if (!complete(resolved)) return resolved;
        PendingWorldDrawing packet;
        packet.indexed = r;
        packet.is_indexed = true;
        packet.storage = std::move(storage);
        packet.source = {owner, cell, converter ? nullptr : reinterpret_cast<const BytePalette*>(request.palette),
                         converter, cell_tint, color_scheme};
        if (!frame->submit_body && !frame->recording_background) {
            frame->pending_background.push_back(std::move(packet));
            return DrawingStatus::drawn;
        }
        return submit_pending_world(*frame, packet);
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus with_tactical_frame(PresentationFrame& frame, void (*call)(void*, PresentationContext&), void* argument,
                                  PresentationContext outer) noexcept {
    // The exact original camera is common to all objects in this invocation.
    // Keep it scoped so a nested pass or a changed floating-point environment
    // cannot change the camera of an already executing object.
    const auto camera = building_voxel_camera();
    TechnoDrawing environment;
    auto* previous_environment = frame.techno_environment;
    const bool previous_ready = frame.techno_environment_ready;
    frame.techno_environment = &environment;
    frame.techno_environment_ready = false;
    const auto* previous_camera = frame.voxel_camera;
    frame.voxel_camera = &camera;
    struct Restore {
        PresentationFrame& frame;
        const Matrix3D* camera;
        TechnoDrawing* environment;
        bool ready;
        ~Restore() {
            frame.voxel_camera = camera;
            frame.techno_environment = environment;
            frame.techno_environment_ready = ready;
        }
    } restore{frame, previous_camera, previous_environment, previous_ready};
    auto ctx = inherit_scopes(outer);
    ctx.frame = &frame;
    try {
        if (call)
            call(argument, ctx);
        else
            frame.status = DrawingStatus::invalid_argument;
    } catch (...) {
        frame.status = DrawingStatus::backend_failure;
    }
    return frame.status;
}
bool begin_lighting_batch(PresentationFrame& frame) noexcept {
    if (!frame.drawing || !supports_drawing(frame.drawing->types, DrawingKind::lighting_shape_batch) || frame.lighting_active) return false;
    auto& batch = frame.lighting;
    batch.entries.clear();
    batch.cell = {-1, -1};
    batch.status = DrawingStatus::skipped;
    batch.shroud = nullptr;
    batch.shroud_origin = {};
    batch.shroud_camera = {};
    frame.lighting_active = true;
    return true;
}
void lighting_batch_cell(PresentationFrame& frame, const CellStruct& cell) noexcept {
    if (frame.lighting_active) frame.lighting.cell = cell;
}
void lighting_batch_world_shroud(PresentationFrame& frame, SHPStruct* image, const Point2D& origin,
                                 const Point2D& camera) noexcept {
    if (frame.lighting_active) {
        frame.lighting.shroud = image;
        frame.lighting.shroud_origin = origin;
        frame.lighting.shroud_camera = camera;
    }
}
DrawingStatus end_lighting_batch(PresentationFrame& frame, const RectangleStruct& view) noexcept {
    const bool active = frame.lighting_active;
    frame.lighting_active = false;
    auto* batch = &frame.lighting;
    if (!active || !frame.drawing) return DrawingStatus::unavailable;
    if (!complete(batch->status)) return batch->status;
    const LightingShapeBatchRequest request{frame.drawing->types.target,
                                            view,
                                            batch->entries.data(),
                                            static_cast<std::uint32_t>(batch->entries.size()),
                                            batch->shroud,
                                            batch->shroud_origin,
                                            batch->shroud_camera};
    return submit_type_lighting_shape_batch(frame.drawing->types, request);
}
void cancel_lighting_batch(PresentationFrame& frame) noexcept { frame.lighting_active = false; }
namespace {
std::uint64_t mix(std::uint64_t h, std::uint64_t v) noexcept {
    std::uint64_t z = h ^ (v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2));
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
std::uint64_t value(const void* p) noexcept { return std::uint64_t(reinterpret_cast<std::uintptr_t>(p)); }
// Per-cell state records; background_audit compares them against retained scene content.
std::uint64_t cell_record(const CellClass& c) noexcept {
    std::uint64_t h =
        mix(mix(mix(std::uint64_t(std::uint32_t(c.IsoTileTypeIndex)), std::uint64_t(std::uint8_t(c.Height))),
                std::uint64_t(std::uint8_t(c.Level))),
            c.SlopeIndex);
    h = mix(mix(mix(mix(h, std::uint64_t(std::uint32_t(c.OverlayTypeIndex))), c.OverlayData),
                std::uint64_t(std::uint32_t(c.SmudgeTypeIndex))),
            c.SmudgeData);
    h = mix(mix(h, static_cast<std::uint32_t>(c.Flags)), static_cast<std::uint32_t>(c.AltFlags));
    return mix(mix(h, value(c.Rubble)), value(c.BridgeOwnerCell));
}
std::uint64_t light_record(const CellClass& c) noexcept {
    std::uint64_t h =
        mix(mix(mix(mix(0x4C, value(c.LightConvert)), c.Intensity_Normal), c.Intensity_Terrain), c.Color1_Blue);
    return mix(mix(mix(h, c.Color2_Red), c.Color2_Green), c.Color2_Blue);
}
}
void background_cell_records(const CellClass& cell, std::uint64_t& geometry, std::uint64_t& lighting) noexcept {
    geometry = cell_record(cell);
    lighting = light_record(cell);
}
void cull_background_item(PresentationFrame& frame) noexcept { frame.background_item_culled = true; }
bool take_background_item_culled(PresentationFrame& frame) noexcept {
    const bool culled = frame.background_item_culled;
    frame.background_item_culled = false;
    return culled;
}
DrawingStatus draw_lighting_shape(PresentationFrame& frame_ref, SHPStruct* source, int index, const Point2D& point,
                                  const RectangleStruct& clip, RasterBlendMode mode) noexcept {
    auto* frame = &frame_ref;
    if (!frame->drawing) return DrawingStatus::unavailable;
    const LightingShapeDrawingRequest request{frame->drawing->types.target, source, index, point, clip, mode};
    auto* batch = frame->lighting_active ? &frame->lighting : nullptr;
    if (!batch) return submit_type_lighting_shape(frame->drawing->types, request);
    // Defer the request with the acceptance submit_type_lighting_shape applies
    // before its backend call; the batch keeps submission order.
    if (!request.target || request.frame < 0 ||
        (mode != RasterBlendMode::shroud && mode != RasterBlendMode::fog && mode != RasterBlendMode::alpha_shape))
        return batch->status = DrawingStatus::invalid_argument;
    if (!request.image) return batch->status = DrawingStatus::unavailable;
    if (request.clip.Width <= 0 || request.clip.Height <= 0) return DrawingStatus::skipped;
    try {
        batch->entries.push_back({request, batch->cell});
    } catch (...) {
        return batch->status = DrawingStatus::backend_failure;
    }
    return DrawingStatus::drawn;
}
void record_tactical_drawing(PresentationFrame& frame, DrawingStatus status) noexcept {
    if (complete(frame.status) && status != DrawingStatus::skipped) frame.status = status;
}
DrawingStatus draw_cell_terrain(CellClass& cell, const MapDrawingContext& drawing, const Point2D& at,
                                const RectangleStruct& bounds) noexcept {
    PresentationFrame frame;
    frame.drawing = &drawing;
    frame.bounds = bounds;
    struct Call {
        CellClass& cell;
        const Point2D& at;
        const RectangleStruct& bounds;
    } call{cell, at, bounds};
    return with_tactical_frame(
        frame,
        [](void* p, PresentationContext& ctx) {
            auto& c = *static_cast<Call*>(p);
            build_cell_tile(ctx, c.cell, c.at, c.bounds, false);
        },
        &call);
}
namespace {
void select_voxel_backend(MapWorld* world, const MapDrawingContext& drawing) {
    if (!world) return;
    auto& w = *world->impl;
    const bool gpu = supports_drawing(drawing.types, DrawingKind::voxel_indexed);
    if (w.presentation.gpu_voxels == gpu) return;
    w.presentation.voxel_surfaces.clear();
    w.presentation.unit_voxel_surfaces.clear();
    w.presentation.shadow_voxel_surfaces.clear();
    w.presentation.gpu_voxel_assets.clear();
    w.presentation.voxel_composites.clear();
    w.presentation.gpu_voxels = gpu;
}
}
DrawingStatus draw_tactical_view(TacticalClass& tactical, MapWorld* world, const MapDrawingContext& drawing,
                                 const RectangleStruct& bounds, MapDrawStatistics& stats) noexcept {
    select_voxel_backend(world, drawing);
    PresentationFrame frame{world, &drawing, bounds, &stats};
    frame.state = world ? &world->impl->presentation : nullptr;
    struct Call {
        TacticalClass& tactical;
    } call{tactical};
    return with_tactical_frame(
        frame,
        [](void* p, PresentationContext& ctx) { render_tactical(static_cast<Call*>(p)->tactical, *ctx.frame); }, &call);
}
DrawingStatus draw_map_world(MapWorld& world, const MapDrawingContext& drawing, const RectangleStruct& bounds,
                             MapDrawStatistics& stats) noexcept {
    select_voxel_backend(&world, drawing);
    PresentationFrame frame{&world, &drawing, bounds, &stats};
    frame.state = &world.impl->presentation;
    frame.include_terrain = false;
    struct Call {
        TacticalClass& tactical;
    } call{*TacticalClass::Instance};
    return with_tactical_frame(
        frame,
        [](void* p, PresentationContext& ctx) { render_tactical(static_cast<Call*>(p)->tactical, *ctx.frame); }, &call);
}
}
