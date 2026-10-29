// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Can_Build; YR 0x4F7870 ground production branches.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/UnitClass.h"
CanBuildResult HouseClass::CanBuild(const TechnoTypeClass* type, bool limitOnly, bool allowInProduction) const {
    if (!type) return CanBuildResult::Unbuildable;
    if (!limitOnly) {
        if (type->Unbuildable) return CanBuildResult::Unbuildable;
        bool override = false;
        for (int index : type->PrerequisiteOverride)
            if (index >= 0 && ActiveBuildingTypes.GetItemCount(index) > 0) {
                override = true;
                break;
            }
        if (!override && !HasFromSecretLab(type)) {
            const DWORD country = DWORD(1) << (Type->ArrayIndex2 & 31);
            DWORD alternate = 0;
            switch (type->WhatAmI()) {
                case AbstractType::BuildingType:
                    alternate = BuildingAltOwner;
                    break;
                case AbstractType::UnitType:
                    alternate = UnitAltOwner;
                    break;
                case AbstractType::InfantryType:
                    alternate = InfantryAltOwner;
                    break;
                case AbstractType::AircraftType:
                    alternate = AircraftAltOwner;
                    break;
                default:
                    break;
            }
            if (type->TechLevel == -1 || !HasAllStolenTech(type) ||
                !type->InRequiredHouses(country | alternate) || type->InForbiddenHouses(country))
                return CanBuildResult::Unbuildable;
            if (!SessionClass::Instance.Config.SWAllowed && type->WhatAmI() == AbstractType::BuildingType) {
                auto* building = static_cast<const BuildingTypeClass*>(type);
                if (building->SuperWeapon != -1) {
                    bool isTech = false;
                    for (auto* entry : RulesClass::Instance->BuildTech)
                        if (entry == building) {
                            isTech = true;
                            break;
                        }
                    if (!isTech && Supers[building->SuperWeapon]->Type->DisableableFromShell)
                        return CanBuildResult::Unbuildable;
                }
            }
            if (type->TechLevel > TechLevel) return CanBuildResult::Unbuildable;
            if (!IsControlledByHuman()) return CanBuildResult::Buildable;
            for (int index : type->Prerequisite) {
                bool satisfied = false;
                if (index >= 0)
                    satisfied = ActiveBuildingTypes.GetItemCount(index) > 0;
                else {
                    const auto& rules = *RulesClass::Instance;
                    const TypeList<int>* groups[]{&rules.PrerequisitePower,    &rules.PrerequisiteFactory,
                                                  &rules.PrerequisiteBarracks, &rules.PrerequisiteRadar,
                                                  &rules.PrerequisiteTech,     &rules.PrerequisiteProc};
                    if (index >= -6)
                        for (int item : *groups[-index - 1])
                            if (ActiveBuildingTypes.GetItemCount(item) > 0) {
                                satisfied = true;
                                break;
                            }
                    if (index == -6 && rules.PrerequisiteProcAlternate &&
                        ActiveUnitTypes.GetItemCount(rules.PrerequisiteProcAlternate->GetArrayIndex()) > 0)
                        satisfied = true;
                }
                if (!satisfied) return CanBuildResult::Unbuildable;
            }
        }
    }
    const int index = type->GetArrayIndex();
    int owned = 0, produced = 0;
    switch (type->WhatAmI()) {
        case AbstractType::InfantryType:
            owned = OwnedInfantryTypes.GetItemCount(index);
            if (static_cast<const InfantryTypeClass*>(type)->VehicleThief)
                for (auto* unit : UnitClass::Array)
                    if (unit->Owner == this && unit->HijackerInfantryType == index) ++owned;
            produced = FactoryProducedInfantryTypes.GetItemCount(index);
            break;
        case AbstractType::UnitType:
            owned = OwnedUnitTypes.GetItemCount(index);
            produced = FactoryProducedUnitTypes.GetItemCount(index);
            break;
        case AbstractType::AircraftType:
            owned = OwnedAircraftTypes.GetItemCount(index);
            produced = FactoryProducedAircraftTypes.GetItemCount(index);
            break;
        case AbstractType::BuildingType:
            owned = OwnedBuildingTypes.GetItemCount(index);
            produced = FactoryProducedBuildingTypes.GetItemCount(index);
            break;
        default:
            return CanBuildResult::Unbuildable;
    }
    if (type->BuildLimit <= 0)
        return static_cast<CanBuildResult>(produced < -static_cast<long long>(type->BuildLimit) ? 1 : 0);
    if (owned < type->BuildLimit) return static_cast<CanBuildResult>(1);
    if (allowInProduction && (type->WhatAmI() != AbstractType::InfantryType || owned == type->BuildLimit))
        for (const auto* factory : FactoryClass::Array)
            if (factory->Owner == this && factory->Object && factory->Object->GetTechnoType() == type)
                return CanBuildResult::Buildable;
    return CanBuildResult::TemporarilyUnbuildable;
}
