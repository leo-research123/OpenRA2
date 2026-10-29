// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 object.cpp::Render; YR 0x005F4B10.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "presentation/object_draw.hpp"
#include "presentation/builders/building_light.hpp"
#include "yrpp/BuildingLightClass.h"
#include "presentation/builders/wave.hpp"
#include "yrpp/WaveClass.h"
#include "presentation/builders/voxel_anim.hpp"
#include "yrpp/VoxelAnimClass.h"
#include "presentation/builders/particle_system.hpp"
#include "yrpp/ParticleSystemClass.h"
#include "presentation/builders/particle.hpp"
#include "yrpp/ParticleClass.h"
#include "presentation/builders/bullet.hpp"
#include "yrpp/BulletClass.h"
#include "presentation/builders/infantry.hpp"
#include "presentation/builders/unit.hpp"
#include "presentation/builders/aircraft.hpp"
#include "presentation/builders/anim.hpp"
#include "presentation/builders/building.hpp"
#include "presentation/builders/terrain.hpp"
#include "yrpp/TerrainClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Drawing.h"
#include "map_runtime.hpp"
#include "debug/debug_state.hpp"
#include "building_render_rect.hpp"
#include "presentation/state.hpp"
#include "terrain_render_rect.hpp"
#include "tactical_drawing.hpp"
#include "background_invalidation.hpp"
#include <bit>
namespace game {
namespace {
// TerrainClass::DrawIfVisible skips an unmarked terrain object when the pass is not forced. The mark is the
// invalidation queue (MarkForRedraw puts the object there): the objects of the batch the frame consumed.
bool terrain_marked(const PresentationContext& ctx, const ObjectClass& object) {
    const auto* changes = ctx.frame ? ctx.frame->invalidations : nullptr;
    if (!changes) return false;
    const auto id = reinterpret_cast<std::uintptr_t>(&object);
    for (std::uint32_t i = 0; i < changes->object_count; ++i)
        if (changes->objects[i] == id) return true;
    return false;
}
bool moved(const ObjectClass& object) {
    return object.WhatAmI() == AbstractType::Infantry || object.WhatAmI() == AbstractType::Unit ||
           object.WhatAmI() == AbstractType::Aircraft || object.WhatAmI() == AbstractType::Anim ||
           object.WhatAmI() == AbstractType::Building || object.WhatAmI() == AbstractType::Bullet || object.WhatAmI() == AbstractType::Particle || object.WhatAmI() == AbstractType::ParticleSystem || object.WhatAmI() == AbstractType::VoxelAnim || object.WhatAmI() == AbstractType::Wave || object.WhatAmI() == AbstractType::BuildingLight || object.WhatAmI() == AbstractType::Terrain;
}
}
void draw_it(PresentationContext& ctx, const ObjectClass& object, Point2D* location, RectangleStruct* bounds,
             bool force_draw) {
    switch (object.WhatAmI()) {
        case AbstractType::Infantry:
            build_infantry(ctx, static_cast<const InfantryClass&>(object), location, bounds);
            return;
        case AbstractType::Unit:
            build_unit(ctx, static_cast<const UnitClass&>(object), location, bounds);
            return;
        case AbstractType::Aircraft:
            build_aircraft(ctx, static_cast<const AircraftClass&>(object), location, bounds);
            return;
        case AbstractType::Anim:
            build_anim(ctx, static_cast<const AnimClass&>(object), location, bounds, force_draw);
            return;
        case AbstractType::Building:
            build_building(ctx, static_cast<const BuildingClass&>(object), location, bounds);
            return;
        case AbstractType::Terrain:
            build_terrain(ctx, static_cast<const TerrainClass&>(object), location, bounds);
            return;
        case AbstractType::Bullet:
            build_bullet(ctx, static_cast<const BulletClass&>(object), location, bounds);
            return;
        case AbstractType::Particle:
            build_particle(ctx, static_cast<const ParticleClass&>(object), location, bounds);
            return;
        case AbstractType::ParticleSystem:
            build_particle_system(ctx, static_cast<const ParticleSystemClass&>(object), location, bounds);
            return;
        case AbstractType::VoxelAnim:
            build_voxel_anim(ctx, static_cast<const VoxelAnimClass&>(object), location, bounds);
            return;
        case AbstractType::Wave:
            build_wave(ctx, static_cast<const WaveClass&>(object), location, bounds);
            return;
        case AbstractType::BuildingLight:
            build_building_light(ctx, static_cast<const BuildingLightClass&>(object), location, bounds);
            return;
        default:
            object.DrawIt(location, bounds);
    }
}
void draw_again(PresentationContext& ctx, const ObjectClass& object, const Point2D& location,
                const RectangleStruct& bounds) {
    if (object.WhatAmI() == AbstractType::Building)
        build_building_again(ctx, static_cast<const BuildingClass&>(object), location, bounds);
    else if (object.WhatAmI() == AbstractType::Terrain)
        build_terrain_again(ctx, static_cast<const TerrainClass&>(object), location, bounds);
    else
        object.DrawAgain(location, bounds);
}
bool draw_object_if_visible(PresentationContext& ctx, const ObjectClass& object, RectangleStruct* bounds, bool forced,
                            DWORD pass) {
    if (!moved(object)) return object.DrawIfVisible(bounds, forced, pass);
    // UnitClass::DrawIfVisible (YR 0x0073B0B0) adds one test before the generic body: a vehicle tethered to a
    // building that opens or closes its unload door is not drawn.
    if (object.WhatAmI() == AbstractType::Unit) {
        const auto& unit = static_cast<const UnitClass&>(object);
        if (unit.IsTether) {
            const auto* radio = unit.GetNthLink();
            if (radio && radio->WhatAmI() == AbstractType::Building &&
                (radio->GetCurrentMission() == Mission::Unload || radio->QueuedMission == Mission::Unload)) {
                // The state queries are not const in the declarations; a copy of the timer answers them.
                auto door = static_cast<const BuildingClass*>(radio)->UnloadTimer;
                if (door.AreStates11() || door.AreStates10() || door.AreStates01() || door.AreStates00()) return false;
            }
        }
    }
    const auto& runtime = map_runtime();
    if (!bounds || !runtime.drawing_bounds || !TacticalClass::Instance) return false;
    const bool building = object.WhatAmI() == AbstractType::Building;
    const bool on_map_only = building || object.WhatAmI() == AbstractType::Terrain;
    // NeedsRedraw is not read: the drawing of a pass that is not forced would skip an object that is not marked, and
    // the only kind that has such a mark outside the object is the terrain (the invalidation queue, below).
    const bool terrain = object.WhatAmI() == AbstractType::Terrain;
    if (!(runtime.debug_map && *runtime.debug_map) && runtime.has_window && runtime.has_window() &&
        ((terrain && !forced && !terrain_marked(ctx, object)) || (on_map_only && !object.IsOnMap) || object.InLimbo))
        return false;
    const auto& tactical = *runtime.drawing_bounds;
    if (building) {
        // BuildingClass::DrawIfVisible (YR 0x0043CEA0).
        Drawing::Intersect(bounds, *bounds, tactical);
        // The memo-free value of GetRenderDimensions: the drawing does not write the building's cache.
        const auto storage = building_render_rect(static_cast<const BuildingClass&>(object), ctx.frame ? ctx.frame->state : nullptr);
        const auto* dimensions = &storage;
        const auto add = [](int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); };
        const int x = add(tactical.X, dimensions->X), y = add(tactical.Y, dimensions->Y);
        if (bounds->X >= add(x, dimensions->Width) || bounds->Y >= add(y, dimensions->Height) ||
            add(bounds->X, bounds->Width) <= x || add(bounds->Y, bounds->Height) <= y)
            return false;
        CoordStruct coordinates;
        Point2D point{};
        TacticalClass::Instance->CoordsToClient(object.GetRenderCoords(&coordinates), &point);
        if (bounds->X > tactical.X)
            point.X = std::bit_cast<int>(unsigned(point.X) + unsigned(tactical.X) - unsigned(bounds->X));
        if (bounds->Y > tactical.Y)
            point.Y = std::bit_cast<int>(unsigned(point.Y) + unsigned(tactical.Y) - unsigned(bounds->Y));
        // 0x0043CFCD consumes only the low byte of the existing DWORD ABI argument.
        if (static_cast<BYTE>(pass)) {
            const auto& self = static_cast<const BuildingClass&>(object);
            if (!self.IsFogged || debug_fog_hidden()) build_building_upper(ctx, self, point, *bounds);
        } else
            draw_it(ctx, object, &point, bounds);
        return true;
    }
    if (object.WhatAmI() == AbstractType::Terrain) {
        // TerrainClass::DrawIfVisible (YR 0x0071CC50).
        const auto& viewport = tactical;
        Drawing::Intersect(bounds, *bounds, viewport);
        RectangleStruct rectangle = terrain_render_rect(static_cast<const TerrainClass&>(object));
        rectangle.X += viewport.X;
        rectangle.Y += viewport.Y;
        if (bounds->X >= rectangle.X + rectangle.Width || bounds->Y >= rectangle.Y + rectangle.Height ||
            bounds->X + bounds->Width <= rectangle.X || bounds->Y + bounds->Height <= rectangle.Y)
            return false;
        const auto coords = object.GetRenderCoords();
        Point2D point;
        TacticalClass::Instance->CoordsToClient(&coords, &point);
        if (bounds->X > viewport.X) point.X += viewport.X - bounds->X;
        if (bounds->Y > viewport.Y) point.Y += viewport.Y - bounds->Y;
        draw_it(ctx, object, &point, bounds);
        return true;
    }
    CoordStruct coordinates;
    Point2D point{};
    if (!TacticalClass::Instance->CoordsToClient(object.GetRenderCoords(&coordinates), &point) &&
        object.WhatAmI() != AbstractType::ParticleSystem)
        return false;
    Drawing::Intersect(bounds, *bounds, tactical);
    if (bounds->X > tactical.X)
        point.X = std::bit_cast<int>(unsigned(point.X) + unsigned(tactical.X) - unsigned(bounds->X));
    if (bounds->Y > tactical.Y)
        point.Y = std::bit_cast<int>(unsigned(point.Y) + unsigned(tactical.Y) - unsigned(bounds->Y));
    // Even an empty intersection reaches DrawIt and returns true in the EXE.
    draw_it(ctx, object, &point, bounds);
    return true;
}
}
