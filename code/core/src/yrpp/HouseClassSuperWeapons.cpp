#include "yrpp/SessionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MapClass.h"
#include "x87_integer.hpp"
#include <bit>

bool HouseClass::Fire_SW(int index, const CellStruct& cell) {
    auto* super = Supers[index];
    SuperClass* dependent = nullptr;
    if (super->Type->PostClick) {
        dependent = Supers[super->Type->PreDependent];
        if (dependent) super->ChronoMapCoords = dependent->ChronoMapCoords;
    }
    super->ClickFire(this == CurrentPlayer, cell);
    if (super->Type->PostClick && dependent) {
        dependent->SetReadiness(false);
        dependent->StopPreclickAnim(this == CurrentPlayer);
    }
    for (int i = Array.Count - 1; i >= 0; --i)
        if (Array[i]) Array[i]->NotifySuperWeaponFired(super, cell);
    return true;
}

void HouseClass::NotifySuperWeaponFired(SuperClass* super, const CellStruct& cell) {
    if (Type->MultiplayPassive || IsHumanPlayer || !super || !super->Type->AIDefendAgainst) return;
    auto* target = MapClass::Instance.GetCellAt(cell);
    CoordStruct origin;
    GetBaseCenterCoords(&origin);
    const auto at = target->GetCoords();
    const auto delta = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
    const double x = delta(origin.X, at.X), y = delta(origin.Y, at.Y), z = delta(origin.Z, at.Z);
    if (game::x87_integer(Math::sqrt(x * x + y * y + z * z)) > RulesClass::Instance->AISuperDefenseDistance) return;
    const int probability = RulesClass::Instance->AISuperDefenseProbability[int(AIDifficulty)];
    if (ScenarioClass::Instance->Random.RandomRanged(0, 99) > probability) return;
    if (ConYards.Count > 0) {
        const auto center = ConYards[0]->GetCoords();
        PreferredDefensiveCell = {short(center.X / 256), short(center.Y / 256)};
    } else
        PreferredDefensiveCell = BaseCenter == CellStruct::Empty ? BaseSpawnCell : BaseCenter;
    PreferredDefensiveCellStartTime = Unsorted::CurrentFrame;
}

void HouseClass::UpdateSuperWeapons() {
    if (!Game::IsActive) return;
    const bool player = this == CurrentPlayer;
    for (int i = 0; i < Supers.Count; ++i) {
        auto* super = Supers[i];
        if (!super->IsPresent || ((!super->CanHold || super->IsOneTime) && !Defeated)) continue;
        bool powered = false, present = false;
        if (!Defeated)
            for (int j = 0; j < BuildingClass::Array.Count; ++j) {
                auto* building = BuildingClass::Array[j];
                if (building->InLimbo || !building->IsAlive || building->Owner->Fetch_ID() != Fetch_ID()) continue;
                for (auto* upgrade : building->Upgrades)
                    if (upgrade && (upgrade->SuperWeapon == i || upgrade->SuperWeapon2 == i)) {
                        present = true;
                        if (!powered) powered = building->HasPower;
                    }
                if (building->FirstActiveSWIdx() == i || building->SecondActiveSWIdx() == i) {
                    present = true;
                    if (powered) break;
                    powered = building->HasPower;
                }
                if (powered && present) break;
            }
        const auto& session = SessionClass::Instance;
        if (super->Type->DisableableFromShell && !session.Config.SWAllowed) present = false;
        if (GetPowerPercentage() < 1.0) powered = false;
        const int tab = SidebarClass::GetObjectTabIdx(AbstractType::SuperWeaponType, super->Type->GetArrayIndex(), 0);
        bool changed = false;
        if (!present || Defeated) {
            changed = super->Lose();
            if (!CurrentPlayer) continue;
        } else if (!powered && super->IsPowered())
            changed = super->SetOnHold(true);
        else if (powered)
            changed = super->SetOnHold(false);
        if (!changed) continue;
        if (player) {
            if (DisplayClass::Instance.CurrentSWTypeIndex == i) DisplayClass::Instance.CurrentSWTypeIndex = -1;
            SidebarClass::Instance.RepaintSidebar(tab);
        }
        RecheckTechTree = true;
    }
}

// 0x0050B1D0: grant from owned buildings after the loss/power pass.
bool HouseClass::AISupers() {
    if (Defeated) return false;
    bool changed = false;
    for (int i = 0; i < Supers.Count; ++i) {
        auto* super = Supers[i];
        if (super->IsPresent && !super->IsOneTime) continue;
        bool present = false;
        for (int j = Buildings.Count - 1; j >= 0; --j) {
            auto* building = Buildings[j];
            if (!building->IsAlive || building->InLimbo) continue;
            for (auto* upgrade : building->Upgrades)
                if (upgrade && (upgrade->SuperWeapon == i || upgrade->SuperWeapon2 == i)) present = true;
            if (building->FirstActiveSWIdx() == i || building->SecondActiveSWIdx() == i) present = true;
            if (present) break;
        }
        const auto& session = SessionClass::Instance;
        if (super->Type->DisableableFromShell && !session.Config.SWAllowed) present = false;
        if (!present) continue;
        changed = super->Grant(false, this == CurrentPlayer, GetPowerPercentage() < 1.0) || changed;
        if (this == CurrentPlayer) {
            SidebarClass::Instance.AddCameo(AbstractType::Special, i);
            SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(AbstractType::Special, i, 0));
        }
    }
    return changed;
}
