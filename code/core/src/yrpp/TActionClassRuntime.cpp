// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 taction.cpp operator(); YR 0x6DD8B0/0x6E2AF0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/TActionClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/WWMouseClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/SuperClass.h"
#include "sidebar_movie.hpp"

// OpenTS TAction_CHANGE_HOUSE, with YR 0x006E0AA0's trigger requirement,
// house selectors and silent transfer. The source is the entering engineer;
// the recipients are all live, placed technos whose tag contains this trigger.
// ALL07's allied Soviet base relies on this action before engineer repair.
bool TActionClass::SwitchAttachedObjectsToHouse(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) {
    if (!trigger || Value == -1) return false;
    auto* house = Value == 8997                   ? trigger->GetHouse()
                  : HouseClass::Index_IsMP(Value) ? HouseClass::FindByIndex(Value)
                                                  : HouseClass::FindByCountryIndex(Value);
    if (!house) return false;
    bool changed = false;
    for (int i = 0; i < TechnoClass::Array.Count; ++i) {
        auto* techno = TechnoClass::Array[i];
        if (techno->IsAlive && techno->IsOnMap && !techno->InLimbo && techno->AttachedTag &&
            techno->AttachedTag->ContainsTrigger(trigger)) {
            techno->SetOwningHouse(house, false);
            changed = true;
        }
    }
    return changed;
}

bool TActionClass::CreateTeam(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    ++Unsorted::ScenarioInit;
    if (TeamType) TeamType->CreateTeam(nullptr);
    --Unsorted::ScenarioInit;
    // The action succeeds even when no team type or allocation is available.
    return true;
}
bool TActionClass::CreateParticleSystemAt(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) {
    auto* type = ParticleSystemTypeClass::Array.GetItemOrDefault(Value);
    if (type && ScenarioClass::Instance) {
        const auto cell = ScenarioClass::Instance->GetWaypointCoords(Waypoint);
        CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
        at.Z = MapClass::Instance.GetCellFloorHeight(at);
        if (auto* memory = YRMemory::Allocate(sizeof(ParticleSystemClass)))
            ::new (memory) ParticleSystemClass(type, at, nullptr, nullptr, CoordStruct::Empty, nullptr);
    }
    return true;
}
bool TActionClass::RemoveParticleSystemsAt(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) {
    if (ScenarioClass::Instance) {
        const auto cell = ScenarioClass::Instance->GetWaypointCoords(Waypoint);
        for (int i = 0; i < ParticleSystemClass::Array.Count; ++i) {
            auto* system = ParticleSystemClass::Array[i];
            if (system->IsAlive && system->GetMapCoords() == cell) system->UnInit();
        }
    }
    return true;
}
bool TActionClass::Execute(HouseClass* house, ObjectClass* object, TriggerClass* trigger, const CellStruct& location) {
    auto* scenario = ScenarioClass::Instance;
    switch (ActionKind) {
        case TriggerAction::DestroyTeam:
            return DestroyAllInstancesOfTeam(house, object, trigger, location);
        case TriggerAction::FireSale:
            return FireSale(house, object, trigger, location);
        case TriggerAction::PlayMovie:
            return PlayMovie(house, object, trigger, location);
        case TriggerAction::RevealWaypointZone:
            return RevealZoneOfWaypoint(house, object, trigger, location);
        case TriggerAction::PlayMusicTheme:
            return PlayTheme(house, object, trigger, location);
        case TriggerAction::ForceTrigger:
            return ForceTrigger(house, object, trigger, location);
        case TriggerAction::TimerExtend:
            return MissionTimerExtend(house, object, trigger, location);
        case TriggerAction::MakeAlly:
            return MakeTwoHousesAlly(house, object, trigger, location);
        case TriggerAction::MakeEnemy:
            return MakeTwoHousesEnemy(house, object, trigger, location);
        case TriggerAction::PlayAnimAt:
            return PlayAnimAtWP(house, object, trigger, location);
        case TriggerAction::DoExplosionAt:
            return FireWeaponAtWP(house, object, trigger, location);
        case TriggerAction::IonStormStart:
            return LightningStormStart(house, object, trigger, location);
        case TriggerAction::CreateRadarEvent:
            return RadarEventAdd(house, object, trigger, location);
        case TriggerAction::TurnOffBuilding:
            return SendAttachedBuildingsOffline(house, object, trigger, location);
        case TriggerAction::Apply100Damage:
            return Apply100DamageAt(house, object, trigger, location);
        case TriggerAction::AITriggersBegin:
            return EnableAITriggers(house, object, trigger, location);
        case TriggerAction::AITriggersStop:
            return DisableAITriggers(house, object, trigger, location);
        case TriggerAction::RatioOfTeamAircraft:
            return SetRatioAITeamAircraft(house, object, trigger, location);
        case TriggerAction::RatioOfTeamInfantry:
            return SetRatioAITeamInfantry(house, object, trigger, location);
        case TriggerAction::WakeupSelf:
            return WakeupAttachedObjects(house, object, trigger, location);
        case TriggerAction::LightningStrike:
            return LightningStrikeAt(house, object, trigger, location);
        case TriggerAction::PlaySoundEffectAtWaypoint:
            return PlayAudioAtSpecificWP(house, object, trigger, location);
        case TriggerAction::LightningStormStrike:
            return LightningStormAtWP(house, object, trigger, location);
        case TriggerAction::FlashTeam:
            return FlashTeamFor(house, object, trigger, location);
        case TriggerAction::SetObjectTechLevel:
            return SetTechLevelOfTechnoType(house, object, trigger, location);
        case TriggerAction::CreateCrate:
            return CrateAtWP(house, object, trigger, location);
        case TriggerAction::SetTabTo:
            return SwitchToTab(house, object, trigger, location);
        case TriggerAction::FlashCameo:
            return FlashCameo(house, object, trigger, location);
        case TriggerAction::StopSounds:
            return StopAudioAtSpecificWP(house, object, trigger, location);
        case TriggerAction::SetSuperWeaponCharge:
            return SetSWChargeLevel(house, object, trigger, location);
        case TriggerAction::SuperWeaponSetRechargeTime:
            return SetSWRechargeTime(house, object, trigger, location);
        case TriggerAction::SetPreferredTargetCell:
            return SetPreferredTargetCell(house, object, trigger, location);
        case TriggerAction::ClearPreferredTargetCell:
            return ResetTargetCell(house, object, trigger, location);
        case TriggerAction::SetBaseCenterCell:
            return SetBaseCenter(house, object, trigger, location);
        case TriggerAction::AllToHunt: {
            auto* target = FindHouseByIndex(trigger, Value);
            if (!target) return false;
            target->All_To_Hunt();
            return true;
        }
        case TriggerAction::PreferredTarget:
            if (house) house->PreferredTargetType = static_cast<QuarryType>(Value);
            return true;

        // Country-sensitive actions use the scenario country index, not the
        // type registry position. Announcements (67/68) use the current player.
        case TriggerAction::Win:
            if (Value == HouseClass::CurrentPlayer->Type->ArrayIndex2)
                HouseClass::CurrentPlayer->Win(false);
            else
                HouseClass::CurrentPlayer->Lose(false);
            return true;
        case TriggerAction::Lose:
            return LoserIs(house, object, trigger, location);
        case TriggerAction::AnnounceWin:
            return PlayerWin(house, object, trigger, location);
        case TriggerAction::AnnounceLose:
            return PlayerLose(house, object, trigger, location);
        case TriggerAction::AutoBaseBuilding:
            return SetAutoBaseBuilding(house, object, trigger, location);
        case TriggerAction::ProductionBegins:
            return EnableProductionForHouse(house, object, trigger, location);
        case TriggerAction::DestroyTrigger:
            return DeleteTrigger(house, object, trigger, location);
        case TriggerAction::DestroyTag:
            return DeleteTag(house, object, trigger, location);
        case TriggerAction::DestroyAttachedObject:
            return DestroyAttached(house, object, trigger, location);
        case TriggerAction::AllChangeHouse:
            return SwitchAllObjectsToHouse(house, object, trigger, location);
        case TriggerAction::SellBuilding:
            return SellAttachedBuildings(house, object, trigger, location);
        case TriggerAction::MakeHouseCheer:
            return Cheer(house, object, trigger, location);
        case TriggerAction::ResizePlayerView:
            return ResizePlayerView(house, object, trigger, location);
        case TriggerAction::TeleportAll:
            return TeleportAllHouseObjectTo(house, object, trigger, location);
        case TriggerAction::TimerStart:
            return MissionTimerStart(house, object, trigger, location);
        case TriggerAction::TimerSet:
            return MissionTimerStartWithText(house, object, trigger, location);
        case TriggerAction::TimerText:
            return MissionTimerSetText(house, object, trigger, location);
        case TriggerAction::ReshroudMap:
            MapClass::Instance.ReshroudShroud(house);
            return true;
        case TriggerAction::PlayIngameMovie:
            game::queue_sidebar_movie(Value, false);
            return true;
        case TriggerAction::PlayIngameMovieAndPause:
            game::queue_sidebar_movie(Value, true);
            return true;
        case TriggerAction::StartChronoScreenEffect:
            ChronoScreenEffect::Start(Value);
            return true;
        case TriggerAction::RevealAroundWaypoint:
            return RevealAroundWaypoint(house, object, trigger, location);
        case TriggerAction::ReshroudMapAtWaypoint:
            return ReshroudMapAroundWaypoint(house, object, trigger, location);
        case TriggerAction::RevealAllMap: {
            auto* player = HouseClass::CurrentPlayer;
            if (!player->Visionary) {
                player->Visionary = true;
                auto& map = MapClass::Instance;
                map.CellIteratorReset();
                while (auto* cell = map.CellIteratorNext())
                    RadarClass::Instance.MapCell(&cell->MapCoords, player);
                map.MarkNeedsRedraw(1);
            }
            return true;
        }
        case TriggerAction::PlaySoundEffect:
            VocClass::PlayGlobal(Value, 0x2000, 1.0f, nullptr);
            return true;
        case TriggerAction::TimerStop:
            return MissionTimerStop(house, object, trigger, location);
        case TriggerAction::EvictOccupiers:
            return EjectOccupantsOfFirstAttachedBuilding(house, object, trigger, location);
        case TriggerAction::DestroyAll:
            return DestroyAllObjectsOf(house, object, trigger, location);
        case TriggerAction::DestroyAllBuildings:
            return DestroyAllBuildingsOf(house, object, trigger, location);
        case TriggerAction::DestroyAllLandUnits:
            return DestroyAllNonNavalUnitsOf(house, object, trigger, location);
        case TriggerAction::DestroyAllNavalUnits:
            return DestroyAllNavalUnitsOf(house, object, trigger, location);
        case TriggerAction::RestoreStartingBuildings:
            return RestoreStartingBuildingsOf(house, object, trigger, location);
        case TriggerAction::CreateBuilding:
            return SpawnBuilding(house, object, trigger, location);
        case TriggerAction::ClearAllSmudges:
            return ClearSmudges(house, object, trigger, location);
        case TriggerAction::BlackoutRadar:
            return RadarBlackout(house, object, trigger, location);
        case TriggerAction::LockInput: {
            auto& mouse = MouseClass::Instance;
            mouse.CurrentBuilding = nullptr;
            mouse.CurrentBuildingType = nullptr;
            mouse.CurrentBuildingOwnerArrayIndex = -1;
            mouse.SetActiveFoundation(nullptr);
            mouse.SetRepairMode(0);
            mouse.SetSellMode(0);
            mouse.PowerToggleMode = mouse.PlanningMode = mouse.PlaceBeaconMode = false;
            ScenarioClass::LockInput();
            return true;
        }
        case TriggerAction::UnlockInput:
            ScenarioClass::UnlockInput();
            return true;
        case TriggerAction::ParticleAnim:
            return CreateParticleSystemAt(house, object, trigger, location);
        case TriggerAction::RemoveParticleAnim:
            return RemoveParticleSystemsAt(house, object, trigger, location);
        case TriggerAction::FlashBuildingsOfType:
            return FlashBuildingsOfType(house, object, trigger, location);
        case TriggerAction::MoveCameraToWaypoint:
        case TriggerAction::CenterCameraAtWaypoint: {
            const auto cell = scenario->GetWaypointCoords(Waypoint);
            CoordStruct target{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
            target.Z = MapClass::Instance.GetCellFloorHeight(target);
            if ((unsigned(MapClass::Instance.GetCellAt(cell)->Flags) & 0x100u) ||
                (unsigned(MapClass::Instance.GetCellAt(cell)->Flags) & 0x400u))
                target.Z += CellClass::BridgeHeight;
            if (ActionKind == TriggerAction::MoveCameraToWaypoint)
                TacticalClass::Instance->FocusOn(&target, Value);
            else
                TacticalClass::Instance->SetTacticalPosition(&target);
            return true;
        }
        case TriggerAction::CreateTeam:
            return CreateTeam(house, object, trigger, location);
        case TriggerAction::TextTrigger:
            return PrintMessage(house, object, trigger, location);
        case TriggerAction::PlaySpeech:
            return PlayEVA(house, object, trigger, location);
        case TriggerAction::ChangeHouse:
            return SwitchAttachedObjectsToHouse(house, object, trigger, location);
        case TriggerAction::ReinforcementAt:
            return SpawnTeamAtWP(house, object, trigger, location);
        case TriggerAction::Reinforcement:
            return SpawnTeam(house, object, trigger, location);
        case TriggerAction::ReinforcementByChrono:
            return ChronoInTeam(house, object, trigger, location);
        case TriggerAction::EnableTrigger:
            for (auto* trigger : TriggerClass::Array)
                if (trigger->Type == TriggerType && TriggerType) {
                    const auto difficulty = scenario->Difficulty1;
                    if (difficulty >= 3 || TriggerType->Difficulty[difficulty]) trigger->Enable();
                }
            return true;
        case TriggerAction::DisableTrigger:
            for (auto* trigger : TriggerClass::Array)
                if (trigger->Type == TriggerType && TriggerType) trigger->Disable();
            return true;
        case TriggerAction::GlobalSet:
            scenario->SetGlobal(Value, 1);
            return true;
        case TriggerAction::GlobalClear:
            scenario->SetGlobal(Value, 0);
            return true;
        case TriggerAction::LocalSet:
            scenario->SetLocal(Value, 1);
            return true;
        case TriggerAction::LocalClear:
            scenario->SetLocal(Value, 0);
            return true;
        default:
            // AllowWin is intentionally unimplemented; see TriggerAction::AllowWin.
            return false;
    }
}
