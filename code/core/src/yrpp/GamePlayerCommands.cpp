#include "debug/debug_state.hpp"
// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 init.cpp UI commands, calibrated to YR 0x730AF0..0x730FE0.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
#include "yrpp/Unsorted.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/VocClass.h"
#include <new>

#if !defined(RA2_YRPP_GAME)
void Game::DeploySelected() noexcept {
    auto& selected = ObjectClass::CurrentObjects;
    if (selected.Count <= 0) return;
    auto* first = selected[0];
    bool done = false;
    if (first && first->WhatAmI() == AbstractType::Building &&
        static_cast<BuildingClass*>(first)->GetOccupantCount() > 0 &&
        game::debug_can_control_house(static_cast<BuildingClass*>(first)->Owner)) {
        first = selected[0];
        if (first && (first->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None)
            done = static_cast<TechnoClass*>(first)->ClickedEvent(EventType::Deploy);
    } else {
        const int count = selected.Count;
        bool deployed = false, undeployed = false;
        for (int i = 0; i < count; ++i) {
            auto* object = selected[i];
            if (!object || !game::debug_accept_player_order(object->GetOwningHouse())) continue;
            if (object->WhatAmI() == AbstractType::Infantry && static_cast<InfantryClass*>(object)->Type->Deployer) {
                const auto sequence = static_cast<InfantryClass*>(object)->SequenceAnim;
                if (sequence >= Sequence::Deploy && sequence <= Sequence::DeployedIdle)
                    deployed = true;
                else
                    undeployed = true;
            } else if (object->WhatAmI() == AbstractType::Unit) {
                const auto* unit = static_cast<UnitClass*>(object);
                if (unit->Type->IsSimpleDeployer) {
                    if (unit->Deployed || unit->Deploying)
                        deployed = true;
                    else
                        undeployed = true;
                }
            }
            if (deployed && undeployed) break;
        }
        for (int i = 0; i < count; ++i) {
            auto* object = selected[i];
            if (!object || !game::debug_accept_player_order(object->GetOwningHouse()) ||
                !(object->IsControllable() || object->IsActive()))
                continue;
            if ((object->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None) continue;
            auto* actor = static_cast<TechnoClass*>(object);
            if (!actor->vt_entry_310() || !actor->CanDeploySlashUnload() || actor->Berzerk) continue;
            auto* infantry = actor->WhatAmI() == AbstractType::Infantry ? static_cast<InfantryClass*>(actor) : nullptr;
            auto* unit = actor->WhatAmI() == AbstractType::Unit ? static_cast<UnitClass*>(actor) : nullptr;
            if (infantry && infantry->Type->Deployer && deployed && undeployed &&
                infantry->SequenceAnim >= Sequence::Deploy && infantry->SequenceAnim <= Sequence::DeployedIdle)
                continue;
            if (unit && unit->Type->IsSimpleDeployer && deployed && undeployed && (unit->Deployed || unit->Deploying))
                continue;
            if (actor->ClickedEvent(EventType::Deploy)) done = true;
        }
    }
    if (done) VocClass::PlayGlobal(RulesClass::Instance->DeploySound, 0x2000, 1.0f, nullptr);
}

void Game::StopSelected() noexcept {
    auto& selected = ObjectClass::CurrentObjects;
    const int count = selected.Count;
    if (count <= 0) return;
    for (int i = 0; i < count; ++i) {
        auto* object = selected[i];
        if (object && game::debug_accept_player_order(object->GetOwningHouse()) &&
            (object->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None &&
            (object->IsControllable() || object->IsActive()) &&
            !static_cast<TechnoClass*>(object)->ClickedEvent(EventType::Idle))
            return;
    }
    VocClass::PlayGlobal(RulesClass::Instance->StopSound, 0x2000, 1.0f, nullptr);
}

void Game::ReportNoGuardArea() noexcept {
    if (PlanningErrorReported) return;
    PlanningErrorReported = true;
    ShowMessage(StringTable::LoadString("MSG:PlanningModeNoGuardArea", nullptr, "D:\\ra2mdpost\\PlanMgr.cpp", 3233),
                480);
    VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000, 1.0f, nullptr);
}

void Game::GuardSelected() noexcept {
    if (PlanningNodeClass::PlanningModeActive) {
        ReportNoGuardArea();
        return;
    }
    auto& selected = ObjectClass::CurrentObjects;
    const int count = selected.Count;
    if (count <= 0) return;
    for (int i = 0; i < count; ++i) {
        auto* object = selected[i];
        if (!object || !game::debug_accept_player_order(object->GetOwningHouse()) ||
            (object->AbstractFlags & AbstractFlags::Techno) == AbstractFlags::None)
            continue;
        bool eligible = object->IsControllable() && object->IsActive() && object->WhatAmI() != AbstractType::Building;
        if (!eligible && object->WhatAmI() == AbstractType::Unit) {
            auto* unit = object->WhatAmI() == AbstractType::Unit ? static_cast<UnitClass*>(object) : nullptr;
            eligible = unit->SlaveManager != nullptr;
            if (!eligible) {
                unit = object->WhatAmI() == AbstractType::Unit ? static_cast<UnitClass*>(object) : nullptr;
                eligible = unit->Type->Harvester;
            }
        }
        if (eligible) {
            auto* actor = static_cast<TechnoClass*>(object);
            auto* cell = actor->GetCellAgain();
            if (!actor->ClickedMission(Mission::Area_Guard, cell, nullptr, nullptr)) return;
        }
    }
    VocClass::PlayGlobal(RulesClass::Instance->GuardSound, 0x2000, 1.0f, nullptr);
}

void Game::ScatterSelected() noexcept {
    auto& selected = ObjectClass::CurrentObjects;
    const int count = selected.Count;
    if (count <= 0) return;
    for (int i = 0; i < count; ++i) {
        auto* object = selected[i];
        if (object && game::debug_accept_player_order(object->GetOwningHouse()) &&
            (object->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None && object->IsControllable() &&
            !static_cast<TechnoClass*>(object)->ClickedEvent(EventType::Scatter))
            return;
    }
    VocClass::PlayGlobal(RulesClass::Instance->ScatterSound, 0x2000, 1.0f, nullptr);
}

bool Game::QueueOptions() noexcept {
    auto* player = HouseClass::CurrentPlayer;
    if (!player || player->IsWinner || player->IsLoser || player->IsGameOver) return false;
    EventClass event;
    ::new (&event) EventClass(player->ArrayIndex, EventType::Options);
    return EventClass::OutList.Add(event);
}
#endif
