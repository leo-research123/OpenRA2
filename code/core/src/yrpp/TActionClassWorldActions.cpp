// Original world-transition actions, 0x6E13E0..0x6E4200.
#include "yrpp/TActionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TagClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include <cstdlib>

namespace {
HouseClass* selected_house(int value, TriggerClass* trigger) noexcept {
    if (!trigger || value == -1) return nullptr;
    return value == 8997                   ? trigger->GetHouse()
           : HouseClass::Index_IsMP(value) ? HouseClass::FindByIndex(value)
                                           : HouseClass::FindByCountryIndex(value);
}
}
bool TActionClass::PlayerWin(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    HouseClass::CurrentPlayer->Win(true);
    return true;
}
bool TActionClass::PlayerLose(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    HouseClass::CurrentPlayer->Lose(true);
    return true;
}
bool TActionClass::LoserIs(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (Value == HouseClass::CurrentPlayer->Type->ArrayIndex2)
        HouseClass::CurrentPlayer->Lose(false);
    else
        HouseClass::CurrentPlayer->Win(false);
    return true;
}
bool TActionClass::EnableProductionForHouse(HouseClass*, ObjectClass*, TriggerClass* trigger,
                                            const CellStruct&) noexcept {
    auto* house = selected_house(Value, trigger);
    if (!house) return false;
    house->Production = true;
    return true;
}
bool TActionClass::RestoreStartingBuildingsOf(HouseClass*, ObjectClass*, TriggerClass* trigger,
                                              const CellStruct&) noexcept {
    auto* house = selected_house(Value, trigger);
    if (!house) return false;
    house->RespawnStartingBuildings();
    return true;
}
bool TActionClass::DestroyAllObjectsOf(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    auto* house = selected_house(Value, trigger);
    if (!house) return false;
    house->DestroyAll();
    return true;
}
bool TActionClass::DestroyAllBuildingsOf(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    auto* house = selected_house(Value, trigger);
    if (!house) return false;
    house->DestroyAllBuildings();
    return true;
}
bool TActionClass::DestroyAllNonNavalUnitsOf(HouseClass*, ObjectClass*, TriggerClass* trigger,
                                             const CellStruct&) noexcept {
    auto* house = selected_house(Value, trigger);
    if (!house) return false;
    house->DestroyAllNonBuildingsNonNaval();
    return true;
}
bool TActionClass::DestroyAllNavalUnitsOf(HouseClass*, ObjectClass*, TriggerClass* trigger,
                                          const CellStruct&) noexcept {
    auto* house = selected_house(Value, trigger);
    if (!house) return false;
    house->DestroyAllNonBuildingsNaval();
    return true;
}
bool TActionClass::EjectOccupantsOfFirstAttachedBuilding(HouseClass*, ObjectClass*, TriggerClass* trigger,
                                                         const CellStruct&) noexcept {
    try {
        for (int i = 0; i < BuildingClass::Array.Count; ++i) {
            auto* building = BuildingClass::Array[i];
            if (building->IsAlive && building->IsOnMap && !building->InLimbo && building->AttachedTag &&
                building->AttachedTag->ContainsTrigger(trigger)) {
                building->UnloadOccupants(false, false);
                return true;
            }
        }
        return false;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::SpawnBuilding(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        if (!house) return false;
        const auto cell = ScenarioClass::Instance->GetWaypointCoords(Waypoint);
        auto position = MapClass::Instance.GetCellAt(cell)->GetCoords();
        const int index = BuildingTypeClass::FindIndex(TechnoID);
        if (index < 0 || index > BuildingTypeClass::Array.Count) return false;
        auto* type = BuildingTypeClass::Array[index];
        if (!type) return false;
        auto* object = type->CreateObject(house);
        if (!object) return false;
        auto* building = static_cast<BuildingClass*>(object);
        if (!building->ForceCreate(position, 0)) {
            building->UnInit();
            return false;
        }
        building->Place(false);
        return true;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::ClearSmudges(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    MapClass::Instance.ClearSmudges();
    MapClass::Instance.MarkNeedsRedraw(1);
    return true;
}
bool TActionClass::MissionTimerStop(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    auto& timer = ScenarioClass::Instance->MissionTimer;
    if (timer.IsTicking()) {
        timer.Pause();
        TabClass::Instance.RefreshMissionTimer();
    }
    return true;
}
bool TActionClass::RadarBlackout(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    if (!house) return false;
    house->CreateRadarOutage(Value);
    return true;
}

// The two ownership passes match 0x6E0B60: powered buildings transfer last.
bool TActionClass::SwitchAllObjectsToHouse(HouseClass* source, ObjectClass*, TriggerClass* trigger,
                                           const CellStruct&) noexcept {
    try {
        auto* target = selected_house(Value, trigger);
        if (!target) return false;
        bool changed = false;
        for (int pass = 0; pass < 2; ++pass)
            for (int i = 0; i < TechnoClass::Array.Count; ++i) {
                auto* object = TechnoClass::Array[i];
                if (object->Owner != source) continue;
                auto* building =
                    object->WhatAmI() == AbstractType::Building ? static_cast<BuildingClass*>(object) : nullptr;
                const bool powered =
                    building && building->Type && (building->Type->Powered || building->Type->PoweredSpecial);
                if (powered != (pass == 1)) continue;
                object->SetOwningHouse(target, false);
                if (powered) building->UpdatePowered();
                changed = true;
            }
        return changed;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::DeleteTrigger(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        if (TriggerType)
            for (int i = TriggerClass::Array.Count - 1; i >= 0; --i)
                if (TriggerClass::Array[i]->Type == TriggerType) TriggerClass::Array[i]->Destroy();
        return true;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::DeleteTag(HouseClass*, ObjectClass*, TriggerClass*, const CellStruct&) noexcept {
    try {
        if (TagType)
            for (int i = 0; i < TagClass::Array.Count; ++i)
                if (TagClass::Array[i]->Type == TagType) {
                    GameDelete(TagClass::Array[i]);
                    --i;
                }
        return true;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::SellAttachedBuildings(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    try {
        bool sold = false;
        for (int i = 0; i < BuildingClass::Array.Count; ++i) {
            auto* building = BuildingClass::Array[i];
            if (building->IsAlive && building->IsOnMap && !building->InLimbo && building->AttachedTag &&
                building->AttachedTag->ContainsTrigger(trigger)) {
                building->Sell(1);
                sold = true;
            }
        }
        return sold;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::DestroyAttached(HouseClass*, ObjectClass*, TriggerClass* trigger,
                                   const CellStruct& location) noexcept {
    try {
        bool destroyed = false, again;
        do {
            again = false;
            for (int i = 0; i < TechnoClass::Array.Count; ++i) {
                auto* object = TechnoClass::Array[i];
                if (object->Health > 0 && object->IsAlive && object->IsOnMap && !object->InLimbo &&
                    object->AttachedTag && object->AttachedTag->ContainsTrigger(trigger)) {
                    int damage = object->Health;
                    object->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false, nullptr);
                    destroyed = again = true;
                    --i;
                }
            }
        } while (again);
        if (location != CellStruct::Empty) {
            for (int i = 0; i < 5; ++i)
                if (MapClass::Instance.DamageBridgeAt(location)) break;
            const CoordStruct coords{int(location.X) * 256 + 128, int(location.Y) * 256 + 128, 0};
            Point2D point;
            TacticalClass::Instance->CoordsToScreen(&point, &coords);
            TacticalClass::Instance->RegisterDirtyArea({point.X - 128, point.Y - 128, 256, 256}, false);
        }
        return destroyed;
    } catch (...) {
        std::abort();
    }
}
bool TActionClass::Cheer(HouseClass*, ObjectClass*, TriggerClass* trigger, const CellStruct&) noexcept {
    auto* target = selected_house(Value, trigger);
    if (!target) return false;
    target->Cheer();
    return true;
}
