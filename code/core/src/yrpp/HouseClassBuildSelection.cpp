// Original rules-list selection, YR 0x005051E0.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/SessionClass.h"

BuildingTypeClass* HouseClass::FirstBuildableFromArray(const DynamicVectorClass<BuildingTypeClass*>& items) {
    const unsigned owners = 1u << (unsigned(Type->FindParentCountryIndex()) & 31u);
    for (auto* type : items) {
        if (!(type->OwnerFlags & owners)) continue;
        const unsigned country = 1u << (unsigned(Type->ArrayIndex2) & 31u);
        if (type->RequiredHouses != -1 && !(unsigned(type->RequiredHouses) & country)) continue;
        if (type->ForbiddenHouses != -1 && (unsigned(type->ForbiddenHouses) & country)) continue;
        if (type->AIBasePlanningSide != -1 && type->AIBasePlanningSide != Type->SideIndex) continue;
        if (SessionClass::Instance.Config.SWAllowed || type->SuperWeapon == -1 ||
            RulesClass::Instance->BuildTech.FindItemIndex(type) >= 0)
            return type;
        if (!Supers[type->SuperWeapon]->Type->DisableableFromShell) return type;
    }
    return nullptr;
}

// This planning predicate uses Build* lists, not the player's Prerequisite* lists.
bool HouseClass::AllPrerequisitesAvailable(const TechnoTypeClass* type,
                                           const DynamicVectorClass<BuildingTypeClass*>& buildings, int count) {
    const auto& rules = *RulesClass::Instance;
    const auto contains = [&](const BuildingTypeClass* wanted) {
        for (int i = 0; i < count; ++i)
            if (buildings[i] == wanted) return true;
        return false;
    };
    for (int index : type->Prerequisite) {
        if (index >= 0) {
            if (!contains(BuildingTypeClass::Array[index])) return false;
            continue;
        }
        const TypeList<BuildingTypeClass*>* group = nullptr;
        switch (static_cast<::Prerequisite>(index)) {
            case ::Prerequisite::Power: group = &rules.BuildPower; break;
            case ::Prerequisite::Factory: group = &rules.BuildWeapons; break;
            case ::Prerequisite::Barracks: group = &rules.BuildBarracks; break;
            case ::Prerequisite::Radar: group = &rules.BuildRadar; break;
            case ::Prerequisite::Tech: group = &rules.BuildTech; break;
            case ::Prerequisite::Proc: group = &rules.BuildRefinery; break;
            default: return false;
        }
        bool found = false;
        for (auto* candidate : *group)
            if (contains(candidate)) { found = true; break; }
        if (!found) return false;
    }
    return true;
}
