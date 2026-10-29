// Campaign actions recovered from YR TActionClass, 0x006DD8B0.
#include "yrpp/TActionClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/ThemeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/SuperClass.h"
#include <bit>
#include <cstring>

HouseClass* TActionClass::FindHouseByIndex(TriggerClass* trigger, int index) const {
    if (!trigger || index == -1) return nullptr;
    if (index == 8997) return trigger->GetHouse();
    return HouseClass::Index_IsMP(index) ? HouseClass::FindByIndex(index) : HouseClass::FindByCountryIndex(index);
}
bool TActionClass::DestroyAllInstancesOfTeam(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (TeamType) TeamType->DestroyAllInstances();
    return true;
}
bool TActionClass::FireSale(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    auto* house = FindHouseByIndex(trigger, Value);
    if (!house) return false;
    house->AIMode = static_cast<AIMode>(4);
    return true;
}
bool TActionClass::PlayMovie(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    // The host owns presentation, cursor visibility and movie input. Indexed
    // playback preserves the original tactical-active state around this call.
    Game::PlayMovie(Value, -1, 1, 1, 1);
    return true;
}
bool TActionClass::PlayTheme(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    ThemeClass::Instance.Queue(Value);
    return true;
}
bool TActionClass::ForceTrigger(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (TriggerType)
        for (int i = 0; i < TriggerClass::Array.Count; ++i)
            if (TriggerClass::Array[i]->Type == TriggerType)
                TriggerClass::Array[i]->FireActions(nullptr, CellStruct::Empty);
    return true;
}
bool TActionClass::MissionTimerExtend(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    auto& timer = ScenarioClass::Instance->MissionTimer;
    timer.Start(std::bit_cast<int>(unsigned(timer.GetTimeLeft()) + 15u * unsigned(Value)));
    TabClass::Instance.RefreshMissionTimer();
    return true;
}
bool TActionClass::MakeTwoHousesAlly(HouseClass* house, ObjectClass*, TriggerClass* trigger,
                                     const CellStruct&) noexcept {
    auto* target = FindHouseByIndex(trigger, Value);
    if (!target) return false;
    house->MakeAlly(target, false);
    target->MakeAlly(house, false);
    return true;
}
bool TActionClass::MakeTwoHousesEnemy(HouseClass* house, ObjectClass*, TriggerClass* trigger,
                                      const CellStruct&) noexcept {
    auto* target = FindHouseByIndex(trigger, Value);
    if (!target) return false;
    house->MakeEnemy(target, false);
    target->MakeEnemy(house, false);
    return true;
}
bool TActionClass::RadarEventAdd(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    RadarEventClass::Create(static_cast<RadarEventType>(Value), ScenarioClass::Instance->GetWaypointCoords(Waypoint));
    return true;
}
bool TActionClass::SendAttachedBuildingsOffline(HouseClass*, ObjectClass*, TriggerClass* trigger,
                                                const CellStruct&) noexcept {
    bool changed = false;
    for (int i = 0; i < BuildingClass::Array.Count; ++i) {
        auto* building = BuildingClass::Array[i];
        if (building->IsAlive && building->IsOnMap && !building->InLimbo && building->AttachedTag &&
            building->AttachedTag->ContainsTrigger(trigger) && building->HasPower) {
            building->GoOffline();
            changed = true;
        }
    }
    return changed;
}
bool TActionClass::EnableAITriggers(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    auto* house = FindHouseByIndex(trigger, Value);
    if (!house) return false;
    house->AITriggersActive = true;
    return true;
}
bool TActionClass::DisableAITriggers(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    auto* house = FindHouseByIndex(trigger, Value);
    if (!house) return false;
    house->AITriggersActive = false;
    return true;
}
bool TActionClass::SetRatioAITeamAircraft(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (house) house->RatioTeamAircraft = Value;
    return true;
}
bool TActionClass::SetRatioAITeamInfantry(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (house) house->RatioTeamInfantry = Value;
    return true;
}
bool TActionClass::WakeupAttachedObjects(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    for (int i = 0; i < TechnoClass::Array.Count; ++i) {
        auto* techno = TechnoClass::Array[i];
        if (techno->WhatAmI() != AbstractType::Building && techno->IsAlive && techno->IsOnMap && techno->AttachedTag &&
            techno->AttachedTag->ContainsTrigger(trigger) &&
            (techno->GetCurrentMission() == Mission::Sleep || techno->GetCurrentMission() == Mission::Harmless))
            techno->QueueMission(Mission::Guard, false);
    }
    return true;
}
bool TActionClass::FlashTeamFor(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (TeamType) TeamType->FlashAllInstances(Value);
    return true;
}
bool TActionClass::SetTechLevelOfTechnoType(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    for (int i = TechnoTypeClass::Array.Count - 1; i >= 0; --i)
        if (!std::strcmp(TechnoTypeClass::Array[i]->ID, TechnoID)) {
            TechnoTypeClass::Array[i]->TechLevel = Value;
            break;
        }
    for (int i = HouseClass::Array.Count - 1; i >= 0; --i)
        HouseClass::Array[i]->RecheckTechTree = true;
    return true;
}
bool TActionClass::CrateAtWP(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    return MapClass::Instance.PlacePowerupCrate(ScenarioClass::Instance->GetWaypointCoords(Waypoint),
                                                static_cast<Powerup>(Value));
}
bool TActionClass::SwitchToTab(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    auto& sidebar = SidebarClass::Instance;
    if (unsigned(Value) >= 4 || sidebar.Tabs[Value].CameoCount <= 0) return false;
    sidebar.SetTab(Value);
    return true;
}
bool TActionClass::FlashCameo(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    for (int i = TechnoTypeClass::Array.Count - 1; i >= 0; --i)
        if (!std::strcmp(TechnoTypeClass::Array[i]->ID, TechnoID)) {
            SidebarClass::Instance.FlashCameo(TechnoTypeClass::Array[i], Value);
            return true;
        }
    return false;
}
bool TActionClass::SetSWChargeLevel(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    // The original admits index == Count. Native storage cannot dereference
    // that sentinel; reject it instead of reading outside the owned array.
    if (!house || Value < 0 || Value >= house->Supers.Count || unsigned(Value2) > 100) return false;
    auto* super = house->Supers[Value];
    if (!super || !super->IsPresent) return false;
    super->SetCharge(Value2);
    return true;
}
bool TActionClass::SetSWRechargeTime(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (!house || Value < 0 || Value >= house->Supers.Count || !house->Supers[Value]) return false;
    house->Supers[Value]->SetRechargeTime(Value2);
    return true;
}
bool TActionClass::SetPreferredTargetCell(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (!house) return false;
    const auto cell = ScenarioClass::Instance->GetWaypointCoords(Waypoint);
    if (cell == CellStruct::Empty) return false;
    house->PreferredTargetCell = cell;
    return true;
}
bool TActionClass::ResetTargetCell(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (!house) return false;
    house->PreferredTargetCell = CellStruct::Empty;
    return true;
}
bool TActionClass::SetBaseCenter(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (!house) return false;
    const auto cell = ScenarioClass::Instance->GetWaypointCoords(Waypoint);
    if (cell == CellStruct::Empty) return false;
    house->BaseCenter = cell;
    return true;
}
