// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tevent.cpp: Attaches_To, operator(), Is_Persistent.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x71F680/0x71E940/0x71F950/0x71F9C0 calibrate event IDs and masks.
#include "yrpp/TEventClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstring>

TriggerAttachType YRPP_FASTCALL TEventClass::GetAttachType(int eventKind) {
    const auto kind = static_cast<TriggerEvent>(eventKind);
    TriggerAttachType flags{};
    switch (kind) {
        case TriggerEvent::None:
        case TriggerEvent::EnteredBy:
        case TriggerEvent::DiscoveredByPlayer:
        case TriggerEvent::AnyEvent:
        case TriggerEvent::ZoneEntryBy:
        case TriggerEvent::CrossesHorizontalLine:
        case TriggerEvent::CrossesVerticalLine:
        case TriggerEvent::AllBridgesDestroyed:
        case TriggerEvent::SpyAsHouse:
        case TriggerEvent::SpyAsInfantry:
        case TriggerEvent::EnteredOrOverflownBy:
            flags |= TriggerAttachType::Global;
            break;
        default:
            break;
    }
    switch (kind) {
        case TriggerEvent::None:
        case TriggerEvent::EnteredBy:
        case TriggerEvent::SpiedBy:
        case TriggerEvent::DiscoveredByPlayer:
        case TriggerEvent::AttackedByAnybody:
        case TriggerEvent::DestroyedByAnybody:
        case TriggerEvent::AnyEvent:
        case TriggerEvent::DestroyedFakesAll:
        case TriggerEvent::SelectedByPlayer:
        case TriggerEvent::ComesNearWaypoint:
        case TriggerEvent::EnemyInSpotlight:
        case TriggerEvent::FirstDamaged_combatonly:
        case TriggerEvent::HalfHealth_combatonly:
        case TriggerEvent::QuarterHealth_combatonly:
        case TriggerEvent::FirstDamaged_anysource:
        case TriggerEvent::HalfHealth_anysource:
        case TriggerEvent::QuarterHealth_anysource:
        case TriggerEvent::AttackedByHouse:
        case TriggerEvent::DestroyedByAnything:
        case TriggerEvent::PickupCrate:
            flags |= TriggerAttachType::Object;
            break;
        default:
            break;
    }
    if (kind == TriggerEvent::AnyEvent || kind == TriggerEvent::ZoneEntryBy) flags |= TriggerAttachType::Map;
    switch (kind) {
        case TriggerEvent::ThievedBy:
        case TriggerEvent::HouseDiscovered:
        case TriggerEvent::AnyEvent:
        case TriggerEvent::DestroyedUnitsAll:
        case TriggerEvent::DestroyedBuildingsAll:
        case TriggerEvent::DestroyedAll:
        case TriggerEvent::CreditsExceed:
        case TriggerEvent::DestroyedBuildingsNum:
        case TriggerEvent::DestroyedUnitsNum:
        case TriggerEvent::NoFactoriesLeft:
        case TriggerEvent::CiviliansEvacuated:
        case TriggerEvent::BuildBuildingType:
        case TriggerEvent::BuildUnitType:
        case TriggerEvent::BuildInfantryType:
        case TriggerEvent::BuildAircraftType:
        case TriggerEvent::LowPower:
        case TriggerEvent::BuildingExists:
        case TriggerEvent::CreditsBelow:
        case TriggerEvent::DestroyedUnitsNaval:
        case TriggerEvent::DestroyedUnitsLand:
        case TriggerEvent::BuildingDoesNotExist:
        case TriggerEvent::PowerFull:
            flags |= TriggerAttachType::House;
            break;
        default:
            break;
    }
    switch (kind) {
        case TriggerEvent::AnyEvent:
        case TriggerEvent::ElapsedTime:
        case TriggerEvent::MissionTimerExpired:
        case TriggerEvent::TeamLeavesMap:
        case TriggerEvent::GlobalSet:
        case TriggerEvent::GlobalCleared:
        case TriggerEvent::LocalSet:
        case TriggerEvent::LocalCleared:
        case TriggerEvent::AmbientLightBelow:
        case TriggerEvent::AmbientLightAbove:
        case TriggerEvent::ElapsedScenarioTime:
        case TriggerEvent::PickupCrate_any:
        case TriggerEvent::RandomDelay:
        case TriggerEvent::TechTypeExists:
        case TriggerEvent::TechTypeDoesntExist:
            flags |= TriggerAttachType::Logic;
            break;
        default:
            break;
    }
    return flags;
}
bool TEventClass::GetStateA() const {
    switch (EventKind) {
        case TriggerEvent::EnteredBy:
        case TriggerEvent::SpiedBy:
        case TriggerEvent::ThievedBy:
        case TriggerEvent::DiscoveredByPlayer:
        case TriggerEvent::AttackedByAnybody:
        case TriggerEvent::DestroyedByAnybody:
        case TriggerEvent::CiviliansEvacuated:
        case TriggerEvent::BuildBuildingType:
        case TriggerEvent::BuildUnitType:
        case TriggerEvent::BuildInfantryType:
        case TriggerEvent::BuildAircraftType:
        case TriggerEvent::TeamLeavesMap:
        case TriggerEvent::ZoneEntryBy:
        case TriggerEvent::CrossesHorizontalLine:
        case TriggerEvent::CrossesVerticalLine:
        case TriggerEvent::DestroyedFakesAll:
        case TriggerEvent::AllBridgesDestroyed:
        case TriggerEvent::SelectedByPlayer:
        case TriggerEvent::ComesNearWaypoint:
        case TriggerEvent::EnemyInSpotlight:
        case TriggerEvent::FirstDamaged_combatonly:
        case TriggerEvent::HalfHealth_combatonly:
        case TriggerEvent::QuarterHealth_combatonly:
        case TriggerEvent::FirstDamaged_anysource:
        case TriggerEvent::HalfHealth_anysource:
        case TriggerEvent::QuarterHealth_anysource:
        case TriggerEvent::AttackedByHouse:
        case TriggerEvent::DestroyedByAnything:
        case TriggerEvent::PickupCrate:
        case TriggerEvent::PickupCrate_any:
        case TriggerEvent::SpyAsHouse:
        case TriggerEvent::SpyAsInfantry:
        case TriggerEvent::EnteredOrOverflownBy:
            return true;
        default:
            return false;
    }
}
bool TEventClass::GetStateB() const {
    switch (EventKind) {
        case TriggerEvent::EnteredBy:
        case TriggerEvent::AttackedByAnybody:
        case TriggerEvent::AllBridgesDestroyed:
        case TriggerEvent::AttackedByHouse:
        case TriggerEvent::EnteredOrOverflownBy:
            return false;
        default:
            return true;
    }
}
bool TEventClass::HasOccured(int eventKind, HouseClass* owner, ObjectClass* object, CDTimerClass* timer,
                             bool* repeating, TechnoClass* source) {
    const auto kind = EventKind;
    auto* scenario = ScenarioClass::Instance;
    switch (kind) {
        case TriggerEvent::ElapsedTime:
        case TriggerEvent::MissionTimerExpired:
        case TriggerEvent::RandomDelay: {
            if (kind == TriggerEvent::MissionTimerExpired) {
                if (!scenario || !scenario->MissionTimer.IsTicking()) return false;
                timer = &scenario->MissionTimer;
            }
            if (!timer) return false;
            // 0x71EC33 uses signed, wrapping frame subtraction. Do not clamp
            // a wrapped negative remainder as TimerStruct::GetTimeLeft does.
            if (!timer->IsTicking()) return timer->TimeLeft == 0;
            const int elapsed = std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame) -
                                                   static_cast<unsigned>(timer->StartTime));
            return elapsed >= timer->TimeLeft ||
                   static_cast<unsigned>(timer->TimeLeft) - static_cast<unsigned>(elapsed) == 0;
        }
        case TriggerEvent::GlobalSet:
        case TriggerEvent::GlobalCleared:
        case TriggerEvent::LocalSet:
        case TriggerEvent::LocalCleared: {
            char value = 0;
            if (!scenario) return false;
            // A failed variable lookup has no valid predicate value. Keep
            // malformed indices outside the original's stack-slot reuse.
            if (kind == TriggerEvent::GlobalSet || kind == TriggerEvent::GlobalCleared) {
                if (!scenario->GetGlobal(Value, &value)) return false;
            } else if (!scenario->GetLocal(Value, &value))
                return false;
            return kind == TriggerEvent::GlobalSet || kind == TriggerEvent::LocalSet ? value != 0 : value == 0;
        }
        case TriggerEvent::AmbientLightBelow:
            return scenario && scenario->AmbientCurrent <= Value;
        case TriggerEvent::AmbientLightAbove:
            return scenario && scenario->AmbientCurrent >= Value;
        case TriggerEvent::ElapsedScenarioTime:
            return Value <= Unsorted::CurrentFrame / 15;
        case TriggerEvent::TechTypeExists:
        case TriggerEvent::TechTypeDoesntExist: {
            TechnoTypeClass* type = nullptr;
            for (int i = TechnoTypeClass::Array.Count - 1; i >= 0; --i)
                if (!std::strcmp(TechnoTypeClass::Array[i]->ID, String)) {
                    type = TechnoTypeClass::Array[i];
                    break;
                }
            if (!type) return false;
            int count = 0;
            for (int i = TechnoClass::Array.Count - 1; i >= 0; --i) {
                if (TechnoClass::Array[i]->GetTechnoType() == type) {
                    if (kind == TriggerEvent::TechTypeDoesntExist) return false;
                    ++count;
                }
                // The threshold is checked after every object, including a
                // nonmatching object. An empty array never satisfies event 60.
                if (kind == TriggerEvent::TechTypeExists && count >= Value) return true;
            }
            return kind == TriggerEvent::TechTypeDoesntExist;
        }
        default:
            break;
    }

    if (kind == TriggerEvent::None) return false;
    // 0x71EC6F..0x71ECA9 bypasses the notification gate for production
    // predicates 19..22. All other gated kinds match GetStateA's table.
    if ((kind < TriggerEvent::BuildBuildingType || kind > TriggerEvent::BuildAircraftType) && GetStateA() &&
        (eventKind != static_cast<int>(kind) || Unsorted::ArmageddonMode))
        return false;

    switch (kind) {
        case TriggerEvent::EnteredBy:
        case TriggerEvent::ZoneEntryBy:
        case TriggerEvent::CrossesHorizontalLine:
        case TriggerEvent::CrossesVerticalLine:
        case TriggerEvent::EnteredOrOverflownBy: {
            // Preserve both original country queries and their ordering
            // around the virtual owner query. Value is a country index.
            if (Value != -1 && !HouseClass::FindByCountryIndex(Value)) return false;
            if (!object) return false;
            if (Value != -1) {
                const int index = object->GetOwningHouseIndex();
                if (index != HouseClass::FindByCountryIndex(Value)->ArrayIndex) return false;
            }
            *repeating = true;
            House = object->GetOwningHouse();
            return true;
        }
        case TriggerEvent::SpyAsHouse: {
            if (!object || Value == -1 || !HouseClass::FindByCountryIndex(Value)) return false;
            if (!object->GetDisguiseHouse(true)) return false;
            auto* disguise = object->GetDisguiseHouse(true);
            if (disguise->ArrayIndex != HouseClass::FindByCountryIndex(Value)->ArrayIndex) return false;
            *repeating = true;
            return true;
        }
        case TriggerEvent::SpyAsInfantry: {
            if (!object) return false;
            auto* disguise = object->GetDisguise(true);
            // Original objects provide a disguise type for this notification.
            if (disguise->WhatAmI() != AbstractType::InfantryType || Value == -1 || disguise->GetArrayIndex() != Value)
                return false;
            *repeating = true;
            return true;
        }
        case TriggerEvent::ComesNearWaypoint: {
            CoordStruct waypoint, location;
            scenario->GetWaypointLocation(&waypoint, Value);
            const auto* coords = object->GetCoords(&location);
            const double dx = std::bit_cast<int>(static_cast<unsigned>(coords->X) - static_cast<unsigned>(waypoint.X));
            const double dy = std::bit_cast<int>(static_cast<unsigned>(coords->Y) - static_cast<unsigned>(waypoint.Y));
            const double dz = std::bit_cast<int>(static_cast<unsigned>(coords->Z) - static_cast<unsigned>(waypoint.Z));
            // 0x71EE38 uses the game's quantized sqrt, followed by truncation.
            // Quantize using the incoming x87 rounding before the lookup.
            const float squared = game::x87_multiply_to_float(dx * dx + dz * dz + dy * dy, 1.0);
            const int distance = game::x87_integer(Math::sqrt(squared));
            return distance <= 1280;
        }
        default:
            break;
    }

    if (kind == TriggerEvent::AttackedByHouse && (!source || Value != source->Owner->ArrayIndex)) return false;

    if (owner) {
        switch (kind) {
            case TriggerEvent::CreditsExceed:
                if (owner->Available_Money() < Value) return false;
                break;
            case TriggerEvent::DestroyedBuildingsNum:
                if (owner->TotalKilledBuildings < Value) return false;
                break;
            case TriggerEvent::DestroyedUnitsNum:
                if (owner->TotalKilledUnits < Value) return false;
                break;
            case TriggerEvent::NoFactoriesLeft:
                if (owner->OwnedBuildings > 0)
                    for (auto* building : owner->Buildings)
                        if (building && !building->InLimbo && building->Type->Factory != AbstractType::None)
                            return false;
                break;
            case TriggerEvent::CiviliansEvacuated:
                if (!owner->CiviliansEvacuated) return false;
                break;
            case TriggerEvent::BuildBuildingType:
            case TriggerEvent::BuildUnitType:
            case TriggerEvent::BuildInfantryType:
            case TriggerEvent::BuildAircraftType: {
                // Original +0x274/+0x278 names are reversed for unit/aircraft.
                int last = owner->LastBuiltBuildingType;
                if (kind == TriggerEvent::BuildUnitType)
                    last = owner->LastBuiltAircraftType;
                else if (kind == TriggerEvent::BuildInfantryType)
                    last = owner->LastBuiltInfantryType;
                else if (kind == TriggerEvent::BuildAircraftType)
                    last = owner->LastBuiltVehicleType;
                if (last != Value) return false;
                *repeating = true;
                break;
            }
            case TriggerEvent::TeamLeavesMap: {
                int i = 0;
                for (; i < TeamClass::Array.Count; ++i) {
                    const auto* team = TeamClass::Array[i];
                    if (team->Type == TeamType && !team->FirstUnit && team->IsLeavingMap) {
                        *repeating = true;
                        break;
                    }
                }
                if (i == TeamClass::Array.Count) return false;
                break;
            }
            case TriggerEvent::BuildingExists:
            case TriggerEvent::BuildingDoesNotExist:
                if ((owner->ActiveBuildingTypes.GetItemCount(Value) != 0) != (kind == TriggerEvent::BuildingExists))
                    return false;
                *repeating = true;
                break;
            case TriggerEvent::CreditsBelow:
                if (owner->Available_Money() > Value) return false;
                break;
            default:
                break;
        }
    }

    // 0x71F064 is shared by every remaining nonzero event, even AnyEvent
    // and unrecognized kinds. The country can differ from the trigger owner.
    auto* house = HouseClass::FindByCountryIndex(Value);
    if (!house) return true;
    switch (kind) {
        case TriggerEvent::ThievedBy:
            return house->HasBeenThieved;
        case TriggerEvent::HouseDiscovered:
            return house->DiscoveredByPlayer;
        case TriggerEvent::DestroyedUnitsAll:
            return house->ActiveUnitTypes.GetTotal() <= 0 && house->ActiveInfantryTypes.GetTotal() <= 0;
        case TriggerEvent::DestroyedBuildingsAll:
            return house->OwnedBuildings <= 0;
        case TriggerEvent::DestroyedAll:
            return house->OwnedBuildings <= 0 && house->ActiveUnitTypes.GetTotal() <= 0 &&
                   house->ActiveInfantryTypes.GetTotal() <= 0;
        case TriggerEvent::LowPower:
            return house->GetPowerPercentage() < 1.0;
        case TriggerEvent::DestroyedUnitsNaval:
            return house->OwnedNavy <= 0;
        case TriggerEvent::DestroyedUnitsLand:
            return std::bit_cast<int>(unsigned(house->OwnedUnits) - unsigned(house->OwnedNavy)) <= 0 &&
                   house->OwnedInfantry <= 0;
        case TriggerEvent::PowerFull:
            return house->GetPowerPercentage() >= 1.0;
        default:
            return true;
    }
}
