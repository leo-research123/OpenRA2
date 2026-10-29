// Original PlanMgr graph and interaction drawing (0x63B0A0 / 0x63B150 / 0x63B2F0).
// The existing native raster target replaces the original Composite surface.
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "planning_cycle.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <new>
namespace game {
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
void prepare_node(PlanningNodeClass* node, bool overlay) noexcept {
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
        if (branch->field_74 == -1 && branch_owner) branch->field_74 = pixel(color(branch_owner, pending));

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
        if (branch.field_74 == -1 && loop_owner) branch.field_74 = pixel(color(loop_owner, pending));

    }
    if (!overlay) {
        const auto clip = DSurface::ViewBounds;
        if (end.X + 4 > clip.X && end.Y + 4 > clip.Y && end.X - 4 < clip.X + clip.Width &&
            end.Y - 4 < clip.Y + clip.Height) {
            try {
                PlanningNodeClass::Unknown2.AddItem(node);
            } catch (const std::bad_alloc&) {
                PlanningNodeClass::Unknown2.IsInitialized = true;
            }
        }
    } else {
        if (node->field_B4 == -1) {
            const auto dark = pixel(color(owner, false));
            bool pending = true;
            for (int i = 0; i < node->PlanningBranches.Count; ++i)
                if (node->PlanningBranches[i]->field_74 == dark) {
                    pending = false;
                    break;
                }
            if (!node->PlanningBranches.Count) pending = !committed(node, owner);
            node->field_B4 = pixel(color(owner, pending));
        }
    }
}
}
void advance_planning_view() noexcept {
    PlanningNodeClass::Unknown2.Clear();
    if (!TacticalClass::Instance || !HouseClass::CurrentPlayer) return;
    for (bool overlay : {false, true})
        for (int i = 0; i < PlanningNodeClass::Unknown1.Count; ++i) {
            auto* node = PlanningNodeClass::Unknown1[i];
            if (node->GetOwner(0)->Owner->IsAlliedWith(HouseClass::CurrentPlayer)) prepare_node(node, overlay);
        }
}
}
