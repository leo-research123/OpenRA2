// Original AI team selection, YR 0x006F0AB0 and 0x006F1E20.
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstdlib>
#include <vector>

bool TeamTypeClass::CanRecruitForAI(FootClass* unit, HouseClass* owner) const {
    if ((unit->Team && unit->Team->Type == this) || unit->Health <= 0 || !unit->IsAlive || unit->InLimbo ||
        unit->HasAnyLink() || unit->Owner != owner)
        return false;
    const auto mission = unit->GetCurrentMission();
    if (mission != Mission::None && !MissionClass::IsRecruitableMission(mission)) return false;
    if ((!unit->RecruitableA && !Autocreate) || (!unit->RecruitableB && Autocreate)) return false;
    if (unit->Team && unit->Team->Type->Priority >= Priority) return false;
    return unit->WhatAmI() != AbstractType::Aircraft || !unit->GetWeapon(0)->WeaponType || unit->Ammo != 0;
}
TypeList<TeamTypeClass*> YRPP_FASTCALL TeamTypeClass::SuggestNew(HouseClass* owner, bool) noexcept {
    try {
        TypeList<TeamTypeClass*> result;
        auto* target =
            owner->EnemyHouseIndex == -1 ? nullptr : HouseClass::Array.GetItemOrDefault(owner->EnemyHouseIndex);
        auto& random = ScenarioClass::Instance->Random;
        auto& rules = *RulesClass::Instance;
        const int difficulty = int(owner->AIDifficulty);
        const auto setting = [difficulty](const TypeList<int>& list) {
            return difficulty >= 0 && difficulty < list.Count ? list[difficulty] : 0;
        };
        if (random.RandomRanged(1, 100) <= owner->RatioAITriggerTeam && owner->AITriggersActive) {
            int count = 0, defense = 0;
            for (auto* team : TeamClass::Array)
                if (team->Owner == owner) {
                    ++count;
                    if (team->Type->IsBaseDefense) ++defense;
                }
            const int cap = setting(rules.TotalAITeamCap);
            bool enough = false;
            if (count < cap || defense < count / 2)
                enough = defense > setting(rules.MaximumAIDefensiveTeams);
            else {
                TeamClass* oldest = nullptr;
                int frame = 0x7FFFFFFF;
                for (auto* team : TeamClass::Array)
                    if (team->Owner == owner && team->Type->IsBaseDefense && team->CreationFrame < frame) {
                        oldest = team;
                        frame = team->CreationFrame;
                    }
                if (oldest) {
                    --count;
                    enough = true;
                    GameDelete(oldest);
                }
            }
            if (count < cap) {
                struct Choice {
                    AITriggerTypeClass* trigger;
                    int weight;
                };
                std::vector<Choice> choices;
                unsigned total = 0;
                bool exclusive = false;
                for (auto* trigger : AITriggerTypeClass::Array) {
                    if (!trigger || !trigger->ConditionMet(owner, target, enough)) continue;
                    const int weight = game::x87_integer(trigger->Weight_Current);
                    if (weight == 5000) {
                        if (!exclusive) {
                            exclusive = true;
                            choices.clear();
                            total = 0;
                        }
                    } else if (exclusive)
                        continue;
                    choices.push_back({trigger, weight});
                    total += unsigned(weight);
                }
                if (total && !choices.empty()) {
                    const unsigned pick = unsigned(random.RandomRanged(1, std::bit_cast<int>(total)));
                    unsigned accumulated = 0;
                    for (const auto& choice : choices) {
                        accumulated += unsigned(choice.weight);
                        if (accumulated < pick) continue;
                        if (choice.trigger->Team1) result.AddItem(choice.trigger->Team1);
                        if (choice.trigger->Team2) result.AddItem(choice.trigger->Team2);
                        break;
                    }
                }
            }
        }
        for (auto* team : TeamClass::Array)
            if (team->Owner == owner && (team->IsForcedActive || !team->IsReforming) &&
                result.FindItemIndex(team->Type) >= 0) {
                result.Clear();
                return result;
            }
        for (auto* type : result)
            type->Autocreate = true;
        return result;
    } catch (...) {
        std::abort();
    }
}
