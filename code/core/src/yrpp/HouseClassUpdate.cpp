// House AI phases recovered from YR 0x004F8440. Strategic AI remains separate.
#include "yrpp/HouseClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/SessionClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MPGameModeClass.h"
#include "x87_integer.hpp"
#include "type_resources.hpp"
#include "game_loop.hpp"
#include <algorithm>
#include <bit>

namespace {
void start_result_timer(CDTimerClass& timer) {
    timer.Start(game::x87_integer(RulesClass::Instance->SavourDelay * 900.0));
    const auto mode = SessionClass::Instance.GameMode;
    if (mode != GameMode::Campaign && mode != GameMode::Skirmish) {
        // 0x4FCA6E..0x4FCAE7: network results share an absolute 10-frame boundary.
        const int left = std::max(timer.GetTimeLeft(), SessionClass::Instance.MaxAhead);
        const int end =
            std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame) + static_cast<unsigned>(left) + 9u);
        timer.Start(
            std::bit_cast<int>(static_cast<unsigned>(end / 10) * 10u - static_cast<unsigned>(Unsorted::CurrentFrame)));
    }
}
}

BYTE HouseClass::Win(bool immediate) {
    if (!IsWinner && !IsGameOver && !IsLoser) {
        IsWinner = true;
        if (!immediate) start_result_timer(BorrowedTime);
        if (SessionClass::Instance.GameMode == GameMode::Campaign && (IsHumanPlayer || IsInPlayerControl)) {
            ScenarioClass::LockInput();
            VoxClass::Play("EVA_MissionAccomplished");
        } else if (SessionClass::Instance.GameMode != GameMode::Campaign && this == CurrentPlayer &&
                   !game::type_resources().audio_unavailable) {
            VoxClass::Play("EVA_YouAreVictorious");
        }
    }
    return IsWinner;
}
BYTE HouseClass::Lose(bool immediate) {
    IsWinner = false;
    if (!IsGameOver && !IsLoser) {
        IsLoser = true;
        if (!immediate) start_result_timer(BorrowedTime);
        if (SessionClass::Instance.GameMode == GameMode::Campaign && (IsHumanPlayer || IsInPlayerControl)) {
            ScenarioClass::LockInput();
            VoxClass::Play("EVA_MissionFailed");
        } else if (SessionClass::Instance.GameMode != GameMode::Campaign && this == CurrentPlayer && this != Observer &&
                   !game::type_resources().audio_unavailable) {
            VoxClass::Play("EVA_YouHaveLost");
        }
    }
    return IsLoser;
}
void HouseClass::Update() {
    auto* loop = game::current_game_loop();
    if (loop && loop->state.result_house == this) {
        if (loop->state.result_phase == game::HouseResultPhase::victory) goto victory_speech;
        if (loop->state.result_phase == game::HouseResultPhase::defeat) goto defeat_speech;
    }
    if (PowerBlackoutTimer.GetTimeLeft() == 1) {
        PowerBlackoutTimer.Start(0);
        RecheckPower = true;
    }
    if (RadarBlackoutTimer.GetTimeLeft() == 1) {
        RadarBlackoutTimer.Start(0);
        RecheckRadar = true;
    }
    // 0x4F84E5 updates accounting before charging, then periodic tag polling.
    if (RecheckPower) {
        UpdatePower();
        RecheckRadar = true;
    }
    if (RecheckRadar) {
        if (!Unsorted::MuteSWLaunches) UpdateRadarAvailability();
        UpdateSpySat();
    }
    // 0x4F851C decays every existing relation, including allied houses.
    if (Unsorted::CurrentFrame % 100 == 0)
        for (int i = 0; i < AngerNodes.Count; ++i)
            if (AngerNodes[i].AngerLevel > 1) --AngerNodes[i].AngerLevel;
    if (!IsControlledByHuman() && (AutoBaseBuilding || IQLevel2 >= RulesClass::Instance->Production)) {
        AutoBaseBuilding = true;
        Production = true;
        AutocreateAllowed = true;
    }
    // 0x4F85CB..0x4F87B9: speech settles before the result is published.
    // Resume at the exact barrier without repeating the accounting above it.
    if (IsWinner && !BorrowedTime.GetTimeLeft() &&
        (SessionClass::Instance.GameMode != GameMode::Campaign || AllowWinBlocks <= 0)) {
    victory_speech:
        if (!game::wait_for_house_result(*this, game::HouseResultPhase::victory)) return;
        IsWinner = false;
        const auto mode = SessionClass::Instance.GameMode;
        auto* game_mode = SessionClass::Instance.MPGameMode;
        bool player_wins = mode == GameMode::Campaign ? IsControlledByHuman() : this == CurrentPlayer;
        if (!player_wins && (mode == GameMode::LAN || mode == GameMode::Internet) && game_mode &&
            game_mode->vt_entry_04() && game_mode->vt_entry_08() && IsControlledByHuman())
            player_wins = true;
        if (player_wins)
            Game::PlayerWins = true;
        else
            Game::PlayerLoses = true;
    }
    if (IsLoser && !BorrowedTime.GetTimeLeft()) {
    defeat_speech:
        if (!game::wait_for_house_result(*this, game::HouseResultPhase::defeat)) return;
        if (SessionClass::Instance.GameMode == GameMode::Campaign ? IsControlledByHuman() : this == CurrentPlayer)
            Game::PlayerLoses = true;
        else
            Game::PlayerWins = true;
    }
    // 0x004F8A6A selects AI teams when the original per-house timer expires.
    if (!TeamDelayTimer.GetTimeLeft() && !IsControlledByHuman() && !Type->MultiplayPassive) {
        const auto teams = TeamTypeClass::SuggestNew(this, false);
        for (auto* type : teams)
            type->CreateTeam(this);
        TeamDelayTimer.Start(TeamDelayFor(int(AIDifficulty)));
    }
    // This is the construction-notification timer, not the strategic AI timer.
    if (!TriggerDelayTimer.GetTimeLeft() || IsBuiltSomething) {
        TriggerDelayTimer.Start(90);
        IsBuiltSomething = false;
    }
    // 0x4F8E32 follows team creation; a team callback may change the super list.
    for (int i = 0; i < Supers.Count; ++i)
        if (Supers[i]->HasChargeProgressed(this == CurrentPlayer) && this == CurrentPlayer)
            SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(AbstractType::Special, i, 0));
    // 0x4F8E92..0x4F8F87: ShortGame keeps the three base-unit types alive;
    // normal games count active mobile types and the third refinery exception.
    if (SessionClass::Instance.GameMode != GameMode::Campaign && !Defeated && Unsorted::CurrentFrame > 0 &&
        !Type->MultiplayPassive) {
        auto& rules = *RulesClass::Instance;
        bool eliminated = false;
        if (SessionClass::Instance.Config.ShortGame) {
            int bases = 0;
            for (int i : {1, 2, 0})
                if (i < rules.BaseUnit.Count && rules.BaseUnit[i])
                    bases += OwnedUnitTypes.GetItemCount(rules.BaseUnit[i]->ArrayIndex);
            eliminated = OwnedBuildings <= 0 && bases <= 0;
        } else {
            int remaining = OwnedBuildings + ActiveUnitTypes.GetTotal() + ActiveInfantryTypes.GetTotal() +
                            ActiveAircraftTypes.GetTotal();
            if (rules.BuildRefinery.Count > 2 && rules.BuildRefinery[2])
                remaining += ActiveBuildingTypes.GetItemCount(rules.BuildRefinery[2]->ArrayIndex);
            eliminated = remaining == 0;
        }
        if (eliminated) {
            DestroyAll();
            AcceptDefeat();
        }
    }
    // 0x4F8F87..0x4F8FBE uses the live array in reverse. Expiration callbacks
    // can compact RelatedTags; a snapshot would change the original order.
    if (Unsorted::CurrentFrame % 8 == 0)
        for (int i = RelatedTags.Count - 1; i >= 0; --i)
            RelatedTags[i]->RaiseEvent(TriggerEvent::AnyEvent, nullptr, CellStruct::Empty, false, nullptr);
    // 0x4F9017 restarts AITimer with the strategic entry's returned interval.
    if (!AITimer.GetTimeLeft() && !IsControlledByHuman() && !Type->MultiplayPassive) {
        AITimer.Start(AI_StrategicUpdate());
    }
    if (!IsControlledByHuman() && !Type->MultiplayPassive && Unsorted::CurrentFrame % 8 == 0) {
        const auto mobile = [&] {
            AI_VehicleConstructionUpdate();
            AI_InfantryConstructionUpdate();
            AI_AircraftConstructionUpdate();
        };
        if (SessionClass::Instance.GameMode == GameMode::Campaign || AIState_1E4 == 0) {
            AI_BaseConstructionUpdate();
            mobile();
        } else if (AIState_1E4 == 1) {
            AI_BaseConstructionUpdate();
            if (ProducingBuildingTypeIndex == -1 ||
                !BuildingTypeClass::Array[ProducingBuildingTypeIndex]->FindFactory(true, true, true, this))
                mobile();
        } else if (AIState_1E4 == 2) {
            AI_VehicleConstructionUpdate();
            const auto harvester = [&]() -> UnitTypeClass* {
                const unsigned owners = 1u << (unsigned(Type->FindParentCountryIndex()) & 31u);
                for (auto* type : RulesClass::Instance->HarvesterUnit)
                    if (type->OwnerFlags & owners) return type;
                return nullptr;
            };
            // The original repeats the country lookup when a type was found.
            const auto* miner = harvester() ? harvester() : nullptr;
            if (!miner || ProducingUnitTypeIndex != miner->ArrayIndex) {
                AI_InfantryConstructionUpdate();
                AI_AircraftConstructionUpdate();
            }
            bool needsBuilding =
                ProducingUnitTypeIndex == -1 && ProducingInfantryTypeIndex == -1 && ProducingAircraftTypeIndex == -1;
            if (ProducingUnitTypeIndex != -1 &&
                !UnitTypeClass::Array[ProducingUnitTypeIndex]->FindFactory(true, true, true, this))
                needsBuilding = true;
            if (ProducingInfantryTypeIndex != -1 &&
                !InfantryTypeClass::Array[ProducingInfantryTypeIndex]->FindFactory(true, true, true, this))
                needsBuilding = true;
            if (ProducingAircraftTypeIndex != -1 &&
                !AircraftTypeClass::Array[ProducingAircraftTypeIndex]->FindFactory(true, true, true, this))
                needsBuilding = true;
            if (needsBuilding) AI_BaseConstructionUpdate();
        }
    }
    UpdateTechTree();
    // Release the shared gate so another building can start automatic repair.
    if (Repairing && !RepairTimer.GetTimeLeft()) Repairing = false;
}
