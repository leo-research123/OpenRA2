// Base-defense response, YR 0x708080.
#include "yrpp/TechnoClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/RulesClass.h"
#include <algorithm>
#include <bit>
void TechnoClass::BaseIsAttacked(TechnoClass* enemy) {
    if (!enemy) return;
    int desired = std::bit_cast<int>(unsigned(enemy->GetThreatValue()) *
                                     unsigned(RulesClass::Instance->ComputerBaseDefenseResponse));
    if (Owner->IsAlliedWith(enemy) || Owner->IsControlledByHuman() || enemy->InLimbo) return;
    const bool campaign = SessionClass::Instance.GameMode == GameMode::Campaign;
    if (campaign && IsArmed()) return;
    if (enemy->WhatAmI() != AbstractType::Infantry && enemy->WhatAmI() != AbstractType::Unit) return;
    if (GetTechnoType()->Insignificant) return;
    auto* enemyFoot =
        (enemy->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None ? static_cast<FootClass*>(enemy) : nullptr;
    if (enemyFoot && enemyFoot->unknown_timer_650.GetTimeLeft()) return;
    TeamClass::SuspendTeams(RulesClass::Instance->SuspendPriority, Owner);
    FootClass* defenders[6]{};
    int values[6]{}, count = 0, weakest = 0;
    const auto collect = [&](FootClass* foot, bool unit) {
        if (!foot || !foot->IsAlive || foot->GetOwningHouseIndex() != GetOwningHouseIndex()) return;
        if ((foot->Team && !foot->Team->Type->IsBaseDefense) || !foot->RecruitableA || !foot->IsArmed() ||
            !foot->RecruitableB || (!foot->CurrentMissionControl()->Recruitable && campaign) ||
            foot->GetFireErrorWithoutRange(enemy, 0) == FireError::ILLEGAL)
            return;
        if (unit && (static_cast<UnitClass*>(foot)->Type->ResourceGatherer || foot->BunkerLinkedItem)) return;
        const auto to = GetDestination();
        const auto from = foot->GetDestination();
        if (!MapClass::IsSameCellZone({short(from.X / 256), short(from.Y / 256)},
                                      {short(to.X / 256), short(to.Y / 256)}, foot->GetTechnoType()->MovementZone,
                                      foot->IsOnBridge(), false, false))
            return;
        int value = foot->RescueMission(enemy);
        if (!value || (unit && SlaveOwner)) return;
        if (ArchiveTarget == this && (!unit || value > 0))
            value = std::bit_cast<int>(unsigned(value) * (unit ? 10u : 100u));
        if (value < 0) {
            desired = std::bit_cast<int>(unsigned(desired) + unsigned(value));
            return;
        }
        if (count < 6) {
            defenders[count] = foot;
            values[count++] = value;
            return;
        }
        if (value <= weakest) return;
        int next = value;
        for (int i = 0; i < count; ++i) {
            if (values[i] == weakest) {
                values[i] = value;
                defenders[i] = foot;
            } else if (values[i] < next)
                next = values[i];
        }
        weakest = next;
    };
    for (int i = 0; i < InfantryClass::Array.Count && desired > 0; ++i)
        collect(InfantryClass::Array[i], false);
    for (int i = 0; i < UnitClass::Array.Count && desired > 0; ++i)
        collect(UnitClass::Array[i], true);
    int total = 0;
    if (desired > 0) {
        for (int i = 0; i < count - 1; ++i)
            for (int j = i + 1; j < count; ++j)
                if (values[i] < values[j]) {
                    std::swap(values[i], values[j]);
                    std::swap(defenders[i], defenders[j]);
                }
        for (int i = 0; i < count; ++i) {
            auto* foot = defenders[i];
            const bool baseDefense = foot->Team && foot->Team->Type->IsBaseDefense;
            const int roll = ScenarioClass::Instance->Random.RandomRanged(0, 99);
            foot->QueueMission(roll >= 66 || baseDefense ? Mission::Area_Guard : Mission::Rescue, false);
            foot->ArchiveTarget = this;
            foot->SetTarget(enemy);
            total = std::bit_cast<int>(unsigned(total) + unsigned(foot->GetThreatValue()));
            if (total > desired) break;
        }
    }
    if (total > desired && enemyFoot)
        enemyFoot->unknown_timer_650.Start(static_cast<int>(RulesClass::Instance->BaseDefenseDelay * 900.0));
}
