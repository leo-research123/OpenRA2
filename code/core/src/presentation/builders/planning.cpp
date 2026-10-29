// Original PlanMgr graph and interaction drawing (0x63B0A0 / 0x63B150 / 0x63B2F0).
// The existing native raster target replaces the original Composite surface.
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "type_drawing.hpp"
#include "presentation/context.hpp"
#include "presentation/builders/planning.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
Point2D project(CoordStruct coords) noexcept {
    Point2D out;
    TacticalClass::Instance->CoordsToClient(&coords, &out);
    out.X += DSurface::ViewBounds.X;
    out.Y += DSurface::ViewBounds.Y;
    return out;
}
WORD pixel(ColorStruct c) noexcept { return WORD((c.R >> 3 << 11) | (c.G >> 2 << 5) | (c.B >> 3)); }
ColorStruct color(TechnoClass* unit, bool pending) noexcept {
    auto* house = HouseClass::Array.GetItemOrDefault(Game::PlanningManager_OwnerIndex(unit));
    auto c = house->Color;
    const int amount = pending ? 100 : 80;
    const int target = pending ? 255 : 0;
    c.R = byte(c.R + amount * (target - int(c.R)) / 256);
    c.G = byte(c.G + amount * (target - int(c.G)) / 256);
    c.B = byte(c.B + amount * (target - int(c.B)) / 256);
    return c;
}
bool committed(PlanningNodeClass* node, TechnoClass* owner) noexcept {
    auto* token = owner->PlanningToken;
    return token->field_8C == -1 || token->PlanningNodes.FindItemIndex(node) < token->field_8C;
}
WORD border(TechnoClass* owner) noexcept {
    const bool selected =
        Game::PlanningNodeAtAC4BF0 ? Game::PlanningNodeAtAC4BF0->FindMemberIndex(owner) != -1 : owner->IsSelected;
    return selected ? 0xFFFF : 0;
}
void fill(game::PresentationContext& ctx, RectangleStruct rect, WORD value) noexcept {
    const auto* context = ctx.type;
    if (!context) return;
    game::RasterDrawingRequest r;
    r.target = context->target;
    r.clip = DSurface::ViewBounds;
    r.position = {rect.X, rect.Y};
    r.width = rect.Width;
    r.height = rect.Height;
    r.color = value;
    game::record_type_drawing_result(ctx, game::submit_type_raster(*context, r));
}
void marker(game::PresentationContext& ctx, Point2D p, int size, WORD value, bool interior) noexcept {
    const int x = p.X - size / 2, y = p.Y - size / 2;
    if (interior) {
        fill(ctx, {x + 1, y + 1, size - 2, size - 2}, value);
        return;
    }
    fill(ctx, {x, y, size, 1}, value);
    fill(ctx, {x, y + size - 1, size, 1}, value);
    fill(ctx, {x, y, 1, size}, value);
    fill(ctx, {x + size - 1, y, 1, size}, value);
}
// 0x63D810: offset perpendicular to the major axis, clip, then draw the
// original four-on/four-off pattern with timeGetTime() >> 5 animation.
void line(game::PresentationContext& ctx, Point2D a, Point2D b, int offset, WORD value, bool dashed) noexcept {
    auto clip = DSurface::ViewBounds;
    if (std::abs(b.X - a.X) <= std::abs(b.Y - a.Y)) {
        a.X += offset;
        b.X += offset;
    } else {
        a.Y += offset;
        b.Y += offset;
    }
    if (!Line_In_Bounds(&a, &b, &clip)) return;
    int phase = (std::bit_cast<int>(0u - SystemTimer::GetMilliseconds()) >> 5) & 15;
    int increment = 1;
    if (a.X > b.X) {
        std::swap(a, b);
        phase = (phase + std::max(std::abs(a.X - b.X), std::abs(a.Y - b.Y) + 1)) & 15;
        increment = -1;
    }
    const int dx = b.X - a.X, dy = std::abs(b.Y - a.Y), sy = b.Y >= a.Y ? 1 : -1;
    const bool horizontal = dx > dy;
    int error = horizontal ? 2 * dy - dx : 2 * dx - dy;
    const int count = std::max(dx, dy) + int(dx == 0 || dy == 0);
    for (int i = 0; i < count; ++i) {
        if (!dashed || (phase & 7) < 4) fill(ctx, {a.X, a.Y, 1, 1}, value);
        phase = (phase + increment) & 15;
        if (horizontal) {
            if (error > 0) {
                a.Y += sy;
                error -= 2 * dx;
            }
            error += 2 * dy;
            ++a.X;
        } else {
            if (error > 0) {
                ++a.X;
                error -= 2 * dy;
            }
            error += 2 * dx;
            a.Y += sy;
        }
    }
}
CoordStruct event_coords(const EventClass& event, TechnoClass* owner) noexcept {
    CoordStruct result;
    Game::PlanningManager_EventCoords(&result, &event);
    if (result == CoordStruct{-1, -1, -1} && owner) return owner->GetCenterCoords();
    return result;
}
void node_links(game::PresentationContext& ctx, PlanningNodeClass* node, bool overlay) noexcept {
    CoordStruct at;
    node->GetCoords(&at);
    const auto end = project(at);
    auto* owner = node->GetOwner(0);
    for (int i = 0; i < node->PlanningBranches.Count; ++i) {
        auto* branch = node->PlanningBranches[i];
        TechnoClass* branch_owner = nullptr;
        bool pending = true;
        for (int m = 0; m < node->PlanningMembers.Count; ++m) {
            auto* member = node->PlanningMembers[m];
            if (member->field_8 != i) continue;
            branch_owner = member->Owner;
            if (committed(node, branch_owner)) {
                pending = false;
                break;
            }
        }
        const auto start = project(event_coords(branch->Packet, branch_owner));
        for (int offset = overlay ? 0 : -1; offset <= (overlay ? 1 : 2); ++offset)
            line(ctx, start, end, offset, overlay ? WORD(branch->field_74) : border(owner), overlay);
    }
    if (node->CommonBranch.MemberCount > 0 && node->field_B0 == -1) {
        TechnoClass* loop_owner = nullptr;
        bool pending = true;
        for (int i = 0; i < node->PlanningMembers.Count; ++i) {
            auto* member = node->PlanningMembers[i];
            if (!member->field_C) continue;
            loop_owner = member->Owner;
            if (!loop_owner->PlanningToken->field_98) {
                pending = false;
                break;
            }
        }
        auto& branch = node->CommonBranch;
        const auto start = project(event_coords(branch.Packet, loop_owner));
        for (int offset = overlay ? 0 : -1; offset <= (overlay ? 0 : 1); ++offset)
            line(ctx, start, end, offset, overlay ? WORD(branch.field_74) : border(owner), overlay);
    }
    if (!overlay) {
        marker(ctx, end, 8, border(owner), false);

    } else {
        marker(ctx, end, 8, WORD(node->field_B4), true);
    }
}
void unit_link(game::PresentationContext& ctx, TechnoClass* unit, bool overlay) noexcept {
    auto* token = unit->PlanningToken;
    if (!token || !token->PlanningNodes.Count) return;
    auto start = project(unit->GetCenterCoords());
    if (unit->GetCurrentMission() == Mission::Wait || unit->CurrentMission == Mission::Wait) {
        start = project(event_coords(token->CurrentEvent, unit));
    } else if (unit->GetCurrentMission() != Mission::Guard) {
        CoordStruct destination;
        unit->GetDestination(&destination);
        const auto next = project(destination);
        const WORD value = overlay ? pixel(color(unit, false)) : border(unit);
        for (int offset = overlay ? 0 : -1; offset <= (overlay ? 0 : 1); ++offset)
            line(ctx, start, next, offset, value, overlay);
        marker(ctx, start, 6, value, overlay);
        marker(ctx, next, 6, value, overlay);
        start = next;
    }
    CoordStruct at;
    token->GetNode(0)->GetCoords(&at);
    const auto end = project(at);
    const WORD value = overlay ? pixel(color(unit, token->field_8C == 0)) : border(unit);
    for (int offset = overlay ? 0 : -1; offset <= (overlay ? 0 : 1); ++offset)
        line(ctx, start, end, offset, value, overlay);
    marker(ctx, start, 6, value, overlay);
}
}
void game::build_planning_underlay(PresentationContext& ctx) noexcept {
    if (!TacticalClass::Instance || !HouseClass::CurrentPlayer) return;
    for (int i = 0; i < PlanningNodeClass::Unknown1.Count; ++i) {
        auto* node = PlanningNodeClass::Unknown1[i];
        if (node->GetOwner(0)->Owner->IsAlliedWith(HouseClass::CurrentPlayer)) node_links(ctx, node, false);
    }
    for (int i = 0; i < TechnoClass::Array.Count; ++i) {
        auto* unit = TechnoClass::Array[i];
        if (unit->Owner->IsAlliedWith(HouseClass::CurrentPlayer)) unit_link(ctx, unit, false);
    }
}
void game::build_planning_overlay(PresentationContext& ctx) noexcept {
    if (!TacticalClass::Instance || !HouseClass::CurrentPlayer) return;
    for (int i = 0; i < PlanningNodeClass::Unknown1.Count; ++i) {
        auto* node = PlanningNodeClass::Unknown1[i];
        if (node->GetOwner(0)->Owner->IsAlliedWith(HouseClass::CurrentPlayer)) node_links(ctx, node, true);
    }
    // Original fills every node again after all branches to keep intersections legible.
    for (int i = 0; i < PlanningNodeClass::Unknown1.Count; ++i) {
        auto* node = PlanningNodeClass::Unknown1[i];
        if (node->GetOwner(0)->Owner->IsAlliedWith(HouseClass::CurrentPlayer)) {
            CoordStruct at;
            node->GetCoords(&at);
            marker(ctx, project(at), 8, WORD(node->field_B4), true);
        }
    }
    for (int i = 0; i < TechnoClass::Array.Count; ++i) {
        auto* unit = TechnoClass::Array[i];
        if (unit->Owner->IsAlliedWith(HouseClass::CurrentPlayer)) unit_link(ctx, unit, true);
    }
}
void game::build_planning_feedback(PresentationContext& ctx) noexcept {
    if (!TacticalClass::Instance) return;
    const auto& white = RGBClass::White;
    const WORD value = static_cast<WORD>(Drawing::RGB_To_Int(white.Red, white.Green, white.Blue));
    const auto highlight = [value, &ctx](PlanningNodeClass* node, int size) noexcept {
        if (!node) return;
        Point2D point;
        node->GetScreenCoords(&point);
        for (int i = 0; i < 3; ++i)
            marker(ctx, point, size - 2 * i, value, false);
    };
    highlight(Game::PlanningNodeAtAC4CCC, 16);
    highlight(Game::PlanningNodeAtAC4BF0, 24);
}
#endif
