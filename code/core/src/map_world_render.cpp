#include "tactical_spatial.hpp"
#include "debug/debug_state.hpp"
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later; EA terms: third_party/opents/LICENSE.md.
// Host resource bindings and request caches for the original tactical passes.
#include "map_world_internal.hpp"
#include "api/drawing_resources.hpp"
#include <cstdlib>
#include <cstring>
#include "convert_revision.hpp"
#include "techno_extras_drawing.hpp"
#include "building_drawing.hpp"
#include "overlay_drawing.hpp"
#include "yrpp/IsometricTileTypeClass.h"
#include "sprite_drawing.hpp"
#include "techno_drawing.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SuperClass.h"
#include "type_drawing.hpp"
#include "map_view.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/EBolt.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/ColorScheme.h"
#include "map_runtime.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include "tactical_drawing.hpp"
#include "presentation/tactical_presenter.hpp"
#include "presentation/object_draw.hpp"
#include "presentation/builders/extras.hpp"
#include "render_phase.hpp"
#include "verify/draw_verify.hpp"
namespace game {
namespace {
bool complete(DrawingStatus s) { return s == DrawingStatus::drawn || s == DrawingStatus::skipped; }
PresentationFrame& frame_of(const PresentationContext& ctx) {
    if (!ctx.frame) throw std::logic_error("missing tactical drawing frame");
    return *ctx.frame;
}
Point2D project(const CoordStruct& loc, const MapViewHandle& v) {
    auto p = TacticalClass::CoordsToScreen(loc);
    p.X -= TacticalClass::Instance->TacticalPos.X;
    p.Y -= TacticalClass::Instance->TacticalPos.Y;
    return p;
}
WorldPaletteRequest drawing_palette_request(DrawingPaletteKind kind, int index) noexcept {
    WorldPaletteRequest request;
    request.scheme_index = index;
    request.source = kind == DrawingPaletteKind::color_scheme ? WorldPaletteSource::color_scheme
                     : kind == DrawingPaletteKind::animation  ? WorldPaletteSource::animation
                                                              : WorldPaletteSource::selection;
    return request;
}
void present_sprite_object(PresentationContext& ambient, MapWorld& world, ObjectClass& object, AnimClass* anim,
                           ObjectClass* owner, CellClass* cell, Point2D point, bool visible = false,
                           bool forced = true, const RectangleStruct* clip = nullptr, bool force_draw = false) {
    struct Sink {
        PresentationFrame& frame;
        MapWorld& world;
        ObjectClass* owner;
        CellClass* cell;
        SpritePalette palette = SpritePalette::animation;
        const ConvertClass* converter = nullptr;
    };
    const auto layer = anim ? anim->InWhichLayer() : Layer::Ground;
    if (layer < Layer::Underground || layer > Layer::Top) return;
    Sink sink{frame_of(ambient), world, owner, cell};
    SpriteDrawing drawing;
    drawing.context = &sink;
    drawing.types.backend_context = &sink;
    drawing.types.target = reinterpret_cast<DrawingTargetHandle*>(&sink);
    drawing.detail_level = GameOptionsClass::Instance.DetailLevel;
    drawing.tactical_rect =
        clip ? *clip : RectangleStruct{0, 0, TacticalClass::ViewBounds.Width, TacticalClass::ViewBounds.Height};
    // This host uses the SHP renderer. The optional original Direct3D RING1
    // triangle path remains available to an environment with that backend.
    drawing.hardware = false;
    drawing.cell_at = [](void*, const CellStruct& at) noexcept { return MapClass::Instance.GetCellAt(at); };
    drawing.cell_at_world = [](void*, const CoordStruct& at) noexcept { return MapClass::Instance.GetCellAt(at); };
    drawing.initialize_light = [](void*, CellClass&) noexcept { return DrawingStatus::drawn; };
    drawing.shape_data = world_sprite_data;
    drawing.frame_bounds = [](void* context,SHPStruct* image, int frame) {
        auto* shape = world_sprite_data(image);
        return shape ? shape->GetFrameBounds(frame) : RectangleStruct{};
    };
    drawing.height = [](int z) noexcept { return TacticalClass::AdjustForZ(z); };
    drawing.building = [](void* context,CellClass& c) noexcept { return c.GetBuilding(); };
    drawing.shrouded = [](void* context,CellClass& c) noexcept { return !debug_fog_hidden() && c.IsShrouded(); };
    drawing.pixel_format = [](void* context) noexcept { return 2; };
    if (auto* rules = RulesClass::Instance) {
        if (unsigned(rules->LaserTargetColor) < 16)
            drawing.laser_color = reinterpret_cast<const unsigned char*>(&rules->ColorAdd[rules->LaserTargetColor]);
        if (unsigned(rules->ForceShieldColor) < 16)
            drawing.shield_color = reinterpret_cast<const unsigned char*>(&rules->ColorAdd[rules->ForceShieldColor]);
    }
    drawing.palette = [](void* p, SpritePalette kind, const AnimClass* anim, CellClass* cell,
                         const DrawingPaletteHandle*& result) noexcept {
        try {
            auto& sink = *static_cast<Sink*>(p);
            auto& w = *sink.world.impl;
            sink.palette = kind;
            sink.converter = nullptr;
            const BytePalette* palette = &w.presentation.anim_palette;
            switch (kind) {
                case SpritePalette::cell:
                    sink.cell = cell;
                    palette = &FileSystem::ISOx_PAL;
                    break;
                case SpritePalette::tiberium:
                    palette = &w.presentation.unit_palette;
                    break;
                case SpritePalette::neutral:
                    sink.converter = ColorScheme::Array.Items[0]->LightConvert;
                    break;
                case SpritePalette::player:
                    if (!HouseClass::CurrentPlayer) return DrawingStatus::unavailable;
                    sink.converter =
                        ColorScheme::Array.Items[HouseClass::CurrentPlayer->ColorSchemeIndex]->LightConvert;
                    break;
                case SpritePalette::alternative:
                    if (!anim || !anim->LightConvert) return DrawingStatus::unavailable;
                    sink.converter = anim->LightConvert;
                    break;
                case SpritePalette::animation:
                    break;
            }
            if ((kind == SpritePalette::neutral || kind == SpritePalette::player) && !sink.converter)
                return DrawingStatus::unavailable;
            result = sink.converter ? reinterpret_cast<const DrawingPaletteHandle*>(sink.converter)
                                    : reinterpret_cast<const DrawingPaletteHandle*>(palette);
            return DrawingStatus::drawn;
        } catch (...) {
            return DrawingStatus::backend_failure;
        }
    };
    drawing.types.backend.shape = [](void* p, const ShapeDrawingRequest& r) noexcept {
        try {
            auto& sink = *static_cast<Sink*>(p);
            return submit_world_shape(sink.frame, r, sink.owner, sink.cell, sink.converter,
                    sink.palette == SpritePalette::cell || sink.palette == SpritePalette::alternative,
                    sink.palette == SpritePalette::neutral || sink.palette == SpritePalette::player ||
                        (sink.palette == SpritePalette::alternative &&
                         reinterpret_cast<const BytePalette*>(r.palette) != &FileSystem::ISOx_PAL));
        } catch (...) {
            return DrawingStatus::backend_failure;
        }
    };
    struct Call {
        const ObjectClass& object;
        RectangleStruct clip;
        bool force;
    } call{object, drawing.tactical_rect, forced};
    const auto result = visible ? with_sprite_drawing(
                                      drawing,
                                      [](void* p, game::PresentationContext& ctx) {
                                          auto& c = *static_cast<Call*>(p);

                                          draw_object_if_visible(ctx, c.object, &c.clip, c.force, 0);
                                      },
                                      &call, inherit_scopes(ambient))
                                : draw_object_sprite(object, drawing, point, drawing.tactical_rect, force_draw,
                                                     inherit_scopes(ambient));
    if (!complete(result)) {
        record_drawing_failure(result, "Object sprite DrawIt", &object);
        throw std::runtime_error(drawing_failure());
    }
}
void present_attached_animation(PresentationContext& ambient, MapWorld& world, AnimClass& a, ObjectClass* owner,
                                CellClass* cell, Point2D point, bool force_draw) {
    present_sprite_object(ambient, world, a, &a, owner, cell, point, false, true, nullptr, force_draw);
}
void present_techno_object(PresentationContext& ambient, MapWorld& world, TechnoClass& object, int pass = -1,
                           bool forced = true, const RectangleStruct* clip = nullptr,
                           const Point2D* preview = nullptr) {
    struct Sink {
        PresentationFrame& frame;
        PresentationContext& ambient;
        MapWorld& world;
        TechnoDrawing* drawing;
        TechnoPalette palette = TechnoPalette::house;
        const ConvertClass* converter = nullptr;
        VoxelCompositeScratch* composite = nullptr;
        BSurface* unit_surface = nullptr;
        std::shared_ptr<VoxelImageHandle> unit_gpu;
        ~Sink() {
            if (composite) --world.impl->presentation.unit_composite_depth;
        }
    } sink{frame_of(ambient), ambient, world, nullptr};
    auto& w = *world.impl;
    auto* frame = ambient.frame;
    if (!frame || !frame->techno_environment) throw std::logic_error("missing tactical drawing environment");
    auto& environment = *frame->techno_environment;
    if (!frame->techno_environment_ready) {
        auto& drawing = environment;
        drawing.frame = Unsorted::CurrentFrame;
        const auto& runtime = map_runtime();
        drawing.window_active = runtime.has_window && runtime.has_window();
        drawing.debug_map = runtime.debug_map && *runtime.debug_map;
        drawing.fog_of_war =
            !debug_fog_hidden() && ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar;
        if (frame->voxel_camera)
            drawing.camera = *frame->voxel_camera;
        else
            drawing.camera = building_voxel_camera();
        drawing.building_depth = FileSystem::BUILDINGZ_SHA;
        drawing.level_height = Unsorted::LevelHeight;
        drawing.bridge_height = 4 * Unsorted::LevelHeight;
        if (auto* rules = RulesClass::Instance) {
            drawing.extra_infantry_light = rules->ExtraInfantryLight;
            drawing.extra_unit_light = rules->ExtraUnitLight;
            drawing.extra_aircraft_light = rules->ExtraAircraftLight;
            const auto tint = [&](int index) {
                return techno_color_mask(reinterpret_cast<const unsigned char*>(&rules->ColorAdd[index]), 2);
            };
            drawing.laser_tint = tint(rules->LaserTargetColor);
            drawing.shield_tint = tint(rules->ForceShieldColor);
            drawing.berserk_tint = tint(rules->BerserkColor);
            drawing.iron_tint = tint(rules->IronCurtainColor);
        }
        if (auto* scenario = ScenarioClass::Instance)
            drawing.level_light = (LightningStorm::Active ? scenario->IonLighting : scenario->NormalLighting).Level;
        drawing.project = [](void* p, const CoordStruct& at) noexcept {
            return project(at, static_cast<Sink*>(p)->world.impl->view);
        };
        drawing.shape_data = world_sprite_data;
        drawing.palette = [](void* p, const TechnoClass& object, TechnoPalette kind, CellClass*, HouseClass* house,
                             const DrawingPaletteHandle*& result) noexcept {
            auto& sink = *static_cast<Sink*>(p);
            sink.palette = kind;
            sink.converter = nullptr;
            WorldPaletteRequest request;
            request.object = &object;
            request.appearance_type = sink.drawing->appearance_type;
            request.house = house;
            switch (kind) {
                case TechnoPalette::cell:
                    request.source = WorldPaletteSource::cell;
                    break;
                case TechnoPalette::animation:
                    request.source = WorldPaletteSource::animation;
                    break;
                case TechnoPalette::normal:
                    request.source = WorldPaletteSource::selection;
                    break;
                case TechnoPalette::house:
                    request.source = WorldPaletteSource::techno_house;
                    break;
                case TechnoPalette::eight_bit:
                    request.source = WorldPaletteSource::techno_unit;
                    break;
            }
            WorldPaletteChoice choice;
            const auto status = choose_world_palette(sink.world, request, choice);
            if (status != DrawingStatus::drawn) return status;
            sink.converter = choice.converter;
            result = choice.converter ? reinterpret_cast<const DrawingPaletteHandle*>(choice.converter)
                                      : reinterpret_cast<const DrawingPaletteHandle*>(choice.palette);
            return DrawingStatus::drawn;
        };
        drawing.types.backend.shape = [](void* p, const ShapeDrawingRequest& r) noexcept {
            try {
                auto& sink = *static_cast<Sink*>(p);
                auto* object = const_cast<TechnoClass*>(sink.drawing->submitting_object);
                if (!object) return DrawingStatus::invalid_argument;
                auto* cell = MapClass::Instance.TryGetCellAt(object->Location);
                return submit_world_shape(sink.frame, r, object, cell, sink.converter,
                                          sink.palette == TechnoPalette::cell, sink.palette == TechnoPalette::house);
            } catch (...) {
                return DrawingStatus::backend_failure;
            }
        };
        drawing.begin_unit_voxel = [](void* p, Surface*& surface) noexcept {
            try {
                auto& sink = *static_cast<Sink*>(p);
                if (!sink.composite) {
                    auto& world = *sink.world.impl;
                    if (world.presentation.unit_composite_depth == world.presentation.unit_composite_pool.size())
                        world.presentation.unit_composite_pool.push_back(std::make_unique<VoxelCompositeScratch>());
                    sink.composite = world.presentation.unit_composite_pool[world.presentation.unit_composite_depth++].get();
                    auto& scratch = *sink.composite;
                    if (!scratch.surface) {
                        scratch.pixels.resize(256 * 256);
                        scratch.surface = BSurface::Create(256, 256, 1, scratch.pixels.data());
                    }
                    if (world.presentation.gpu_voxels) {
                        if (!scratch.image) {
                            scratch.image = std::make_shared<VoxelImageHandle>();
                            scratch.image->kind = VoxelImageKind::composite;
                            scratch.image->width = scratch.image->height = 256;
                        }
                        scratch.image->children.clear();
                        scratch.image->layers.clear();
                        sink.unit_gpu = scratch.image;
                        if (std::getenv("RA2_VERIFY_VOXELS"))
                            std::memset(scratch.pixels.data(), 0, scratch.pixels.size());
                    } else
                        std::memset(scratch.pixels.data(), 0, scratch.pixels.size());
                    sink.unit_surface = scratch.surface;
                }
                sink.drawing->unit_voxel_software = !sink.unit_gpu;
                surface = sink.unit_surface;
                sink.drawing->unit_voxel_surface = surface;
                return surface ? DrawingStatus::drawn : DrawingStatus::backend_failure;
            } catch (...) {
                return DrawingStatus::backend_failure;
            }
        };
        drawing.submit_unit_voxel = [](void* p, const UnitVoxelCompositeRequest& r) noexcept {
            try {
                auto& sink = *static_cast<Sink*>(p);
                auto rect = r.source_rect;
                if (rect.Width <= 0 || rect.Height <= 0) return DrawingStatus::skipped;
                if (r.source != sink.unit_surface || sink.composite->pixels.size() != 256 * 256)
                    return DrawingStatus::invalid_argument;
                // Keep the original composite bounds for the sinking waterline. Clip only
                // the final copy, moving source and destination together as in 0x7BBE20.
                RectangleStruct destination{std::bit_cast<int>(unsigned(r.position.X) - unsigned(r.clip.X)),
                                            std::bit_cast<int>(unsigned(r.position.Y) - unsigned(r.clip.Y)), rect.Width,
                                            rect.Height};
                if (!Drawing::BlitClip(destination, r.clip, rect, RectangleStruct{0, 0, 256, 256}))
                    return DrawingStatus::skipped;
                if (rect.X < 0 || rect.Y < 0 || rect.Width > 256 || rect.Height > 256 || rect.X > 256 - rect.Width ||
                    rect.Y > 256 - rect.Height)
                    return DrawingStatus::invalid_argument;
                std::shared_ptr<VoxelSurface> voxel;
                std::shared_ptr<VoxelImageHandle> composite;
                if (sink.unit_gpu)
                    composite = sink.world.impl->presentation.voxel_composites.get(*sink.unit_gpu, rect);
                else {
                    voxel = std::make_shared<VoxelSurface>();
                    voxel->width = rect.Width;
                    voxel->height = rect.Height;
                    voxel->pixels.resize(std::size_t(rect.Width) * rect.Height);
                    for (int y = 0; y < rect.Height; ++y)
                        for (int x = 0; x < rect.Width; ++x) {
                            const auto index = sink.composite->pixels[(rect.Y + y) * 256 + rect.X + x];
                            voxel->pixels[std::size_t(y) * rect.Width + x] = index ? 0x1000000u | unsigned(index) : 0;
                        }
                }
                if (!voxel) {
                    voxel = std::make_shared<VoxelSurface>();
                    voxel->width = rect.Width; voxel->height = rect.Height;
                    voxel->gpu = composite;
                }
                IndexedDrawingRequest request;
                request.palette = r.palette;
                request.position = {std::bit_cast<int>(unsigned(destination.X) + unsigned(r.clip.X)),
                                    std::bit_cast<int>(unsigned(destination.Y) + unsigned(r.clip.Y))};
                request.clip = r.clip;
                request.voxel = composite.get();
                request.width = rect.Width;
                request.height = rect.Height;
                if (voxel) {
                    request.pixels = voxel->pixels.data();
                    request.pixel_count = std::uint32_t(voxel->pixels.size());
                }
                request.absolute_depth = r.depth;
                request.depth_mode = ShapeDepthMode::legacy;
                request.intensity = r.intensity;
                request.flags = r.flags;
                request.tint = r.tint;
                request.gradient = r.gradient;
                return submit_world_indexed(sink.frame, request, const_cast<TechnoClass*>(r.object),
                                            MapClass::Instance.TryGetCellAt(r.object->Location), sink.converter,
                                            sink.palette == TechnoPalette::cell,
                                            sink.palette == TechnoPalette::house, true, voxel);
            } catch (...) {
                return DrawingStatus::backend_failure;
            }
        };
        drawing.voxel = [](void* p, const TechnoVoxelRequest& r) noexcept {
            try {
                auto& sink = *static_cast<Sink*>(p);
                auto& w = *sink.world.impl;
                std::shared_ptr<VoxelSurface> voxel;
                const auto cached =
                    cache_world_voxel(sink.world, r, voxel, r.shadow_exclusion ? sink.unit_gpu : nullptr);
                if (cached != DrawingStatus::drawn) return cached;
                if (r.raster_bounds) *r.raster_bounds = {voxel->offset.X, voxel->offset.Y, voxel->width, voxel->height};
                if (!r.shadow && sink.drawing->unit_voxel_surface == sink.drawing->voxel_surface && sink.unit_surface) {
                    if (sink.unit_gpu && voxel->gpu) {
                        if (!voxel->gpu->reference_pixels.empty()) {
                            VoxelSurface reference;
                            reference.width = voxel->width;
                            reference.height = voxel->height;
                            reference.offset = voxel->offset;
                            reference.pixels = voxel->gpu->reference_pixels;
                            const auto status = composite_unit_voxel(*sink.unit_surface, r.position, r.clip, reference);
                            if (status != DrawingStatus::drawn && status != DrawingStatus::skipped) return status;
                        }
                        const Point2D position{r.clip.X + r.position.X + voxel->offset.X,
                                               r.clip.Y + r.position.Y + voxel->offset.Y};
                        sink.unit_gpu->children.push_back(voxel->gpu);
                        sink.unit_gpu->layers.push_back({voxel->gpu.get(), position, r.clip});
                        return DrawingStatus::drawn;
                    }
                    const auto status = composite_unit_voxel(*sink.unit_surface, r.position, r.clip, *voxel);
                    return status;
                }
                IndexedDrawingRequest request;
                request.palette = r.palette;
                request.position = {r.position.X + voxel->offset.X, r.position.Y + voxel->offset.Y};
                request.voxel = voxel->gpu.get();
                request.width = voxel->width;
                request.height = voxel->height;
                request.pixels = voxel->pixels.data();
                request.pixel_count = std::uint32_t(voxel->pixels.size());
                request.absolute_depth = r.depth;
                request.depth_mode = ShapeDepthMode::legacy;
                request.intensity = r.intensity;
                request.shadow = r.shadow;
                request.flags = r.flags;
                request.tint = r.tint;
                request.gradient = 2;
                return submit_world_indexed(sink.frame, request, const_cast<TechnoClass*>(r.object),
                                            MapClass::Instance.TryGetCellAt(r.object->Location), sink.converter,
                                            sink.palette == TechnoPalette::cell,
                                            sink.palette == TechnoPalette::house, false, voxel);
            } catch (...) {
                return DrawingStatus::backend_failure;
            }
        };
        drawing.animation = [](void* p, const TechnoClass& object, AnimClass& anim, const Point2D&,
                               const RectangleStruct&, bool force_draw) noexcept {
            try {
                auto& sink = *static_cast<Sink*>(p);
                auto& w = *sink.world.impl;
                const auto at = anim.GetRenderCoords();
                auto* cell = MapClass::Instance.TryGetCellAt(object.Location);
                present_attached_animation(sink.ambient, sink.world, anim, const_cast<TechnoClass*>(&object), cell, project(at, w.view), force_draw);
                return DrawingStatus::drawn;
            } catch (...) {
                return DrawingStatus::backend_failure;
            }
        };
        if (frame) frame->techno_environment_ready = true;
    }
    // Only mutable per-object and nested-call state is copied into this scope.
    // Palette callbacks continue to resolve cell/converter state at request time.
    TechnoDrawing drawing = environment;
    sink.drawing = &drawing;
    drawing.context = &sink;
    drawing.types.backend_context = &sink;
    drawing.types.target = reinterpret_cast<DrawingTargetHandle*>(&sink);
    drawing.tactical_rect =
        clip ? *clip : RectangleStruct{0, 0, TacticalClass::ViewBounds.Width, TacticalClass::ViewBounds.Height};
    DrawingStatus status;
    if (pass < 0)
        status = draw_techno_object(object, drawing, project(object.GetRenderCoords(), w.view), drawing.tactical_rect,
                                    inherit_scopes(ambient));
    else {
        struct Call {
            TechnoClass& object;
            RectangleStruct clip;
            bool force;
            int pass;
            BuildingDrawing* building;
            Point2D point;
        } call{object, drawing.tactical_rect, forced, pass, nullptr, preview ? *preview : Point2D{}};
        const auto invoke = [](void* p, PresentationContext& ctx) {
            auto& c = *static_cast<Call*>(p);
            if (c.pass == 2)
                draw_again(ctx, c.object, c.point, c.clip);
            else
                draw_object_if_visible(ctx, c.object, &c.clip, c.force, DWORD(c.pass));
        };
        if (object.WhatAmI() == AbstractType::Building) {
            auto* cell = MapClass::Instance.TryGetCellAt(object.Location);
            BuildingDrawing parts{drawing.types, FileSystem::BUILDINGZ_SHA,
                                  cell ? std::bit_cast<short>(cell->Intensity_Normal) : 1000};
            call.building = &parts;
            status = with_techno_drawing(
                drawing,
                [](void* p, game::PresentationContext& ctx) {
                    auto& c = *static_cast<Call*>(p);
                    const auto result = with_building_drawing(
                        *c.building,
                        [](void* q, game::PresentationContext& ctx) {
                            auto& c = *static_cast<Call*>(q);

                            if (c.pass == 2)
                                draw_again(ctx, c.object, c.point, c.clip);
                            else
                                draw_object_if_visible(ctx, c.object, &c.clip, c.force, DWORD(c.pass));
                        },
                        p, ctx);
                    record_techno_drawing(*ctx.techno, result);
                },
                &call, inherit_scopes(ambient));
        } else
            status = with_techno_drawing(drawing, invoke, &call, inherit_scopes(ambient));
    }
    if (!complete(status)) {
        record_drawing_failure(status, "Techno DrawIt", &object);
        throw std::runtime_error(drawing_failure());
    }
}

}
DrawingStatus draw_tactical_building_preview(PresentationContext& outer, BuildingClass& building, Point2D point,
                                             const RectangleStruct& clip) noexcept {
    auto* frame = outer.frame;
    if (!frame) return DrawingStatus::unavailable;
    if (!frame->world) {
        auto ctx = inherit_scopes(outer);
        draw_again(ctx, building, point, clip);
        return DrawingStatus::drawn;
    }
    const bool previous = frame->submit_body;
    frame->submit_body = true;
    struct Restore {
        PresentationFrame& frame;
        bool value;
        ~Restore() { frame.submit_body = value; }
    } restore{*frame, previous};
    try {
        present_techno_object(outer, *frame->world, building, 2, true, &clip, &point);
        return DrawingStatus::drawn;
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus draw_tactical_object(PresentationContext& outer, ObjectClass& object, bool forced,
                                   const RectangleStruct& clip, bool upper) noexcept {
    if (!upper) draw_coverage_object(object.WhatAmI());
    auto* frame = outer.frame;
    if (!frame || !frame->world) return DrawingStatus::unavailable;
    try {
        auto& world = *frame->world;
        auto& w = *world.impl;
        if ((object.AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None)
            present_techno_object(outer, world, static_cast<TechnoClass&>(object), upper ? 1 : 0, forced, &clip);
        else if (object.WhatAmI() == AbstractType::Bullet || object.WhatAmI() == AbstractType::VoxelAnim) {
            struct Sink {
                PresentationFrame& frame;
                MapWorld& world;
                ObjectClass& object;
                DrawingPaletteKind palette = DrawingPaletteKind::normal;
                const ConvertClass* converter = nullptr;
                const BytePalette* raw_palette = nullptr;
            } sink{*frame, world, object};
            TypeDrawingContext parts;
            parts.target = reinterpret_cast<DrawingTargetHandle*>(&sink);
            parts.backend_context = &sink;
            DrawingResources resources;
            resources.context = &sink;
            resources.voxel_camera = frame->voxel_camera;
            resources.palette = [](void* p, DrawingPaletteKind kind, int index,
                                   const DrawingPaletteHandle*& result) noexcept {
                auto& sink = *static_cast<Sink*>(p);
                sink.palette = kind;
                sink.converter = nullptr;
                sink.raw_palette = nullptr;
                WorldPaletteChoice choice;
                const auto status = choose_world_palette(sink.world, drawing_palette_request(kind, index), choice);
                if (status != DrawingStatus::drawn) return status;
                sink.converter = choice.converter;
                sink.raw_palette = choice.palette;
                result = choice.converter ? reinterpret_cast<const DrawingPaletteHandle*>(choice.converter)
                                          : reinterpret_cast<const DrawingPaletteHandle*>(choice.palette);
                return DrawingStatus::drawn;
            };
            resources.voxel = [](void* p, const ObjectVoxelRequest& r) noexcept {
                try {
                    auto& sink = *static_cast<Sink*>(p);
                    auto& object = sink.object;
                    BuildingVoxelPart part;
                    part.resource = r.resource;
                    part.local = r.matrix;
                    part.camera_applied = true;
                    part.frame = r.frame;
                    part.shadow = r.shadow;
                    part.apply_hva = r.apply_hva;
                    part.voxel_index = r.voxel_index;
                    if (r.voxel_index >= 0) part.shadow_layer = r.voxel_index;
                    std::shared_ptr<VoxelSurface> voxel;
                    const auto status = cache_world_voxel(sink.world, part, voxel);
                    if (status != DrawingStatus::drawn) return status;
                    IndexedDrawingRequest request;
                    request.palette = reinterpret_cast<const DrawingPaletteHandle*>(sink.raw_palette);
                    request.position = {r.position.X + voxel->offset.X, r.position.Y + voxel->offset.Y};
                    request.voxel = voxel->gpu.get();
                    request.width = voxel->width;
                    request.height = voxel->height;
                    request.pixels = voxel->pixels.data();
                    request.pixel_count = std::uint32_t(voxel->pixels.size());
                    request.absolute_depth = r.depth_adjustment;
                    request.depth_mode = ShapeDepthMode::legacy;
                    request.intensity = r.intensity;
                    request.shadow = r.shadow;
                    request.flags = r.flags;
                    request.gradient = r.flat ? 0 : 2;
                    return submit_world_indexed(sink.frame, request, &object, object.GetCell(), sink.converter, false, false, false, voxel);
                } catch (...) {
                    return DrawingStatus::backend_failure;
                }
            };
            parts.backend.shape = [](void* p, const ShapeDrawingRequest& r) noexcept {
                try {
                    auto& sink = *static_cast<Sink*>(p);
                    auto& object = sink.object;
                    return submit_world_shape(sink.frame, r, &object, object.GetCell(), sink.converter, false,
                                              sink.palette == DrawingPaletteKind::color_scheme);
                } catch (...) {
                    return DrawingStatus::backend_failure;
                }
            };
            struct Call {
                ObjectClass& object;
                RectangleStruct clip;
                bool forced;
            } call{object, clip, forced};
            return with_drawing_resources(
                resources, parts,
                [](void* p, PresentationContext& ctx) {
                    auto& c = *static_cast<Call*>(p);
                    draw_object_if_visible(ctx, c.object, &c.clip, c.forced, 0);
                },
                &call, inherit_scopes(outer));
        } else if (object.WhatAmI() == AbstractType::Particle || object.WhatAmI() == AbstractType::ParticleSystem ||
                   object.WhatAmI() == AbstractType::Wave || object.WhatAmI() == AbstractType::BuildingLight) {
            auto drawing = frame->drawing->types;
            struct Call {
                ObjectClass& object;
                RectangleStruct clip;
                bool forced;
            } call{object, clip, forced};
            return with_type_drawing(
                drawing,
                [](void* p, PresentationContext& ctx) {
                    auto& c = *static_cast<Call*>(p);
                    draw_object_if_visible(ctx, c.object, &c.clip, c.forced, 0);
                },
                &call, inherit_scopes(outer));
        } else if (object.WhatAmI() == AbstractType::Anim) {
            auto& anim = static_cast<AnimClass&>(object);
            auto* owner = anim.OwnerObject;
            auto* cell = MapClass::Instance.TryGetCellAt(anim.GetRenderCoords());
            present_sprite_object(outer, world, anim, &anim, owner, cell, project(anim.GetRenderCoords(), w.view), true, forced, &clip);
        } else {
            auto* cell = MapClass::Instance.TryGetCellAt(object.Location);
            present_sprite_object(outer, world, object, nullptr, &object, cell, project(object.GetRenderCoords(), w.view), true, forced, &clip);
        }
        return DrawingStatus::drawn;
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus dispatch_tactical_object(PresentationContext& ctx, ObjectClass& object, TacticalObjectPass pass, Point2D* point,
                                       RectangleStruct* clip, bool forced) noexcept {
    auto* frame = ctx.frame;
    if (!frame || !clip) return DrawingStatus::unavailable;
    try {
        // Without a host world, keep the original virtual calls available to direct
        // bindings and instruction probes; no parallel object list is constructed.
        if (!frame->world) {
            if (pass == TacticalObjectPass::behind)
                object.DrawBehind(point, clip);
            else if (pass == TacticalObjectPass::extras)
                object.DrawExtras(point, clip);
            else if (pass == TacticalObjectPass::building_info)
                { build_building_info(ctx, static_cast<const BuildingClass&>(object), point, clip); }
            else {
                        draw_object_if_visible(ctx, object, clip, forced, pass == TacticalObjectPass::building_upper);
            }
            return DrawingStatus::drawn;
        }
        auto& world = *frame->world;
        auto& w = *world.impl;
        if (pass == TacticalObjectPass::body || pass == TacticalObjectPass::building_upper) {
            const bool previous = frame->submit_body;
            frame->submit_body = true;
            struct RestoreBody {
                PresentationFrame& frame;
                bool previous;
                ~RestoreBody() { frame.submit_body = previous; }
            } restore_body{*frame, previous};
            const auto result = draw_tactical_object(ctx, object, forced, *clip, pass == TacticalObjectPass::building_upper);
            if (!complete(result)) return record_drawing_failure(result, "DrawIfVisible", &object);
            return result;
        }
        auto* extras = ctx.extras;
        if (!extras) return DrawingStatus::unavailable;
        if (pass == TacticalObjectPass::building_info)
            build_building_info(ctx, static_cast<const BuildingClass&>(object), point, clip);
        else if ((object.AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None) {
            const auto& techno = static_cast<const TechnoClass&>(object);
            if (pass == TacticalObjectPass::behind) build_techno_behind(ctx, techno, point, clip);
            else build_techno_extras(ctx, techno, point, clip);
        }
        return complete(extras->status) ? extras->status
                                        : record_drawing_failure(extras->status, "Object drawing hook", &object);
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}

namespace {
DrawingStatus with_world_overlays(PresentationFrame& frame, void (*call)(void*, PresentationContext&), void* argument, PresentationContext ctx) {
    if (!frame.world) {
        call(argument, ctx);
        return frame.status;
    }
    auto& world = *frame.world;
    // CellClass owns all overlay decisions. This adapter only retains requests
    // and translates converter identities into the existing palette cache.
    struct OverlaySink {
        PresentationFrame& frame;
        MapWorld& world;
        CellClass* cell = nullptr;
        OverlayPalette palette = OverlayPalette::cell;
        const ConvertClass* converter = nullptr;
    } sink{frame, world};
    OverlayDrawing overlays;
    overlays.context = &sink;
    overlays.types.backend_context = &sink;
    overlays.types.target = reinterpret_cast<DrawingTargetHandle*>(&sink);
    overlays.frame = Unsorted::CurrentFrame;
    overlays.redraws = static_cast<unsigned char>(static_cast<unsigned char>(MapClass::Instance.Redraws) +
                                                  world.impl->presentation.tactical.redraw_epoch);
    overlays.tactical_rect = frame.bounds.Width > 0 ? frame.bounds : TacticalClass::ViewBounds;
    std::copy(std::begin(IsometricTileTypeClass::SlopeDepthShapes), std::end(IsometricTileTypeClass::SlopeDepthShapes),
              overlays.slope_depth + 1);
    overlays.initialize_light = [](void*, CellClass&) noexcept {
        // Native map loading already computes Cell's light/tint fields. GPU palette
        // conversion is deferred until draw_map_world; no fake LightConvert object.
        return DrawingStatus::drawn;
    };
    overlays.palette = [](void* p, CellClass& cell, OverlayPalette kind, const DrawingPaletteHandle*& result) noexcept {
        try {
            auto& sink = *static_cast<OverlaySink*>(p);
            sink.cell = &cell;
            sink.palette = kind;
            sink.converter = nullptr;
            const BytePalette* palette = &FileSystem::ISOx_PAL;
            if (kind == OverlayPalette::theater)
                palette = &FileSystem::TEMPERAT_PAL;
            else if (kind == OverlayPalette::wall) {
                if (!HouseClass::CurrentPlayer) return DrawingStatus::unavailable;
                // 0x0047F96A: walls draw with the current player's original scheme converter.
                if (ColorScheme::Array.Count) {
                    auto* scheme = ColorScheme::Array.GetItemOrDefault(HouseClass::CurrentPlayer->ColorSchemeIndex);
                    if (!scheme || !scheme->LightConvert) return DrawingStatus::unavailable;
                    sink.converter = scheme->LightConvert;
                    result = reinterpret_cast<const DrawingPaletteHandle*>(sink.converter);
                    return DrawingStatus::drawn;
                }
                // Resource-free native maps can draw before a color registry is loaded.
                palette = world_palette(sink.world, sink.world.impl->presentation.unit_palette, HouseClass::CurrentPlayer);
            }
            result = reinterpret_cast<const DrawingPaletteHandle*>(palette);
            return DrawingStatus::drawn;
        } catch (...) {
            return DrawingStatus::backend_failure;
        }
    };
    overlays.types.backend.shape = [](void* p, const ShapeDrawingRequest& r) noexcept {
        try {
            auto& sink = *static_cast<OverlaySink*>(p);
            auto* cell = sink.cell;
            return submit_world_shape(sink.frame, r, nullptr, cell, sink.converter, sink.palette == OverlayPalette::cell,
                                      sink.palette == OverlayPalette::wall && !sink.converter);
        } catch (...) {
            return DrawingStatus::backend_failure;
        }
    };

    return with_overlay_drawing(overlays, call, argument, ctx);
}
}

void submit_tactical_background(PresentationContext& ctx) noexcept {
    if (ctx.frame) record_tactical_drawing(*ctx.frame, flush_world_background(*ctx.frame));
}

DrawingStatus with_tactical_render_resources(PresentationFrame& frame_ref, bool foreground,
                                             void (*call)(void*, PresentationContext&), void* argument,
                                             PresentationContext outer) noexcept {
    auto ctx = inherit_scopes(outer);
    ctx.frame = &frame_ref;
    auto* frame = &frame_ref;
    if (!call) return DrawingStatus::unavailable;
    try {
        if (!frame->drawing || !frame->statistics || !frame->drawing->types.target)
            return DrawingStatus::unavailable;
        if (frame->bounds.Width <= 0 || frame->bounds.Height <= 0) return DrawingStatus::unavailable;
        if (frame->include_terrain && frame->statistics) *frame->statistics = {};
        frame->status = DrawingStatus::skipped;
        frame->background_requests = 0;
        frame->draw_world = frame->world && frame->world->impl->loaded;
        const bool hadBolts = foreground && EBolt::Array.Count != 0;
        if (frame->draw_world) {
            auto& w = *frame->world->impl;
            frame->pending_background.clear();
            w.presentation.missing_voxel_parts = w.presentation.invalid_voxel_parts = 0;
            if (w.presentation.voxel_asset_revision != voxel_resource_revision()) {
                w.presentation.voxel_surfaces.clear();
                w.presentation.unit_voxel_surfaces.clear();
                w.presentation.shadow_voxel_surfaces.clear();
                w.presentation.gpu_voxel_assets.clear();
                w.presentation.voxel_composites.clear();
                w.presentation.voxel_asset_revision = voxel_resource_revision();
            }
            // The host requests a fresh target even while paused. The original screen entry advances
            // only the low byte; DrawOverlays itself never owns this epoch.
            ++w.presentation.tactical.redraw_epoch;
        }
        DrawingStatus status;
        if (!foreground || !frame->draw_world) {
            status = with_world_overlays(*frame, call, argument, ctx);
        } else {
            auto& w = *frame->world->impl;
            const auto& c = *frame->drawing;
            if (!c.shape_palette)
                return record_drawing_failure(DrawingStatus::unavailable, "Tactical shape palette callback");
            TechnoExtrasDrawing extras{c.types, FileSystem::PIPS_SHP};
            extras.wrench = FileSystem::WRENCH_SHP;
            extras.selection_palette = &w.presentation.selection_palette;
            extras.camera = TacticalClass::Instance->TacticalPos;
            extras.pip_border = FileSystem::PIPBRD_SHP;
            extras.mobile_pips = FileSystem::PIPS2_SHP;
            // PALETTE_PAL is constructed with 53 shades at YR 0x0052BFAA.
            // Alpha-lit extras need its dark rows to retain per-pixel shroud coverage.
            status = c.shape_palette(c.types.backend_context, w.presentation.selection_palette, 53, extras.palette);
            if (!complete(status)) return status;
            status = c.shape_palette(c.types.backend_context, w.presentation.anim_palette, 53, extras.wrench_palette);
            if (!complete(status)) return status;
            DrawingResources resources;
            resources.normal_palette = &w.presentation.selection_palette;
            resources.clip = frame->bounds;
            resources.context = frame;
            resources.palette = [](void* p, DrawingPaletteKind kind, int index,
                                   const DrawingPaletteHandle*& result) noexcept {
                auto& frame = *static_cast<PresentationFrame*>(p);
                const auto& drawing = *frame.drawing;
                WorldPaletteChoice choice;
                const auto status = choose_world_palette(*frame.world, drawing_palette_request(kind, index), choice);
                if (status != DrawingStatus::drawn) return status;
                // This pass draws straight to the backend, which resolves the handles itself.
                if (choice.converter) {
                    if (!drawing.converter_palette) return DrawingStatus::unavailable;
                    return drawing.converter_palette(drawing.types.backend_context, *choice.converter,
                                                     converter_revision(*choice.converter), result);
                }
                if (!drawing.shape_palette) return DrawingStatus::unavailable;
                return drawing.shape_palette(drawing.types.backend_context, *choice.palette, 53, result);
            };
            struct Call {
                PresentationFrame& frame;
                TechnoExtrasDrawing& extras;
                DrawingResources& resources;
                void (*call)(void*, PresentationContext&);
                void* argument;
                PresentationContext* ctx;
            } context{*frame, extras, resources, call, argument, nullptr};
            status = with_techno_extras_drawing(
                extras,
                [](void* p, game::PresentationContext& ctx) {
                    auto& c = *static_cast<Call*>(p);
                    c.ctx = &ctx;
                    ctx.resources = &c.resources;
                    record_tactical_drawing(c.frame, with_drawing_resources(
                        c.resources, c.frame.drawing->types,
                        [](void* p, PresentationContext& scoped) {
                            auto& c = *static_cast<Call*>(p);
                            record_tactical_drawing(c.frame, with_world_overlays(c.frame, c.call, c.argument, scoped));
                        },
                        p, *c.ctx));
                },
                &context, ctx);
        }
        record_tactical_drawing(*frame, status);
        if (hadBolts) map_object_changed();
        if (!complete(frame->status)) return frame->status;
        if (frame->include_terrain && frame->statistics)
            return frame->statistics->drawn ? DrawingStatus::drawn : DrawingStatus::skipped;
        return frame->draw_world &&
                       (frame->statistics->drawn != 0)
                   ? DrawingStatus::drawn
                   : DrawingStatus::skipped;
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}

void render_tactical(TacticalClass& tactical, PresentationFrame& frame) {
    // One tactical render is one round of the render phase (see render_phase.hpp): the step, then the
    // presentation (which must leave game state alone), then the step that follows the draw.
    render_phase_tactical_before();
    {
        DrawVerifyScope verify;
        present_tactical(tactical, frame, 0x3);
    }
    render_phase_tactical_after();
}

// The table of selectable objects for this tick, camera and viewport: the buildings in view, then the mobile
// bodies in draw order (selectable_registration.cpp). Called every round of the render phase and, through the
// cache check below, when input needs the table before the next render.
void register_world_selectables(MapWorld& world) {
    auto& w = *world.impl;
    auto& tactical = *TacticalClass::Instance;
    const auto& runtime = map_runtime();
    const auto bounds = runtime.drawing_bounds ? *runtime.drawing_bounds : RectangleStruct{};
    const auto view_bounds = TacticalClass::ViewBounds;
    const unsigned environment = unsigned(runtime.has_window && runtime.has_window()) |
        (unsigned(runtime.debug_map && *runtime.debug_map) << 1) |
        (unsigned(debug_fog_hidden()) << 2) |
        (unsigned(ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar) << 3) |
        (unsigned(runtime.drawing_bounds != nullptr) << 4);
    const auto same = [](const RectangleStruct& a, const RectangleStruct& b) {
        return a.X == b.X && a.Y == b.Y && a.Width == b.Width && a.Height == b.Height;
    };
    flush_tactical_spatial();
    if (w.selectables_valid && w.selectable_count == tactical.SelectableCount &&
        w.selectable_table_revision == selectable_index_revision() && w.selectable_spatial_generation == tactical_spatial_generation() &&
        w.selectable_revision == world.presentation_revision &&
        w.selectable_camera == tactical.TacticalPos && same(w.selectable_bounds, bounds) &&
        same(w.selectable_view_bounds, view_bounds) && w.selectable_environment == environment &&
        w.selectable_player == HouseClass::CurrentPlayer && w.selectable_resources == shape_resource_generation()) {
        verify_tactical_selectables(tactical);
        w.selectable_table_revision = selectable_index_revision();
        return;
    }
    w.selectables_valid = false;
    rebuild_tactical_selectables(tactical);
    w.selectable_spatial_generation = tactical_spatial_generation();
    w.selectable_tick = world.simulation_tick;
    w.selectable_revision = world.presentation_revision;
    w.selectable_camera = tactical.TacticalPos;
    w.selectable_viewport = {view_bounds.Width, view_bounds.Height};
    w.selectable_bounds = bounds;
    w.selectable_view_bounds = view_bounds;
    w.selectable_environment = environment;
    w.selectable_player = HouseClass::CurrentPlayer;
    w.selectable_resources = shape_resource_generation();
    verify_tactical_selectables(tactical);
    w.selectable_table_revision = selectable_index_revision();
    w.selectable_count = tactical.SelectableCount;
    w.selectables_valid = true;
}
void rebuild_world_selectables(MapWorld& world) {
    register_world_selectables(world);
}
}
