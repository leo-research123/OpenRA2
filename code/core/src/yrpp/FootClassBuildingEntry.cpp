// Original YR building-entry virtuals, 0x4DFA70..0x4E0078.
#include "yrpp/FootClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/YRMath.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <climits>

namespace {
int entry_distance(const FootClass* foot, const BuildingClass* building, bool yFirst = false) {
    const auto at = building->GetCoords();
    const auto origin = foot->GetCoords();
    const auto difference = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
    const double x = difference(origin.X, at.X), y = difference(origin.Y, at.Y), z = difference(origin.Z, at.Z);
    return game::x87_integer(Math::sqrt(yFirst ? x * x + y * y + z * z : x * x + z * z + y * y));
}
InfantryClass* entry_infantry(FootClass* foot) {
    return foot->WhatAmI() == AbstractType::Infantry ? static_cast<InfantryClass*>(foot) : nullptr;
}
}

int FootClass::Mission_Eaten() {
    auto* infantry = entry_infantry(this);
    bool destinationReady = false;
    if (Target && Target->WhatAmI() == AbstractType::Building) {
        destinationReady = Destination != nullptr;
        if (!destinationReady && infantry &&
            (infantry->Type->C4 || HasAbility(Ability::C4) || infantry->Type->Engineer))
            SetDestination(Target, true);
    }
    if (!destinationReady && !Destination) {
        EnterIdleMode(false, true);
        const auto coords = GetCoords();
        const CellStruct cell{short(coords.X / 256), short(coords.Y / 256)};
        if (MapClass::Instance.GetCellAt(cell)->GetBuilding()) Scatter(CoordStruct::Empty, true, false);
    }
    const int delay = game::x87_scaled_quotient_to_integer(CurrentMissionControl()->Rate, 900.0, 1);
    return std::bit_cast<int>(unsigned(delay) + unsigned(ScenarioClass::Instance->Random.RandomRanged(0, 2)));
}

bool FootClass::EnterGrinder() {
    BuildingClass* best = nullptr;
    int closest = INT_MAX;
    for (int i = Owner->Grinders.Count - 1; i >= 0; --i) {
        auto* building = Owner->Grinders[i];
        const int distance = entry_distance(this, building);
        if (distance < closest) {
            closest = distance;
            best = building;
        }
    }
    if (!best) return false;
    SetDestination(best, true);
    QueueMission(Mission::Eaten, false);
    return true;
}

bool FootClass::EnterBioReactor() {
    BuildingClass* best = nullptr;
    int closest = INT_MAX;
    for (int i = Owner->Absorbers.Count - 1; i >= 0; --i) {
        auto* building = Owner->Absorbers[i];
        if (SendCommand(RadioCommand::QueryCanEnter, building) != RadioCommand::AnswerPositive) continue;
        const int distance = entry_distance(this, building);
        if (distance < closest) {
            closest = distance;
            best = building;
        }
    }
    ShouldEnterAbsorber = best != nullptr;
    if (!best) return false;
    if (GetNthLink() != best && QueueUpToEnter != best) {
        QueueMission(Mission::Enter, false);
        SetDestination(best, true);
    }
    return true;
}

bool FootClass::EnterTankBunker() {
    if (!CanEnterTankBunker()) return false;
    BuildingClass* best = nullptr;
    int closest = INT_MAX;
    for (int i = Owner->Bunkers.Count - 1; i >= 0; --i) {
        auto* building = Owner->Bunkers[i];
        const int distance = entry_distance(this, building);
        if (distance < closest && !building->GetNthLink() && !building->BunkerLinkedItem) {
            closest = distance;
            best = building;
        }
    }
    if (!best) return false;
    SetDestination(best, true);
    SendCommand(RadioCommand::RequestLink, best);
    QueueMission(Mission::Enter, true);
    return true;
}

bool FootClass::EnterBattleBunker() {
    BuildingClass* best = nullptr;
    int closest = INT_MAX;
    for (int i = Owner->Occupiables.Count - 1; i >= 0; --i) {
        auto* building = Owner->Occupiables[i];
        const int distance = entry_distance(this, building, true);
        if (distance < closest && building->CanBeOccupiedBy(entry_infantry(this))) {
            closest = distance;
            best = building;
        }
    }
    ShouldEnterOccupiable = best != nullptr;
    if (!best) return false;
    if (Destination != best || CurrentMission != Mission::Capture) {
        SetDestination(best, true);
        QueueMission(Mission::Capture, true);
    }
    return true;
}

bool FootClass::GarrisonStructure() {
    BuildingClass* best = nullptr;
    int closest = INT_MAX;
    for (int i = BuildingClass::Array.Count - 1; i >= 0; --i) {
        auto* building = BuildingClass::Array[i];
        if (!building->CanBeOccupiedBy(entry_infantry(this))) continue;
        const int distance = entry_distance(this, building);
        if (distance < closest) {
            closest = distance;
            best = building;
        }
    }
    ShouldGarrisonStructure = best != nullptr;
    if (!best) return false;
    if (Destination != best || CurrentMission != Mission::Capture) {
        SetDestination(best, true);
        QueueMission(Mission::Capture, true);
    }
    return true;
}
