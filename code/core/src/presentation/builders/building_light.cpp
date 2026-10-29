// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors; adaptations Copyright 2026 OpenRA2.
// OpenTS blight.cpp, 44fac744f70235e0d5ddca107364a68f95132ce9.
// YR 0x00435820 / 0x004361D0 / 0x00435BE0; YR beam spread is 5.973333333333333.
#include "yrpp/BuildingLightClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/YRMath.h"
#include <bit>
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_world.hpp"
#include <cmath>
#include <algorithm>
#include "presentation/builders/building_light.hpp"
#include "presentation/builders/spotlight.hpp"
#include "surface_drawing.hpp"
namespace game {
namespace {
float chop(double value) {
    float result = float(value);
    if (std::abs(double(result)) > std::abs(value))
        result = std::bit_cast<float>(std::bit_cast<std::uint32_t>(result) - 1);
    return result;
}
CoordStruct rotate(CoordStruct origin, CoordStruct target, double angle) {
    const float a = chop(angle), c = chop(Math::cos(a)), s = chop(Math::sin(a));
    const float x = float(target.X - origin.X), y = float(target.Y - origin.Y);
    const float rx = chop(double(c) * x - double(s) * y), ry = chop(double(s) * x + double(c) * y);
    return {int(double(origin.X) + rx), int(double(origin.Y) + ry), target.Z};
}
int distance(CoordStruct a, CoordStruct b) {
    return int(Math::sqrt(double(a.X - b.X) * (a.X - b.X) + double(a.Y - b.Y) * (a.Y - b.Y) +
                          double(a.Z - b.Z) * (a.Z - b.Z)));
}
}
void build_building_light(PresentationContext& ctx, const BuildingLightClass& self, Point2D*, RectangleStruct* clip) {
    auto* context = ctx.type;
    auto* tactical = TacticalClass::Instance;
    auto* r = RulesClass::Instance;
    if (!context || !tactical || !r || !clip || self.BehaviourMode == SpotlightBehaviour::None || !self.OwnerObject ||
        self.OwnerObject->WhatAmI() != AbstractType::Building)
        return;
    auto* b = static_cast<BuildingClass*>(self.OwnerObject);
    if (!b->IsAlive || !b->IsPowerOnline() || b->IsFogged) return;
    if (ScenarioClass::Instance && ScenarioClass::Instance->SpecialFlags.FogOfWar) {
        auto* cell = MapClass::Instance.TryGetCellAt(self.Location);
        if (!cell || cell->IsFogged()) return;
    }
    const auto center = b->GetCenterCoords();
    const int dist = distance(self.Location, center);
    const int step = (r->SpotlightMovementRadius - r->SpotlightLocationRadius) / 10;
    const int stage = dist < r->SpotlightLocationRadius || !step ? 0 : (dist - r->SpotlightLocationRadius) / step;
    const int movement_radius = self.BehaviourMode == SpotlightBehaviour::Follow && dist > r->SpotlightLocationRadius
                              ? std::clamp(stage + 80, 0, 89) : 80;
    build_spotlight(ctx, self.Location, 16, movement_radius, SpotlightFlags::None);
    const int detection = int(stage * 5.973333333333333 + r->SpotlightRadius);
    if (dist <= 0 || detection < 0 || dist < detection) return;
    const double angle = Math::asin(double(detection) / dist);
    CoordStruct caster = center;
    caster.Z += 430;
    Point2D start{};
    TacticalClass::CoordsToClient(caster, tactical->TacticalPos, *clip, start);
    for (double a : {angle, -angle}) {
        auto endpoint = rotate(center, self.Location, a);
        Point2D end{};
        TacticalClass::CoordsToClient(endpoint, tactical->TacticalPos, *clip, end);
        game::submit_glow_line(ctx, *clip, start, end, -TacticalClass::AdjustForZ(center.Z + 400),
                               -TacticalClass::AdjustForZ(self.Location.Z + 250), 75 - 6 * stage);
    }
}

}
