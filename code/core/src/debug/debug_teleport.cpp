#include "debug/debug_state.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include "runtime_diagnostics.hpp"
#include "visibility_grid.hpp"
#include "yrpp/FootClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/JumpjetLocomotionClass.h"
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/AircraftTrackerClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <vector>
#include <memory>
#include <cstring>

namespace game {
template <int Capacity>
static void cancel_debug_pending_orders(QueueClass<EventClass, Capacity>& queue, FootClass& foot) noexcept {
    const TargetClass actor(&foot);
    for (int i = 0; i < queue.Count; ++i) {
        auto& event = queue[i];
        TargetClass* source = nullptr;
        switch (event.Type) {
            case EventType::MegaMission:
            case EventType::MegaMissionF:
                source = &event.MegaMission.Whom;
                break;
            case EventType::Archive:
                source = &event.Archive.Whom1;
                break;
            case EventType::Scatter:
                source = &event.Scatter.Whom;
                break;
            case EventType::Idle:
                source = &event.Idle.Whom;
                break;
            case EventType::Deploy:
                source = &event.Deploy.Whom;
                break;
            default:
                break;
        }
        if (!source || source->m_ID != actor.m_ID || source->m_RTTI != actor.m_RTTI) continue;
        // Keep ring order/timestamps and other actors' events. Original
        // dispatch treats a missing actor as an expired, harmless order.
        *source = TargetClass();
        if (event.Type == EventType::MegaMission || event.Type == EventType::MegaMissionF)
            event.MegaMission.IsPlanningEvent = 0;
    }
}
struct DebugLocomotorOwner {
    ILocomotion* pointer = nullptr;
    ~DebugLocomotorOwner() {
        if (pointer) pointer->Release();
    }
};
static DebugTargetFailure debug_teleport_state(FootClass& foot) {
    if (foot.IsDead()) return DebugTargetFailure::dead_object;
    if (!foot.IsOnMap || foot.InLimbo || foot.Transporter || foot.IsTether || foot.IsSinking || foot.IsFallingDown ||
        foot.IsImmobilized || foot.IsBeingWarpedOut() || foot.IsWarpingIn() || foot.IsAttackedByLocomotor ||
        foot.BunkerLinkedItem || !foot.Locomotor || foot.IsDeploying || foot.TubeIndex >= 0)
        return DebugTargetFailure::invalid_state;
    auto* type = foot.GetTechnoType();
    if (!type || type->IsTrain) return DebugTargetFailure::unsupported_type;
    return DebugTargetFailure::none;
}
DebugTargetFailure debug_teleport_unit(FootClass& foot, CoordStruct destination) {
    const auto state = debug_teleport_state(foot);
    if (state != DebugTargetFailure::none) return state;
    auto* type = foot.GetTechnoType();
    auto& map = MapClass::Instance;
    auto* cell = map.TryGetCellAt(destination);
    if (!cell || !map.IsWithinUsableArea(cell->MapCoords, true)) return DebugTargetFailure::blocked_destination;
    const int height = std::max(foot.GetHeight(), 0);
    const bool airborne = foot.IsInAir();
    const bool bridge = !airborne && (unsigned(cell->Flags) & 0x100u);
    if (!airborne) {
        for (auto* object = bridge ? cell->AltObject : cell->FirstObject; object; object = object->NextObject) {
            if (object == &foot || object->IsDead() || object->InLimbo) continue;
            if (object->WhatAmI() != AbstractType::Infantry || foot.WhatAmI() != AbstractType::Infantry ||
                object->Location.X == destination.X && object->Location.Y == destination.Y)
                return DebugTargetFailure::blocked_destination;
        }
    }
    destination.Z =
        map.GetCellFloorHeight(destination) + (bridge ? CellClass::BridgeHeight : 0) + (airborne ? height : 0);

    // Construct a clean instance of the original movement class before writes.
    // Old track/head coordinates must never pull the unit back on the next tick.
    DebugLocomotorOwner next;
    const auto created = LocomotionClass::CreateInstance(&next.pointer, &type->Locomotor, nullptr, 7);
    if (created < 0 || !next.pointer) return DebugTargetFailure::invalid_state;
    if (next.pointer->Link_To_Object(&foot) < 0) return DebugTargetFailure::invalid_state;
    if (!std::memcmp(&type->Locomotor, &LocomotionClass::CLSIDs::Jumpjet, sizeof(CLSID))) {
        auto& jumpjet = *static_cast<JumpjetLocomotionClass*>(next.pointer);
        jumpjet.State = airborne ? JumpjetLocomotionClass::Hovering : JumpjetLocomotionClass::Grounded;
        jumpjet.CurrentHeight = height;
        jumpjet.LocomotionFacing = foot.PrimaryFacing;
    } else if (!std::memcmp(&type->Locomotor, &LocomotionClass::CLSIDs::Fly, sizeof(CLSID))) {
        auto& fly = *static_cast<FlyLocomotionClass*>(next.pointer);
        fly.FlightLevel = height;
        fly.XYZ2 = destination;
    }
    if (foot.Team) foot.Team->LiberateMember(&foot, -1, 1);
    cancel_debug_pending_orders(EventClass::OutList, foot);
    cancel_debug_pending_orders(EventClass::DoList, foot);
    cancel_debug_pending_orders(EventClass::MegaMissionList, foot);
    foot.ClearPlanningTokens(nullptr);
    foot.SendToEachLink(RadioCommand::NotifyUnlink);
    foot.SetTarget(nullptr);
    foot.SetDestination(nullptr, true);
    foot.SetArchiveTarget(nullptr);
    foot.ClearNavigationList();
    foot.NavQueue.Clear();
    foot.ClearMegaMissionData();
    foot.PlanningPathIdx = -1;
    foot.WaypointIndex = 0;
    foot.PathDirections[0] = -1;
    foot.Locomotor->Mark_All_Occupation_Bits(MarkType::Up);
    foot.UnmarkAllOccupationBits(foot.Location);
    foot.Locomotor->Limbo();
    const auto old_layer = foot.InWhichLayer();
    if (foot.GetLastFlightMapCoords() != CellStruct{-1, -1}) AircraftTrackerClass::Instance.Remove(&foot);
    foot.Mark(MarkType::Up);
    foot.OnBridge = bridge;
    foot.SetLocation(destination);
    foot.UpdatePassengerCoords();
#if defined(_MSC_VER)
    foot.Locomotor = next.pointer;
#else
    auto* old = foot.Locomotor;
    foot.Locomotor = next.pointer;
    next.pointer->AddRef();
    old->Release();
#endif
    foot.Locomotor->Unlimbo();
    foot.Locomotor->Power_On();
    foot.FrozenStill = true;
    foot.CurrentMapCoords = cell->MapCoords;
    foot.Mark(MarkType::Down);
    if (!airborne) foot.MarkAllOccupationBits(destination);
    if (old_layer != foot.InWhichLayer()) {
        DisplayClass::Remove(&foot);
        DisplayClass::Submit(&foot);
    }
    if (airborne && type->ConsideredAircraft) AircraftTrackerClass::Instance.Add(&foot);
    foot.ForceMission(Mission::Guard);
    foot.IsInPlayfield = true;
    // Preserve the sight handoff from infantry arrival (0x51A8D7..0x51A925).
    // Releasing the cached old circle is required even after Location changes.
    {
        SightMoveScope sight_move;
        foot.vt_entry_48C(false, 0, false, nullptr);
        if (!foot.IsDead() && !foot.InLimbo) foot.UpdateSight(false, 0, false, nullptr, 0);
    }
    if (foot.IsDead() || foot.InLimbo) {
        map_object_changed();
        return DebugTargetFailure::none;
    }
    auto sight_at = foot.Location;
    map.RevealArea3(&sight_at, 0, foot.LastSightRange + 3, false);
    // Use the original base arrival bookkeeping without entering transport,
    // harvesting or deployment arrival overrides.
    foot.FootClass::UpdatePosition(PCPType::End);
    if (!foot.IsDead() && !airborne) {
        if (foot.WhatAmI() == AbstractType::Infantry && !bridge && cell->LandType == LandType::Water &&
            foot.IsCellOccupied(cell, FacingType::None, -1, nullptr, true) == Move::No) {
            // Chronosphere landing kills non-swimming infantry through C4,
            // which produces land death sequences/corpses. Debug relocation
            // instead enters the existing sinking state without combat damage.
            foot.IsSinking = true;
            foot.Stun();
            static_cast<InfantryClass&>(foot).PlayAnim(Sequence::Ready, true);
        } else {
            TeleportLocomotionClass landing;
            landing.Link_To_Object(&foot);
            landing.CheckChronoLanding(destination);
        }
    }
    foot.NeedsRedraw = true;
    map_object_changed();
    return DebugTargetFailure::none;
}

struct DebugTeleportTarget {
    MapObjectId id;
    CellStruct destination;
};
using DebugMegaMissionQueue = QueueClass<EventClass, EventClass::MEGA_CAPACITY>;
struct DebugMegaMissionScope {
    std::unique_ptr<DebugMegaMissionQueue> previous;
    DebugMegaMissionScope() : previous(std::make_unique<DebugMegaMissionQueue>(EventClass::MegaMissionList)) {}
    ~DebugMegaMissionScope() { EventClass::MegaMissionList = *previous; }
};
static void assign_debug_teleport_destinations(MapWorld& world, std::vector<DebugTeleportTarget>& targets,
                                               CellStruct destination) {
    // Reuse ordinary movement's original group distribution, also used by
    // Chrono Legionnaires. Restore every pending mission before relocating.
    DebugMegaMissionScope scope;
    auto& queue = EventClass::MegaMissionList;
    for (std::size_t first = 0; first < targets.size(); first += EventClass::MEGA_CAPACITY) {
        queue.Init();
        const auto count = std::min(targets.size() - first, std::size_t(EventClass::MEGA_CAPACITY));
        for (std::size_t i = 0; i < count; ++i) {
            auto* actor = resolve_map_object(world, targets[first + i].id);
            const EventClass order(HouseClass::CurrentPlayer->ArrayIndex, TargetClass(actor), Mission::Move,
                                   TargetClass(), TargetClass(MapClass::Instance.GetCellAt(destination)),
                                   TargetClass());
            queue.Add(order, 0);
        }
        if (count > 1) EventClass::AdjustMegaMissionDestinations(0, int(count));
        for (std::size_t i = 0; i < count; ++i) {
            if (auto* cell = queue[int(i)].MegaMission.Destination.As_Cell())
                targets[first + i].destination = cell->MapCoords;
        }
    }
}
static bool debug_teleport_cell_position(FootClass& foot, CellClass& cell, CoordStruct& output) {
    auto& map = MapClass::Instance;
    if (!map.IsWithinUsableArea(cell.MapCoords, true)) return false;
    auto position = cell.GetCoords();
    if (foot.IsInAir()) {
        output = position;
        return true;
    }
    const bool bridge = (unsigned(cell.Flags) & 0x100u) != 0;
    for (auto* object = bridge ? cell.AltObject : cell.FirstObject; object; object = object->NextObject) {
        if (object == &foot || object->IsDead() || object->InLimbo) continue;
        if (foot.WhatAmI() != AbstractType::Infantry || object->WhatAmI() != AbstractType::Infantry) return false;
    }
    if (foot.WhatAmI() == AbstractType::Infantry) {
        position = cell.FindInfantrySubposition(position, false, bridge, true);
        if (position == CoordStruct::Empty) return false;
    }
    output = position;
    return true;
}
static bool debug_teleport_move_position(FootClass& foot, CellStruct destination, CoordStruct& output) {
    auto& map = MapClass::Instance;
    auto* cell = map.TryGetCellAt(destination);
    if (!cell || !map.IsWithinUsableArea(destination, true)) return false;
    if (debug_teleport_cell_position(foot, *cell, output)) return true;
    // Occupied destinations get the same nearby-cell search as the original
    // teleport locomotor. Debug water clicks additionally allow sinking there.
    if (cell->LandType == LandType::Water && !(unsigned(cell->Flags) & 0x100u)) {
        const auto count = CellSpread::NumCells(6);
        for (std::size_t i = 1; i < count; ++i) {
            const auto offset = CellSpread::GetCell(i);
            const CellStruct candidate{short(destination.X + offset.X), short(destination.Y + offset.Y)};
            auto* nearby = map.TryGetCellAt(candidate);
            if (nearby && nearby->LandType == LandType::Water && !(unsigned(nearby->Flags) & 0x100u) &&
                debug_teleport_cell_position(foot, *nearby, output))
                return true;
        }
        return false;
    }
    const auto* type = foot.GetTechnoType();
    const auto nearby = map.NearByLocation(destination, type->SpeedType, -1, type->MovementZone, false, 1, 1, false,
                                           true, false, true, CellStruct::Empty, false, false);
    auto* target = map.TryGetCellAt(nearby);
    return nearby != CellStruct::Empty && target && debug_teleport_cell_position(foot, *target, output);
}
bool debug_teleport_selected(CellStruct destination) noexcept {
    if (!debug_teleport_active()) return false;
    auto* world = current_map_world();
    if (!world) return false;
    try {
        std::vector<DebugTeleportTarget> targets;
        unsigned applied = 0, skipped = 0;
        DebugTargetFailure failure = DebugTargetFailure::none;
        const auto fail = [&](DebugTargetFailure reason) {
            ++skipped;
            if (failure == DebugTargetFailure::none) failure = reason;
        };
        for (auto* object : ObjectClass::CurrentObjects) {
            if ((object->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None ||
                !debug_can_control_house(object->GetOwningHouse()))
                continue;
            auto* foot = static_cast<FootClass*>(object);
            if (!foot->IsControllable()) continue;
            const auto state = debug_teleport_state(*foot);
            if (state != DebugTargetFailure::none) {
                fail(state);
                continue;
            }
            targets.push_back({object_id(*world, object), destination});
        }
        if (targets.empty() && !skipped) return true;
        auto* clicked = MapClass::Instance.TryGetCellAt(destination);
        const bool usable = clicked && MapClass::Instance.IsWithinUsableArea(destination, true);
        if (usable && !targets.empty()) assign_debug_teleport_destinations(*world, targets, destination);
        for (const auto& target : targets) {
            auto* object = resolve_map_object(*world, target.id);
            if (!object) {
                fail(DebugTargetFailure::expired_object);
                continue;
            }
            CoordStruct at;
            if (!usable || !debug_teleport_move_position(static_cast<FootClass&>(*object), target.destination, at)) {
                fail(DebugTargetFailure::blocked_destination);
                continue;
            }
            const auto reason = debug_teleport_unit(static_cast<FootClass&>(*object), at);
            if (reason == DebugTargetFailure::none)
                ++applied;
            else
                fail(reason);
        }
        runtime_log_event("debug_teleport", "world=%llu cell=%d,%d applied=%u skipped=%u reason=%s",
                          static_cast<unsigned long long>(world->impl->world_id), destination.X, destination.Y, applied,
                          skipped, debug_target_failure_name(failure));
        if (skipped) {
            wchar_t message[128];
            std::swprintf(message, 128, L"Teleport: %u moved, %u skipped (%hs)", applied, skipped,
                          debug_target_failure_name(failure));
            Game::ShowMessage(message, 120);
        }
        refresh_map_world_hover(*world);
    } catch (...) {
        runtime_log_event("debug_teleport_failure", "reason=exception");
        Game::ShowMessage(L"Teleport interrupted", 120);
    }
    return true;
}
}
