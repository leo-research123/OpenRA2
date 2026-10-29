#include "tactical_spatial.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tactical.cpp: selectable registration/query, rubber band and Select_These.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR: fixed 500-entry table, inclusive band corners, current-camera subtraction.
// Standalone candidate storage uses the shared control capacity.
#include "yrpp/TacticalClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/DisplayClass.h"
#include "debug/debug_state.hpp"

// OpenTS Reset_Action_Line_Timer; YR 0x0070D150.
void TacticalClass::StartDrawActionLineTimer() { TechnoClass::ActionLineTimer.Start(25); }
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <bit>

namespace {
TacticalSelectableStruct selectables[TacticalClass::SELECTABLE_CAPACITY]{};
RectangleStruct band_rect(const LTRBStruct& band) {
    return {std::min(band.Left, band.Right), std::min(band.Top, band.Bottom), std::abs(band.Right - band.Left) + 1,
            std::abs(band.Bottom - band.Top) + 1};
}
bool contains(const RectangleStruct& rect, Point2D at) {
    return at.X >= rect.X && at.X < rect.X + rect.Width && at.Y >= rect.Y && at.Y < rect.Y + rect.Height;
}
}
TacticalSelectableStruct (&TacticalClass::SelectableObjects)[SELECTABLE_CAPACITY] = selectables;
#if !defined(RA2_YRPP_GAME)
TacticalSelectableStruct (&Unsorted::TacticalSelectables)[RA2_CONTROL_CAPACITY] = selectables;
#endif

// YR 0x006DA560: an expired object leaves the selectable table, and the copy of
// the placement building held by the display is released.
void TacticalClass::PointerExpired(AbstractClass* object, bool) {
    for (auto& entry : SelectableObjects)
        if (entry.Object == object) entry.Object = nullptr;
    game::invalidate_selectable_index();
    auto& display = DisplayClass::Instance;
    if (object == display.CurrentBuildingCopy) {
        display.SetActiveFoundationCopy(nullptr);
        display.CurrentBuildingCopy = nullptr;
        display.CurrentBuildingTypeCopy = nullptr;
        display.CurrentBuildingOwnerArrayIndexCopy = -1;
    }
}

bool TacticalClass::AddSelectable(ObjectClass* object, int x, int y) {
    if (SelectableCount >= SELECTABLE_CAPACITY || x < -32 || x > ViewBounds.Width + 32 || y < -32 ||
        y > ViewBounds.Height + 32)
        return false;
    SelectableObjects[SelectableCount++] = {object, x + TacticalPos.X, y + TacticalPos.Y};
    game::invalidate_selectable_index();
    return true;
}
void TacticalClass::AddBuildingsToSelectables(RectangleStruct bounds) {
    const int left = bounds.X + TacticalPos.X, top = bounds.Y + TacticalPos.Y;
    const int right = left + bounds.Width, bottom = top + bounds.Height;
    thread_local std::vector<ObjectClass*> candidates;
    game::collect_tactical_buildings(bounds, candidates);
    for (auto* object : candidates) {
        auto* building = static_cast<BuildingClass*>(object);
        if (!building->IsAlive || !building->IsOnMap || building->Type->InvisibleInGame) continue;
        // 0x006D9CE0 uses Location, not GetRenderCoords or a frame's crop.
        const auto at = CoordsToScreen(building->Location);
        if (at.X >= left && at.X <= right && at.Y >= top && at.Y <= bottom)
            AddSelectable(building, at.X - TacticalPos.X, at.Y - TacticalPos.Y);
    }
}
ObjectClass* TacticalClass::GetSelectableObject(const Point2D& point) {
    ObjectClass* best = nullptr;
    unsigned bestDistance = ~0u;
    const Point2D pixel{point.X + TacticalPos.X, point.Y + TacticalPos.Y};
    thread_local std::vector<std::uint32_t> candidates;
    // Beyond the supported native viewport, preserve the original wrapped
    // distance arithmetic by using its full table (including extreme tests).
    const bool indexed = point.X >= -8192 && point.X <= 16384 && point.Y >= -8192 && point.Y <= 16384 &&
                         ViewBounds.Width <= 8192 && ViewBounds.Height <= 8192 &&
                         game::query_selectable_indices({point.X - 14, point.Y - 19, 29, 39}, candidates);
    for (int n = 0; n < (indexed ? int(candidates.size()) : SelectableCount); ++n) {
        const int i = indexed ? int(candidates[n]) : n;
        const auto& candidate = SelectableObjects[i];
        auto* object = candidate.Object;
        if (!object) continue;
        switch (object->WhatAmI()) {
            case AbstractType::Unit:
            case AbstractType::Aircraft:
            case AbstractType::Building:
            case AbstractType::Infantry: {
                auto* techno = static_cast<TechnoClass*>(object);
                if (!object->IsAlive || object->InLimbo) continue;
                if (!techno->IsOwnedByCurrentPlayer && techno->CloakState == CloakState::Cloaked) {
                    auto* cell = MapClass::Instance.GetCellAt(techno->GetCoords());
                    if (!cell->Sensors_InclHouse(HouseClass::CurrentPlayer->ArrayIndex)) continue;
                }
                break;
            }
            default: {
                if (!object->GetType() || object->GetType()->WhatAmI() != AbstractType::TerrainType ||
                    !static_cast<TerrainTypeClass*>(object->GetType())->IsVeinhole)
                    continue;
                break;
            }
        }
        // Original 32-bit IMUL squares, then x87 truncation of dx²+dy²/2.
        const auto dx = unsigned(candidate.X) - unsigned(pixel.X), dy = unsigned(candidate.Y) - unsigned(pixel.Y);
        const double value = double(std::bit_cast<int>(dx * dx)) + double(std::bit_cast<int>(dy * dy)) * 0.5;
        const unsigned distance = static_cast<unsigned>(static_cast<std::int64_t>(value));
        if (distance < bestDistance && distance < 200) {
            best = object;
            bestDistance = distance;
        }
    }
    if (best) return best;
    CellStruct cell;
    const auto& bounds = DSurface::ViewBounds;
    if (!PickTerrainCell({point.X + bounds.X, point.Y + bounds.Y}, bounds, cell)) return nullptr;
    // 0x006DA4FB returns FirstObject, not a nearest-object or shadow query.
    return MapClass::Instance.GetCellAt(cell)->FirstObject;
}
void TacticalClass::StartRubberBand(const Point2D& point) {
    if (!Band.Left && !Band.Top) Band = {point.X, point.Y, point.X, point.Y};
}
void TacticalClass::ModifyRubberBand(const Point2D& point) {
    if (Band.Left || Band.Top) {
        Band.Right = point.X;
        Band.Bottom = point.Y;
    }
}
void TacticalClass::EndRubberBand() { Band = {}; }
bool TacticalClass::HasBandObjects() const {
    const auto rect = band_rect(Band);
    thread_local std::vector<std::uint32_t> candidates;
    const bool indexed = game::query_selectable_indices(rect, candidates);
    for (int n = 0; n < (indexed ? int(candidates.size()) : SelectableCount); ++n) {
        const int i = indexed ? int(candidates[n]) : n;
        const auto& candidate = SelectableObjects[i];
        if (candidate.Object && candidate.Object->IsAlive &&
            contains(rect, {candidate.X - TacticalPos.X, candidate.Y - TacticalPos.Y}))
            return true;
    }
    return false;
}
void TacticalClass::SelectRubberBand(SelectionCallback callback) {
    if (Band.Left || Band.Top) {
        SelectThese(band_rect(Band), callback);
        Band.Left = Band.Top = 0;
    }
}
void TacticalClass::SelectThese(const RectangleStruct& rect, SelectionCallback callback) {
    Unsorted::MoveFeedback = true;
    struct Query {
        std::vector<std::uint32_t> values;
        std::uint64_t revision = 0;
        std::size_t cursor = 0;
        int previous = -1;
        bool indexed = false;
        Query() { values.swap(pool()); }
        ~Query() { values.swap(pool()); }
        static std::vector<std::uint32_t>& pool() {
            thread_local std::vector<std::uint32_t> cache;
            return cache;
        }
        int next(const RectangleStruct& rect, int count) {
            const auto current = game::selectable_index_revision();
            if (current != revision) {
                indexed = game::query_selectable_indices(rect, values);
                revision = current;
                cursor = std::upper_bound(values.begin(), values.end(), previous,
                                          [](int a, std::uint32_t b) { return std::int64_t(a) < b; }) -
                         values.begin();
            }
            if (!indexed) return ++previous < count ? previous : -1;
            if (cursor == values.size()) return -1;
            return previous = int(values[cursor++]);
        }
    } candidates;
    if (rect.Width > 0 && rect.Height > 0)
        for (int i; (i = candidates.next(rect, SelectableCount)) >= 0;) {
            const auto candidate = SelectableObjects[i];
            auto* object = candidate.Object;
            if (!object || !object->IsAlive ||
                !contains(rect, {candidate.X - TacticalPos.X, candidate.Y - TacticalPos.Y}))
                continue;
            if (Game::IsTypeSelecting()) {
                Game::UICommands_TypeSelect_7327D0(object->GetType()->ID);
                continue;
            }
            if (callback) {
                callback(object);
                continue;
            }
            bool deployable = false;
            if (object->WhatAmI() == AbstractType::Building) {
                const auto* type = static_cast<BuildingClass*>(object)->Type;
                deployable = type && type->UndeploysInto && type->IsVehicle();
            }
            auto* owner = object->GetOwningHouse();
            if (game::debug_can_select_house(owner) && object->CanBeSelected() &&
                (object->WhatAmI() != AbstractType::Building || deployable) && object->Select())
                Unsorted::MoveFeedback = false;
        }
    Unsorted::MoveFeedback = true;
}
