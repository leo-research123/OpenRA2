// Original factory-plant cost stacking, YR 0x0050BF60.
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include "x87_integer.hpp"

void HouseClass::CalculateCostMultipliers() {
    CostInfantryMult = CostUnitsMult = CostAircraftMult = CostBuildingsMult = CostDefensesMult = 1.0f;
    for (int i = 0; i < FactoryPlants.Count; ++i) {
        const auto* type = FactoryPlants[i]->Type;
        CostInfantryMult = game::x87_multiply_to_float(type->InfantryCostBonus, CostInfantryMult);
        CostUnitsMult = game::x87_multiply_to_float(type->UnitsCostBonus, CostUnitsMult);
        CostAircraftMult = game::x87_multiply_to_float(type->AircraftCostBonus, CostAircraftMult);
        CostBuildingsMult = game::x87_multiply_to_float(type->BuildingsCostBonus, CostBuildingsMult);
        CostDefensesMult = game::x87_multiply_to_float(type->DefensesCostBonus, CostDefensesMult);
    }
}
