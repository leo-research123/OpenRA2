// Original starting records, 0x50D610, and building restoration, 0x50D320.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include <cstdlib>

void HouseClass::RecordStartingUnits() noexcept {
    try {
        for (int i = TechnoClass::Array.Count - 1; i >= 0; --i) {
            auto* techno = TechnoClass::Array[i];
            if (techno->Owner != this) continue;
            CellStruct output;
            const auto cell = *techno->GetMapCoords(&output);
            auto* type = techno->GetTechnoType();
            auto* record = GameCreate<StartingTechnoStruct>(type, cell);
            // The original append is nontransactional: a refusing vector
            // does not roll back earlier entries or reclaim this record.
            (void)StartingUnits.AddItem(record);
        }
    } catch (...) {
        std::abort();
    }  // Original allocation failure is fatal.
}

void HouseClass::RespawnStartingBuildings() noexcept {
    try {
        for (int i = StartingUnits.Count - 1; i >= 0; --i) {
            auto* record = StartingUnits[i];
            auto* type = record->Unit;
            if (type->WhatAmI() != AbstractType::BuildingType) continue;
            auto* cell = MapClass::Instance.GetCellAt(record->Cell);
            auto* building = cell->GetBuilding();
            if (building && building->Type == type && building->Owner == this) {
                int healing = -type->Strength;
                building->ReceiveDamage(&healing, 0, RulesClass::Instance->C4Warhead, nullptr, false, false, nullptr);
            } else {
                auto* object = type->CreateObject(this);
                auto* restored = object && object->WhatAmI() == AbstractType::Building
                                     ? static_cast<BuildingClass*>(object)
                                     : nullptr;
                // Creation can change cell contents. The original re-queries
                // the map before forced placement rather than caching it.
                auto position = MapClass::Instance.GetCellAt(record->Cell)->GetCoords();
                if (!restored) std::abort();  // Original null/wrong-type path is fatal.
                if (!restored->ForceCreate(position, 0)) restored->UnInit();
            }
        }
    } catch (...) {
        std::abort();
    }
}
