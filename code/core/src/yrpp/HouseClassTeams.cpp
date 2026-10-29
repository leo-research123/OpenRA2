// YR house team admission (0x005095D0/0x00509610) and hunt order (0x00501400).
#include "yrpp/HouseClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/RulesClass.h"

int HouseClass::CountTeams(TeamTypeClass* type) const {
    int count = 0;
    for (int i = TeamClass::Array.Count - 1; i >= 0; --i) {
        auto* team = TeamClass::Array[i];
        if (team && team->Owner == this && team->Type == type) ++count;
    }
    return count;
}
bool HouseClass::CanBuildTeam(TeamTypeClass* team) const {
    if (!team || !team->TaskForce) return false;
    for (int i = 0; i < team->TaskForce->CountEntries; ++i) {
        auto* type = team->TaskForce->Entries[i].Type;
        if (type && type->FindFactory(true, true, false, this)) continue;
        if (SessionClass::Instance.GameMode != GameMode::Campaign) return false;
        bool found = false;
        for (int j = FootClass::Array.Count - 1; j >= 0; --j) {
            auto* foot = FootClass::Array[j];
            if (foot->Owner == this && foot->GetTechnoType() == type &&
                team->CanRecruitForAI(foot, const_cast<HouseClass*>(this))) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}
HRESULT YRPP_STDCALL HouseClass::Fire_Sale() const {
    if (OwnedBuildings <= 0) return 1;
    const int count = Buildings.Count;
    for (int i = 0; i < count; ++i) {
        auto* building = Buildings[i];
        if (building && !building->InLimbo && building->Health > 0) building->Sell(1);
    }
    return 0;
}
HRESULT YRPP_STDCALL HouseClass::All_To_Hunt() {
    for (int i = TechnoClass::Array.Count - 1; i >= 0; --i) {
        auto* techno = TechnoClass::Array[i];
        if (techno->Owner != this || !techno->IsOnMap || techno->InLimbo) continue;
        if (techno->MindControlledByAUnit && techno->GetTechnoType()->Insignificant && !IsHumanPlayer) {
            int damage = techno->GetTechnoType()->Strength;
            techno->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, true, nullptr);
        } else if ((unsigned(techno->AbstractFlags) & 4u) != 0) {
            auto* foot = static_cast<FootClass*>(techno);
            if (foot->Team) foot->Team->LiberateMember(foot, -1, 0);
            foot->QueueMission(Mission::Hunt, false);
        } else if (techno->WhatAmI() == AbstractType::Building && techno->GetOccupantCount() > 0)
            static_cast<BuildingClass*>(techno)->UnloadOccupants(true, false);
    }
    AllToHunt = true;
    return 0;
}
