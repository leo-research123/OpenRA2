// HouseClass alliance changes: YR 0x00501540 CanAlly, 0x00509400
// AdjustThreats, 0x004F9B70 MakeAlly, 0x004F9F90 MakeEnemy and 0x00501640
// AllyAIHouses.
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/TargetClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/VoxClass.h"
#include "scenario_runtime.hpp"
#include "x87_integer.hpp"
#include <algorithm>
#include <bit>
#include <iterator>

namespace {
unsigned house_bit(int index) { return 1u << (static_cast<unsigned>(index) & 31u); }
// All control checks read the same live original session.
GameMode game_mode() { return SessionClass::Instance.GameMode; }
bool campaign() { return game_mode() == GameMode::Campaign; }
// The inline control test of these functions: a human player, or in a
// campaign also a house under player control.
bool player_side(const HouseClass& house) { return house.IsHumanPlayer || (campaign() && house.IsInPlayerControl); }
// The original formats the CSF text with the CRT's wide swprintf, where %s
// takes a wide string. Each %s takes the next name; the buffer holds 80
// characters, which the original does not bound.
void format_names(wchar_t (&buffer)[80], const wchar_t* format, const wchar_t* first, const wchar_t* second) {
    std::size_t out = 0;
    const wchar_t* names[]{first, second};
    int next = 0;
    const auto put = [&](wchar_t c) {
        if (out + 1 < std::size(buffer)) buffer[out++] = c;
    };
    for (const wchar_t* p = format ? format : L""; *p; ++p) {
        if (p[0] == L'%' && p[1] == L's' && next < 2) {
            for (const wchar_t* q = names[next++]; q && *q; ++q)
                put(*q);
            ++p;
        } else if (p[0] == L'%' && p[1] == L'%') {
            put(L'%');
            ++p;
        } else {
            put(*p);
        }
    }
    buffer[out] = 0;
}
void add_message(const HouseClass& house, const char* label, int line, const HouseClass& other) {
    wchar_t buffer[80];
    format_names(buffer, game::scenario_text(label, line), house.UIName, other.UIName);
    const int timeout = game::x87_integer(RulesClass::Instance->MessageDelay * 900.0);
    MessageListClass::Instance.AddMessage(nullptr, 0, buffer, house.Type->ColorSchemeIndex,
                                          static_cast<TextPrintType>(0x4046), timeout, false);
}
TechnoClass* as_techno(AbstractClass* item) {
    switch (item->WhatAmI()) {
        case AbstractType::Aircraft:
        case AbstractType::Unit:
        case AbstractType::Building:
        case AbstractType::Infantry:
            return static_cast<TechnoClass*>(item);
        default:
            return nullptr;
    }
}
}

bool HouseClass::CanAlly(HouseClass* other) const {
    if (other) {
        if (other == this) return false;
        const int index = other->ArrayIndex;
        if (index == ArrayIndex || (index != -1 && (Allies.data & house_bit(index)))) return false;
    }
    // A null house is dereferenced here, as in the original.
    if (other->Type->SideIndex == SideClass::FindIndex("Civilian") && !campaign()) return false;
    if (Unsorted::ScenarioInit) return true;
    if (Defeated) return false;
    int active = 0, allied = 0;
    for (auto* house : Array) {
        if (house->Defeated || house->Type->MultiplayPassive) continue;
        ++active;
        if (house == this || house->ArrayIndex == ArrayIndex ||
            (house->ArrayIndex != -1 && (Allies.data & house_bit(house->ArrayIndex))))
            ++allied;
    }
    return active != allied + 1;
}

void HouseClass::AdjustThreats() {
    // Each threat is added to its region and the eight around it, halved
    // once on the sides and twice on the corners; a negative sum becomes 0.
    static constexpr int shifts[9]{2, 1, 2, 1, 0, 1, 2, 1, 2};
    static constexpr int offsets[9]{-131, -130, -129, -1, 0, 1, 129, 130, 131};
    auto* threats = &ThreatPosedEstimates[0][0];
    std::fill_n(threats, 130 * 130, 0u);
    const auto add = [&](int region, int value) {
        for (int k = 0; k < 9; ++k) {
            auto& slot = threats[region + offsets[k]];
            const unsigned sum = slot + static_cast<unsigned>(value >> shifts[k]);
            slot = std::bit_cast<int>(sum) < 0 ? 0u : sum;
        }
    };
    for (int i = TechnoClass::Array.Count - 1; i >= 0; --i) {
        auto* techno = TechnoClass::Array[i];
        if (techno->InLimbo || !techno->IsAlive) continue;
        const int value = techno->GetThreatValue();
        if (value <= 0) continue;
        const auto kind = techno->WhatAmI();
        if (kind == AbstractType::Infantry || kind == AbstractType::Unit || kind == AbstractType::Aircraft) {
            const auto& last = static_cast<FootClass*>(techno)->LastMapCoords;
            // 0x00A8EF98 holds the packed (-1,-1) cell.
            if (last.X == -1 && last.Y == -1) continue;
            const int region = MapClass::CellRegion(last);
            auto* owner = techno->Owner;
            if (owner == this) continue;
            if (player_side(*this) && owner &&
                (owner->ArrayIndex == ArrayIndex ||
                 (owner->ArrayIndex != -1 && (Allies.data & house_bit(owner->ArrayIndex)))))
                continue;
            add(region, value);
        } else {
            add(MapClass::CellRegion(techno->GetCell()->MapCoords), value);
        }
    }
}

void HouseClass::MakeAlly(HouseClass* ally, bool announce) {
    if (!CanAlly(ally)) return;
    DisplayClass::Instance.WithdrawSharedSight(false, 0);
    Allies.data |= house_bit(ally->ArrayIndex);
    AdjustThreats();
    for (int i = 0; i < AngerNodes.Count; ++i) {
        if (AngerNodes[i].House != ally) continue;
        RegisterDamage(-AngerNodes[i].AngerLevel, ally);
        break;
    }
    if (EnemyHouseIndex == ally->ArrayIndex) EnemyHouseIndex = -1;
    if (Unsorted::ScenarioInit) AltAllies.data |= house_bit(ally->ArrayIndex);
    if (game_mode() == GameMode::Internet || !Unsorted::ScenarioInit) {
        if (player_side(*this) && RulesClass::Instance->Paranoid && !ally->Type->MultiplayPassive && !campaign())
            AllyAIHouses();
        // Objects of this house stop targeting the new ally and its allies.
        for (int i = 0; i < LogicClass::Instance.Count; ++i) {
            auto* item = LogicClass::Instance[i];
            if (!item) continue;
            auto* techno = as_techno(item);
            if (!techno || techno->InLimbo || techno->GetOwningHouseIndex() != ArrayIndex) continue;
            TargetClass target(techno->Target);
            if (!target.m_RTTI || !target.As_Techno()) continue;
            auto* owner = target.As_Techno()->GetOwningHouse();
            if (owner && IsAlliedWith(owner)) techno->SetTarget(nullptr);
        }
        if (player_side(*this) && !campaign() && !ally->Type->MultiplayPassive) {
            add_message(*this, "TXT_HAS_ALLIED", 3357, *ally);
            if (announce) {
                auto* player = HouseClass::CurrentPlayer;
                if (ally->IsAlliedWith(this)) {
                    if (!Unsorted::MuteSWLaunches) {
                        VoxClass::Play(player->IsAlliedWith(ally) && player->IsAlliedWith(this)
                                           ? "EVA_AllianceFormed"
                                           : "EVA_EnemyAllianceFormed");
                    }
                } else if (!campaign() ? this == player : (IsHumanPlayer || IsInPlayerControl)) {
                    VoxClass::Play("EVA_RequestingAlliance");
                } else if (!campaign() ? ally == player : (ally->IsHumanPlayer || ally->IsInPlayerControl)) {
                    VoxClass::Play("EVA_AllianceRequested");
                }
            }
        }
        MapClass::Instance.GScreenClass::MarkNeedsRedraw(0);
    }
    DisplayClass::Instance.RestoreSharedSight(false, 0);
}

void HouseClass::MakeEnemy(HouseClass* enemy, bool announce) {
    if (!campaign() && (enemy->Type->MultiplayPassive || Type->MultiplayPassive)) return;
    DisplayClass::Instance.WithdrawSharedSight(false, 0);
    RegisterDamage(1, enemy);
    if (enemy && (enemy == this || enemy->ArrayIndex == ArrayIndex ||
                  (enemy->ArrayIndex != -1 && (Allies.data & house_bit(enemy->ArrayIndex))))) {
        Allies.data &= ~house_bit(enemy->ArrayIndex);
        // The original keeps only the comparison of the bit with zero here,
        // which clears every alternate ally.
        if (Unsorted::ScenarioInit) AltAllies.data &= house_bit(enemy->ArrayIndex) == 0 ? 1u : 0u;
        AdjustThreats();
        bool broken = false;
        if (enemy->IsAlliedWith(this)) {
            broken = true;
            enemy->Allies.data &= ~house_bit(ArrayIndex);
            // The original clears this house's own alternate bit here.
            if (Unsorted::ScenarioInit) AltAllies.data &= ~house_bit(ArrayIndex);
            enemy->AdjustThreats();
            enemy->RegisterDamage(1, enemy);
        }
        if (!campaign() && !Unsorted::ScenarioInit && IsHumanPlayer) {
            add_message(*this, "TXT_AT_WAR", 3480, *enemy);
            MapClass::Instance.GScreenClass::MarkNeedsRedraw(0);
            auto* player = HouseClass::CurrentPlayer;
            if (announce && broken && (this == player || enemy == player)) VoxClass::Play("EVA_AllianceBroken");
        }
    }
    DisplayClass::Instance.RestoreSharedSight(false, 0);
}

void HouseClass::AllyAIHouses() {
    auto* mode = SessionClass::Instance.MPGameMode;
    if (!((mode && mode->vt_entry_04()) || RulesClass::Instance->AlliesAllowed) ||
        ScenarioClass::Instance->TeamsPresent)
        return;
    for (int i = 0; i < Array.Count; ++i) {
        auto* house = Array[i];
        if (!house || house->Defeated || player_side(*house)) continue;
        house->IsParanoid = true;
        for (int j = 0; j < Array.Count; ++j) {
            auto* other = Array[j];
            if (!other || other->Defeated) continue;
            if (player_side(*other))
                house->MakeEnemy(other, false);
            else
                house->MakeAlly(other, false);
        }
    }
}
