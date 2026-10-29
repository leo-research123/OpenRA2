// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// SPDX-License-Identifier: GPL-3.0-or-later
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Background traversal derives from OpenTS 44fac744 tactical.cpp, calibrated to YR.
#include "presentation/tactical_presenter.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/LaserDrawClass.h"
#include "yrpp/IonBlastClass.h"
#include "yrpp/RadBeam.h"
#include "yrpp/EBolt.h"
#include "yrpp/LineTrail.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/StringTable.h"
#include "tactical_drawing.hpp"
#include "presentation/builders/ebolt.hpp"
#include "presentation/builders/shroud.hpp"
#include "presentation/builders/ion_blast.hpp"
#include "presentation/builders/line_trail.hpp"
#include "presentation/builders/laser.hpp"
#include "presentation/builders/spotlight.hpp"
#include "presentation/builders/radbeam.hpp"
#include "presentation/visibility.hpp"
#include "presentation/builders/terrain.hpp"
#include "presentation/builders/cell.hpp"
#include "presentation/builders/extras.hpp"
#include "presentation/builders/tactical_extras.hpp"
#include "presentation/builders/timers.hpp"
#include "render_phase.hpp"
#include "presentation/builders/planning.hpp"
#include "presentation/builders/beacon.hpp"
#include "presentation/builders/radial.hpp"
#include "presentation/builders/placement.hpp"
#include "yrpp/FootClass.h"
#include "presentation/builders/overlay.hpp"
#include "presentation/builders/building.hpp"
#include "map_world_internal.hpp"
#include "background_invalidation.hpp"
#include "overlay_drawing.hpp"
#include "type_drawing.hpp"
#include <algorithm>
#include <bit>

namespace game {
namespace {
bool complete(DrawingStatus status) { return drawing_completed(status); }
bool complete(PresentationContext& ctx) {
    record_tactical_drawing(ctx, type_drawing_status(ctx));
    const auto* frame = ctx.frame;
    return frame && complete(frame->status);
}
// These are implementation helpers for portions of Render, not extra YR entries.
void draw_background(PresentationContext& ctx, TacticalClass& tactical, const RectangleStruct& bounds) {
    build_overlays(ctx, tactical, bounds);
    if (!complete(ctx)) return;
    build_terrain_objects(ctx, true, bounds, bounds);
    if (!complete(ctx)) return;
    build_tile_shadows(ctx, tactical, bounds, bounds);
    if (!complete(ctx)) return;
    build_buildings(ctx, true, bounds, bounds);
}
// Render 0x6D46DD..0x6D47F6; planning and Temporal remain separate stages.
void draw_action_lines_and_links(PresentationContext& ctx) {
    const bool planning = PlanningNodeClass::PlanningModeActive;
    if (planning) {
        render_phase_planning_before();
        build_planning_underlay(ctx);
        build_planning_overlay(ctx);
    }
    const int count = TechnoClass::Array.Count;
    for (int i = 0; i < count; ++i) {
        auto* techno = TechnoClass::Array[i];
        if (techno->Owner->IsControlledByCurrentPlayer() && !planning && techno->IsSelected && TechnoClass::ActionLines)
            if (techno->WhatAmI() != AbstractType::Building)
                build_action_lines(ctx, static_cast<const FootClass&>(*techno), false, 0);
        // Do not deduplicate managers: the controller and controlled-object
        // entries independently visit the same link drawer in original order.
        if (!planning && techno->Health) {
            if (techno->CaptureManager && techno->CaptureManager->NeedsToDrawLinks())
                build_capture_links(ctx, *techno->CaptureManager);
            if (techno->Health && techno->MindControlledBy && techno->MindControlledBy->CaptureManager &&
                techno->MindControlledBy->CaptureManager->NeedsToDrawLinks())
                build_capture_links(ctx, *techno->MindControlledBy->CaptureManager);
        }
    }
    // 0x6D491A: interaction frames are drawn after the graph/object links.
    if (planning) build_planning_feedback(ctx);
}
// Render 0x6D4941..0x6D4B25: preserve list order and skip without consuming a row.

}

void present_tactical(TacticalClass& tactical_instance, PresentationFrame& bound, int mode) {
    auto* frame = &bound;
    // The drawing's own redraw requests (a factory door) become notifications for this frame.
    redraw_requests().apply();
    BackgroundInvalidationView changes;
    if (frame->include_terrain) {
        take_background_invalidations(changes);
        audit_background_invalidations(&changes);
    }
    frame->invalidations = &changes;
    // Native targets are redrawn in full. Modes 0x1 and 0x3 retain the original
    // background/combined phase split; DDraw scrolling and surface copies belong
    // to the target backend. Cached foreground-only mode is not implemented.
    if (mode != 0x1 && mode != 0x3) {
        frame->status = DrawingStatus::unsupported;
        return;
    }
    if (ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar) {
        frame->status = record_drawing_failure(DrawingStatus::unsupported, "deprecated FogOfWar mode");
        return;
    }
    struct Call {
        TacticalClass& tactical;
        bool foreground;
        const BackgroundInvalidationView& changes;
        PresentationFrame& frame;
    } call{tactical_instance, mode == 0x3, changes, bound};
    frame->status = with_tactical_render_resources(
        bound, call.foreground,
        [](void* p, PresentationContext& ctx) {
            auto& call = *static_cast<Call*>(p);
            auto& tactical = call.tactical;
            auto& frame = call.frame;
            const auto& bounds = frame.bounds;
            bool resident = false;
            if (frame.include_terrain) {
                if (!MapClass::Instance.Cells.Items) {
                    frame.status = DrawingStatus::unavailable;
                    return;
                }
                // A resident scene keeps the shroud of every cell; DrawShroud's per-cell
                // pass remains for hosts and frames without it.
                if (world_shroud_available(frame))
                    record_tactical_drawing(ctx, draw_world_shroud(ctx, tactical, call.changes, bounds));
                else {
                            build_shroud(ctx, tactical, bounds);
                }
                if (!complete(ctx)) return;
                if (frame.draw_world) {
                    const auto status = draw_background_scene(ctx, call.changes);
                    resident = status != DrawingStatus::unavailable;
                    if (resident) {
                        record_tactical_drawing(ctx, status);
                        if (!complete(ctx)) return;
                        if (status == DrawingStatus::drawn && frame.statistics) {
                            ++frame.statistics->visited;
                            ++frame.statistics->drawn;
                        }
                    }
                }
                if (!resident) {
                    build_tiles(ctx, tactical, bounds, bounds);
                    if (!complete(ctx)) return;
                }
            }
            if (frame.draw_world) {
                if (resident)
                    build_buildings(ctx, true, bounds, bounds);
                else
                    draw_background(ctx, tactical, bounds);
                if (!complete(ctx)) return;
                submit_tactical_background(ctx);
                if (!complete(ctx)) return;
            }
            // 0x6D4539..0x6D458E publishes the completed background before foreground.
            // Hosts that cache it learn the boundary and the view origin it used.
            if (frame.drawing) {
                BackgroundDrawingRequest published;
                published.target = frame.drawing->types.target;
                published.view_origin = tactical.TacticalPos;
                const auto status = submit_background_complete(frame.drawing->types, published);
                if (status != DrawingStatus::unavailable) {
                    record_tactical_drawing(ctx, status);
                    if (!complete(ctx)) return;
                }
            }
            // 0x6D45xx: the camera and view origin this pass drew. The original keeps them on the
            // Tactical for its next incremental redraw; here they are the presenter's own memory.
            if (auto* world = frame.world) {
                auto& memo = world->impl->presentation.tactical;
                memo.drawn_camera = tactical.TacticalPos;
                memo.view_origin = {tactical.TacticalCoord1.X, tactical.TacticalCoord1.Y};
                memo.presented = true;
            }
            if (!call.foreground) return;
            if (frame.draw_world) {
                {
                    build_rally_points(ctx, tactical, false);
                    if (!complete(ctx)) return;
                    build_placement(ctx, tactical, false);
                    if (!complete(ctx)) return;
                }
                present_objects(ctx, tactical, true);
                if (!complete(ctx)) return;
                {
                    { build_spotlights(ctx); }
                    if (!complete(ctx)) return;
                    { build_lasers(ctx); }
                    if (!complete(ctx)) return;
                    {
                                    build_ebolts(ctx);
                    }
                    if (!complete(ctx)) return;
                    { build_line_trails(ctx); }
                    if (!complete(ctx)) return;
                    build_beacons(ctx, BeaconManagerClass::Instance, nullptr, bounds);
                    if (!complete(ctx)) return;
                    { build_radbeams(ctx); }
                    if (!complete(ctx)) return;
                    { build_ion_blasts(ctx); }
                    if (!complete(ctx)) return;
                    build_rubber_band(ctx, tactical);
                    if (!complete(ctx)) return;
                    build_rally_points(ctx, tactical, true);
                    if (!complete(ctx)) return;
                    build_placement(ctx, tactical, true);
                    if (!complete(ctx)) return;
                    draw_action_lines_and_links(ctx);
                    if (!complete(ctx)) return;
                    render_phase_radials_before();
                    build_radial_indicators(ctx);
                    if (!complete(ctx)) return;
                }
            }
            {
                render_phase_timers_before();
                build_tactical_timers(ctx);
                if (!complete(ctx)) return;
            }
            // Remaining original waypoint, Temporal, terrain sparkle and screen
            // text stages are not implemented here. This is not full Render parity.
        },
        &call);
}
}
