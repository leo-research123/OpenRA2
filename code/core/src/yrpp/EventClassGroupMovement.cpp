// YR Queue.CPP 0x0064CDA0: group destinations, including the original six-step search.
#include "yrpp/EventClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/AStarClass.h"
#include "yrpp/IndexClass.h"
#include "yrpp/YRMath.h"
#include <bit>
#include <exception>

namespace {
int subtract(int a, int b) noexcept { return std::bit_cast<int>(static_cast<unsigned>(a) - static_cast<unsigned>(b)); }
CoordStruct centroid(const DynamicVectorClass<ObjectClass*>& objects) noexcept {
    // 0x005F6790 sums Location in reverse order with 32-bit wrapping.
    unsigned x = 0, y = 0, z = 0;
    for (int i = objects.Count - 1; i >= 0; --i) {
        const auto& at = objects[i]->Location;
        x += unsigned(at.X);
        y += unsigned(at.Y);
        z += unsigned(at.Z);
    }
    return {std::bit_cast<int>(x) / objects.Count, std::bit_cast<int>(y) / objects.Count,
            std::bit_cast<int>(z) / objects.Count};
}
ObjectClass* closest(const DynamicVectorClass<ObjectClass*>& objects, const CoordStruct& center) {
    // 0x005F6810 keeps the last member on equal integer distances.
    ObjectClass* result = nullptr;
    int distance = 99999999;
    for (int i = objects.Count - 1; i >= 0; --i) {
        const auto& at = objects[i]->Location;
        const double x = subtract(center.X, at.X), y = subtract(center.Y, at.Y), z = subtract(center.Z, at.Z);
        const int next = int(Math::sqrt(x * x + y * y + z * z));
        if (!result || next < distance) {
            result = objects[i];
            distance = next;
        }
    }
    return result;
}
bool bridge(const CellClass* cell) noexcept { return (static_cast<unsigned>(cell->Flags) & 0x100u) != 0; }
void reserve(CellClass* cell) noexcept {
    cell->Flags = static_cast<CellFlags>(static_cast<unsigned>(cell->Flags) | 0x8000u);
}
void set_destination(int first, int count, ObjectClass* object, const CellStruct& destination) {
    const TargetClass target(MapClass::Instance.GetCellAt(destination)), actor(object);
    // Original search includes the slot immediately following the group.
    for (int i = first; i <= first + count; ++i) {
        auto& mission = EventClass::MegaMissionList[i].MegaMission;
        if (mission.Whom.m_ID == actor.m_ID && mission.Whom.m_RTTI == actor.m_RTTI) {
            mission.Destination = target;
            break;
        }
    }
}
}

void YRPP_FASTCALL EventClass::AdjustMegaMissionDestinations(int first, int count) noexcept {
    try {
        auto& destination = MegaMissionList[first].MegaMission.Destination;
        if (destination.m_RTTI != static_cast<BYTE>(AbstractType::Cell)) return;
        auto* target = destination.As_Cell();
        if (!target || target == &MapClass::InvalidCell || target->MapCoords == CellStruct::Empty) return;
        const CellStruct origin = target->MapCoords;
        auto& map = MapClass::Instance;
        DynamicVectorClass<ObjectClass*> objects;
        for (int i = first; i < first + count; ++i) {
            auto* object = MegaMissionList[i].MegaMission.Whom.As_Techno();
            if (object && (object->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None &&
                static_cast<FootClass*>(object)->TubeIndex < 0)
                objects.AddItem(object);
        }
        if (!objects.Count) return;
        auto* center = closest(objects, centroid(objects));
        IndexClass<int, ObjectClass*> ordered;
        for (int i = 0; i < objects.Count; ++i) {
            const auto reference = center->Location;
            const auto at = objects[i]->GetCoords();
            const double x = subtract(at.X, reference.X), y = subtract(at.Y, reference.Y),
                         z = subtract(at.Z, reference.Z);
            const int distance = int(Math::sqrt(x * x + y * y + z * z));
            ordered.AddIndex(std::bit_cast<int>(unsigned(i) + 1000u * unsigned(distance)), objects[i]);
        }
        objects.Clear();
        DynamicVectorClass<CellStruct> destinations;
        DynamicVectorClass<bool> assigned;
        ordered.Sort();
        for (const auto& entry : ordered) {
            objects.AddItem(entry.Data);
            destinations.AddItem(origin);
            assigned.AddItem(false);
        }
        const auto center_cell = center->GetMapCoords();
        for (int i = 0; i < objects.Count; ++i) {
            auto* object = objects[i];
            CellStruct result = origin;
            if (i == 0) {
                reserve(map.GetCellAt(origin));
                assigned[0] = true;
            } else if (!assigned[i]) {
                const auto at = object->GetDestination();
                const bool origin_bridge = bridge(map.GetCellAt(origin));
                const int level = static_cast<signed char>(map.GetCellAt(origin)->Level) + 4 * int(origin_bridge);
                float x = float(origin.X) + 0.5f, y = float(origin.Y) + 0.5f;
                const float dx = float(short(at.X / 256) - center_cell.X),
                            dy = float(short(at.Y / 256) - center_cell.Y);
                const double length = Math::sqrt(double(dx) * dx + double(dy) * dy);
                const float step_x = length == 0 ? dx : float(dx / length),
                            step_y = length == 0 ? dy : float(dy / length);
                auto* type = object->GetTechnoType();
                const int zone = map.GetMovementZoneType(origin, type->MovementZone, true);
                bool crossed_blocker = false;
                for (int step = 0; step < 6; ++step) {
                    x = float(x + step_x);
                    y = float(y + step_y);
                    CellStruct candidate{short(int(x)), short(int(y))};
                    auto* cell = map.GetCellAt(candidate);
                    if (!map.IsWithinUsableArea(candidate, true)) break;
                    const int next_zone = map.GetMovementZoneType(candidate, type->MovementZone, bridge(cell));
                    const auto move = object->IsCellOccupied(cell, FacingType::None, level, nullptr, true);
                    if (next_zone != zone)
                        crossed_blocker = true;
                    else if ((static_cast<unsigned>(cell->Flags) & 0x8000u) != 0)
                        result = candidate;
                    else {
                        const int next_level = static_cast<signed char>(cell->Level) + 4 * int(bridge(cell));
                        if (std::abs(level - next_level) > 2) break;
                        if (int(move) < 4 || int(move) == 6) {
                            if (crossed_blocker) {
                                // Original calls AttemptPath for its side effects and
                                // retains the last reserved cell regardless of return.
                                const bool to_bridge = bridge(map.GetCellAt(origin));
                                const bool from_bridge = bridge(map.GetCellAt(candidate));
                                auto from = candidate, to = origin;
                                AStarClass::Instance.AttemptPath(&from, &to, static_cast<FootClass*>(object),
                                                                 from_bridge, to_bridge, MovementZone::None);
                            } else
                                result = candidate;
                            break;
                        }
                        crossed_blocker = true;
                    }
                }
                destinations[i] = result;
                set_destination(first, count, object, result);
                reserve(map.GetCellAt(result));
                assigned[i] = true;
            }
            // Members starting on the same cell share the assigned destination.
            for (int j = i + 1; j < objects.Count; ++j)
                if (!assigned[j]) {
                    const auto other = objects[j]->GetMapCoords();
                    if (object->GetMapCoords() == other) {
                        set_destination(first, count, objects[j], result);
                        assigned[j] = true;
                    }
                }
        }
        for (const auto& cell : destinations) {
            auto* value = map.GetCellAt(cell);
            value->Flags = static_cast<CellFlags>(static_cast<unsigned>(value->Flags) & ~0x8000u);
        }
    } catch (...) {
        std::terminate();
    }
}
