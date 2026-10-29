// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 reinf.cpp Do_Reinforcements/Create_Group/Place_Reinforcements.
// YR 0x65D8E0/0x65DD30/0x65E010: explicit map waypoint reinforcement path.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TActionClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/ScriptClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/RulesClass.h"
#include <bit>
#include <cstdlib>
#include <memory>
#include <vector>
#include <stdexcept>
#if !defined(RA2_YRPP_GAME)
namespace {
struct ReinforcementInitScope {
    ReinforcementInitScope() { ++Unsorted::ScenarioInit; }
    ~ReinforcementInitScope() { --Unsorted::ScenarioInit; }
};

FootClass* create_reinforcement_group(TeamTypeClass& type) {
    std::unique_ptr<TeamClass> team(GameCreate<TeamClass>(&type, type.GetHouse(), 0));
    if (!team || !team->CurrentScript) return nullptr;
    team->IsForcedActive = true;
    team->IsUnderStrength = false;
    bool hasUnload = false, aircraftTransport = false, naval = false;
    for (int i = 0; i < type.ScriptType->ActionsCount; ++i)
        hasUnload |= type.ScriptType->ScriptActions[i].Action == 8;
    for (int i = 0; i < type.TaskForce->CountEntries; ++i) {
        const auto* member = type.TaskForce->Entries[i].Type;
        aircraftTransport |= member->WhatAmI() == AbstractType::AircraftType && member->Passengers > 0;
        naval |= member->Naval;
    }
    FootClass* head = nullptr;
    FootClass* transports = nullptr;
    try {
        for (int i = 0; i < type.TaskForce->CountEntries; ++i) {
            const auto& entry = type.TaskForce->Entries[i];
            for (int n = 0; n < entry.Amount; ++n) {
                ObjectClass* object;
                {
                    ReinforcementInitScope init;
                    object = entry.Type->CreateObject(team->Owner);
                }
                if (!object) continue;
                auto* foot = static_cast<FootClass*>(object);
                // Unsupported native locomotors cannot publish a partial
                // group. The outer host boundary contains this failure.
                if (!foot->Locomotor) {
                    delete foot;
                    throw std::runtime_error("Unsupported reinforcement locomotor");
                }
                if (entry.Type->Storage > 0 && type.Full) foot->Tiberium.AddAmount(float(entry.Type->Storage), 0);
                if (type.VeteranLevel == 0)
                    foot->Veterancy.SetRookie();
                else if (type.VeteranLevel == 2)
                    foot->Veterancy.SetVeteran();
                else if (type.VeteranLevel == 3)
                    foot->Veterancy.SetElite();
                {
                    ReinforcementInitScope init;
                    team->AddMember(foot, false);
                }
                foot->IsInitiated = true;
                const bool transport =
                    type.Full && entry.Type->Passengers > 0 &&
                    foot->WhatAmI() == (aircraftTransport ? AbstractType::Aircraft : AbstractType::Unit) &&
                    (entry.Type->Naval || !naval);
                auto*& list = transport ? transports : head;
                foot->NextObject = list;
                list = foot;
            }
        }
    } catch (...) {
        while (head) {
            auto* next = static_cast<FootClass*>(head->NextObject);
            head->NextObject = nullptr;
            if (head->Team) head->Team->LiberateMember(head, -1, 1);
            delete head;
            head = next;
        }
        while (transports) {
            auto* next = static_cast<FootClass*>(transports->NextObject);
            transports->NextObject = nullptr;
            if (transports->Team) transports->Team->LiberateMember(transports, -1, 1);
            delete transports;
            transports = next;
        }
        throw;
    }
    // 0x65DF63: the original attaches the entire cargo chain to the first
    // transport. Cargo stays in limbo; only the transport list is placed.
    if (transports) {
        if (head) {
            transports->Passengers.AddPassenger(head);
            if (hasUnload && transports->WhatAmI() == AbstractType::Aircraft) transports->IsALoaner = true;
        }
        head = transports;
    }
    if (head) team.release();
    return head;
}

bool place_reinforcement_group(TeamTypeClass& type, FootClass* group, const CellStruct& origin, bool within) {
    auto& map = MapClass::Instance;
    auto at = origin;
    int facing = 0;
    if (within) {
        at = map.NearByLocation(at, group->GetTechnoType()->SpeedType, -1, MovementZone::Normal, false, 1, 1, false,
                                false, false, true, CellStruct{0, 0}, false, false);
    } else if (auto* house = type.GetHouse()) {
        if (house->StartingEdge != Edge::None) facing = int(house->StartingEdge) * 64;
    }
    bool placed = false;
    while (group) {
        auto* next = static_cast<FootClass*>(group->NextObject);
        group->NextObject = nullptr;
        bool admitted = false;
        while (at != CellStruct{0, 0}) {
            ReinforcementInitScope init;
            const auto direction =
                group->WhatAmI() == AbstractType::Aircraft ? DirType((facing - 97) & 0xE0) : DirType(facing);
            admitted = group->Unlimbo(map.GetCellAt(at)->GetCoords(), direction);
            if (admitted) {
                placed = true;
                if (group->WhatAmI() != AbstractType::Aircraft) {
                    group->QueueMission(Mission::Guard, false);
                    group->NextMission();
                }
                break;
            }
            // 0x65E506: walk the original adjacency order outside the legal
            // map diamond. A blocked unit retries before the next member.
            CellStruct candidate{0, 0};
            for (int i = 0; i < 8; ++i) {
                const auto delta = Unsorted::AdjacentCell[i];
                const CellStruct cell{short(at.X + delta.X), short(at.Y + delta.Y)};
                if (!map.CoordinatesLegal(cell) &&
                    group->IsCellOccupied(map.GetCellAt(cell), FacingType(i), -1, nullptr, true) == Move::OK) {
                    candidate = cell;
                    break;
                }
            }
            at = candidate;
            if (at == CellStruct{0, 0}) {
                if (group->Team) group->Team->LiberateMember(group, -1, 1);
                delete group;
                group = nullptr;
            }
        }
        if (!admitted && group) {
            // Native ownership repair: 0x65E5AE advances past the next
            // member when cells run out. Release that stranded member too.
            if (group->Team) group->Team->LiberateMember(group, -1, 1);
            delete group;
        }
        group = next;
    }
    return placed;
}

bool reinforce_special_team(TeamTypeClass& type, int waypoint) {
    if (!type.Owner || !type.TaskForce || !type.ScriptType || type.Loadable) return false;
    const auto cell = ScenarioClass::Instance->GetWaypointCoords(waypoint == -1 ? type.Waypoint : waypoint);
    auto* spawn = MapClass::Instance.TryGetCellAt(cell);
    if (!spawn) return false;
    std::unique_ptr<TeamClass> team(GameCreate<TeamClass>(&type, type.Owner, 0));
    if (!team || !team->CurrentScript) return false;
    team->IsForcedActive = true;
    team->IsUnderStrength = false;
    std::vector<std::unique_ptr<FootClass>> members;
    const int previous = Unsorted::ScenarioInit;
    struct Restore {
        int depth;
        ~Restore() { Unsorted::ScenarioInit = depth; }
    } restore{previous};
    ++Unsorted::ScenarioInit;
    for (int i = 0; i < type.TaskForce->CountEntries; ++i) {
        const auto& entry = type.TaskForce->Entries[i];
        if (!entry.Type) return false;
        for (int n = 0; n < entry.Amount; ++n) {
            auto* object = entry.Type->CreateObject(type.Owner);
            if (!object) return false;
            if (!(static_cast<unsigned>(object->AbstractFlags) & static_cast<unsigned>(AbstractFlags::Foot))) {
                delete object;
                return false;
            }
            std::unique_ptr<FootClass> foot(static_cast<FootClass*>(object));
            if (foot->WhatAmI() == AbstractType::Unit && !static_cast<UnitClass*>(foot.get())->InitializeLocomotor())
                return false;
            if (foot->WhatAmI() == AbstractType::Infantry &&
                !static_cast<InfantryClass*>(foot.get())->InitializeLocomotor())
                return false;
            if (foot->WhatAmI() == AbstractType::Aircraft &&
                !static_cast<AircraftClass*>(foot.get())->InitializeLocomotor())
                return false;
            auto at = spawn->GetCoords();
            const auto facing = foot->WhatAmI() == AbstractType::Aircraft ? DirType::South : DirType::North;
            if (!type.Droppod && !foot->Unlimbo(at, facing)) return false;
            if (type.VeteranLevel == 2)
                foot->Veterancy.SetVeteran();
            else if (type.VeteranLevel == 3)
                foot->Veterancy.SetElite();
            members.push_back(std::move(foot));
        }
    }
    for (auto& foot : members) {
        if (!team->AddMember(foot.get(), false)) {
            while (team->FirstUnit)
                team->LiberateMember(team->FirstUnit);
            return false;
        }
        foot->IsInitiated = true;
    }
    if (type.Droppod) {
        // YR 0x65D8E0 replaces TS drop pods with a PDPLANE carrying the
        // existing team. Cargo remains in limbo until ParadropCargo places it.
        auto* planeType = AircraftTypeClass::Find("PDPLANE");
        if (!planeType) return false;
        std::unique_ptr<AircraftClass> plane(GameCreate<AircraftClass>(planeType, type.Owner));
        if (!plane) return false;
        plane->IsALoaner = true;
        CellStruct origin;
        if (type.UseTransportOrigin)
            origin = ScenarioClass::Instance->GetWaypointCoords(type.TransportWaypoint);
        else {
            auto edge = type.Owner->StartingEdge;
            if (unsigned(edge) >= 4) edge = unsigned(type.Owner->Edge) < 4 ? type.Owner->Edge : Edge::North;
            origin = MapClass::Instance.PickCellOnEdge(edge, CellStruct::Empty, CellStruct::Empty, SpeedType::Winged,
                                                       true, MovementZone::Normal);
        }
        auto* entry = MapClass::Instance.TryGetCellAt(origin);
        if (!entry) return false;
        plane->QueueMission(Mission::ParadropApproach, false);
        plane->SetDestination(nullptr, true);
        plane->SetTarget(spawn);
        if (!plane->Unlimbo(entry->GetCoords(), DirType::North)) return false;
        plane->HasPassengers = true;
        for (auto& member : members)
            plane->Passengers.AddPassenger(member.get());
        plane->NextMission();
        plane.release();
    }
    for (auto& foot : members)
        foot.release();
    team.release();
    return true;
}

bool reinforce_team(TeamTypeClass& type, int waypoint) noexcept {
    try {
        if (type.Droppod) return reinforce_special_team(type, waypoint);
        if (!type.GetHouse() || !type.TaskForce || !type.TaskForce->CountEntries) return false;
        if (!type.ScriptType) type.ScriptType = GameCreate<ScriptTypeClass>(nullptr);
        if (!type.ScriptType->ActionsCount) type.ScriptType->ScriptActions[type.ScriptType->ActionsCount++] = {5, 0};
        auto* group = create_reinforcement_group(type);
        if (!group) return false;
        group->Team->IsTransient = false;
        const auto cell = ScenarioClass::Instance->GetWaypointCoords(waypoint == -1 ? type.Waypoint : waypoint);
        const bool within = waypoint != -1 && MapClass::Instance.IsWithinUsableArea(cell, true);
        if (place_reinforcement_group(type, group, cell, within))
            if (auto* house = type.GetHouse();
                house && HouseClass::CurrentPlayer && house->IsAlliedWith(HouseClass::CurrentPlayer))
                RadarEventClass::AddHistory(cell);
        // 0x65DB9C returns success after attempting placement, including failure.
        return true;
    } catch (...) {
        return false;
    }
}

// YR 0x65EC30. The chrono path creates a group directly (ignoring Max),
// then uses the original temporary teleport locomotor and materialization arm.
bool reinforce_team_by_chrono(TeamTypeClass& type, int waypoint) noexcept {
    try {
        if (!type.TaskForce || !type.TaskForce->CountEntries) return false;
        if (!type.ScriptType) type.ScriptType = GameCreate<ScriptTypeClass>(nullptr);
        if (!type.ScriptType) return false;
        if (!type.ScriptType->ActionsCount) type.ScriptType->ScriptActions[type.ScriptType->ActionsCount++] = {5, 0};
        auto* group = create_reinforcement_group(type);
        // The original dereferences these before its null checks. Native
        // allocation/no-member failure is contained on this side instead.
        if (!group) return false;
        group->Team->IsTransient = false;
        CellStruct origin;
        type.GetWaypoint(&origin);
        if (waypoint != -1) origin = ScenarioClass::Instance->GetWaypointCoords(waypoint);
        FootClass* admitted = nullptr;
        auto& map = MapClass::Instance;
        const auto check = [](HRESULT result) {
            if (result < 0 && result != static_cast<HRESULT>(0x80004002u)) std::abort();
        };
        while (group) {
            auto* nextGroup = static_cast<FootClass*>(group->NextObject);
            group->NextObject = nullptr;
            const bool unit = group->WhatAmI() == AbstractType::Unit;
            const bool aircraft = group->WhatAmI() == AbstractType::Aircraft;
            CoordStruct coordinateOutput;
            auto destination = *map.GetCellAt(origin)->GetCoords(&coordinateOutput);
            if (!unit && !aircraft) {
                destination.X =
                    std::bit_cast<int>(unsigned(destination.X) + ((unsigned(group->Ammo) & 3u) - 2u) * 256u);
                destination.Y =
                    std::bit_cast<int>(unsigned(destination.Y) + ((unsigned(group->Ammo) & 0xCu) - 4u) * 64u);
            }
            if (!aircraft) group->Ammo = std::bit_cast<int>(unsigned(group->Ammo) - 1u);
            auto* cell = map.GetCellAt(destination);
            CoordStruct found = CoordStruct::Empty;
            for (int attempts = 0; found == CoordStruct::Empty && attempts < 255; ++attempts) {
                const auto cellCoords = *cell->GetCoords(&coordinateOutput);
                const auto position = CellClass::Coord2Cell(cellCoords);
                CellStruct output;
                const auto nearby = *map.NearByLocation(
                    output, position, aircraft ? SpeedType::Track : group->GetTechnoType()->SpeedType, -1,
                    MovementZone::Normal, false, 1, 1, false, false, false, true, CellStruct{0, 0}, false, false);
                cell = map.GetCellAt(nearby);
                const auto coords = *cell->GetCoords(&coordinateOutput);
                if (unit || aircraft)
                    found = coords;
                else {
                    CoordStruct outputCoords;
                    found = *cell->FindInfantrySubposition(&outputCoords, coords, false, false, false);
                }
            }
            // 0x65EFE0 only replaces X/Y. Z remains the waypoint cell's
            // original height, even when the search moves to another cell.
            destination.X = found.X;
            destination.Y = found.Y;
            bool placed;
            {
                ReinforcementInitScope init;
                placed = group->Unlimbo(destination, DirType::North);
            }
            if (!placed) {
                delete group;
                group = nextGroup;
                continue;
            }
            ILocomotion* old = group->Locomotor;
            IPiggyback* piggy = nullptr;
            if (old) check(LocomotionClass::TryPiggyback(&piggy, &old));
            ILocomotion* teleport = nullptr;
            check(LocomotionClass::CreateInstance(&teleport, &LocomotionClass::CLSIDs::Teleport, nullptr, 7));
            if (!teleport) std::abort();
            teleport->Link_To_Object(group);
            IPiggyback* acquired = nullptr;
            check(LocomotionClass::TryPiggyback(&acquired, &teleport));
            // Match the original smart-pointer replacement order, including
            // the old locomotor's queried piggyback interface and temporary.
            if (piggy != acquired) {
                auto* previous = piggy;
                piggy = acquired;
                if (piggy) piggy->AddRef();
                if (previous) previous->Release();
            }
            if (acquired) acquired->Release();
            if (piggy) {
                piggy->Begin_Piggyback(group->Locomotor);
#if defined(_MSC_VER)
                group->Locomotor = teleport;
#else
                if (group->Locomotor != teleport) {
                    auto* previous = group->Locomotor;
                    group->Locomotor = teleport;
                    teleport->AddRef();
                    if (previous) previous->Release();
                }
#endif
            }
            group->ChronoDestCoords = destination;
            if (!group->Locomotor) std::abort();
            group->Locomotor->Move_To(destination);
            group->WarpingOut = true;
            group->NextObject = admitted;
            admitted = group;
            group->ChronoLockRemaining = RulesClass::Instance->ChronoReinfDelay;
            group = nextGroup;
            teleport->Release();
            if (piggy) piggy->Release();
        }
        if (!admitted) return false;
        while (admitted) {
            auto* next = static_cast<FootClass*>(admitted->NextObject);
            admitted->NextObject = nullptr;
            admitted->Mark(MarkType::Up);
            admitted->unknown_280 = 3;
            admitted->NextMission();
            admitted = next;
        }
        if (type.GetHouse()) {
            if (type.GetHouse()->IsAlliedWith(HouseClass::CurrentPlayer)) RadarEventClass::AddHistory(origin);
        }
        return true;
    } catch (...) {
        std::abort();
    }
}
}

bool TActionClass::SpawnTeam(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) {
    return TeamType && reinforce_team(*TeamType, -1);
}
bool TActionClass::SpawnTeamAtWP(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) {
    return TeamType && Waypoint != -1 && reinforce_team(*TeamType, Waypoint);
}
bool TActionClass::ChronoInTeam(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    return TeamType && Waypoint != -1 && reinforce_team_by_chrono(*TeamType, Waypoint);
}

#endif
