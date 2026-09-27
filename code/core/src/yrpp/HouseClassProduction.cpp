#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/UnitClass.h"
#include <bit>

double HouseClass::GetBuildTimeMultiplier(TechnoTypeClass* type) const {
    switch(type->WhatAmI()) {
    case AbstractType::AircraftType:return Type->BuildtimeAircraftMult;
    case AbstractType::InfantryType:return Type->BuildtimeInfantryMult;
    case AbstractType::UnitType:return Type->BuildtimeUnitsMult;
    case AbstractType::BuildingType:
        return static_cast<BuildingTypeClass*>(type)->BuildCat==BuildCat::Combat?
            Type->BuildtimeDefensesMult:Type->BuildtimeBuildingsMult;
    default:return 1.0;
    }
}

int HouseClass::CountFactories(AbstractType type,bool naval) const {
    switch(type) {
    case AbstractType::Aircraft:case AbstractType::AircraftType:return NumAirpads;
    case AbstractType::Infantry:case AbstractType::InfantryType:return NumBarracks;
    case AbstractType::Unit:case AbstractType::UnitType:return naval?NumShipyards:NumWarFactories;
    case AbstractType::Building:case AbstractType::BuildingType:return NumConYards;
    default:return 0;
    }
}

// 0x0050B370: queried by the active sidebar when a techno is lost. Preserve
// positive (currently owned), nonpositive (ever produced), airport capacity
// and hijacker branches; this is also reached by ordinary map garrisons.
bool HouseClass::HasReachedBuildLimit(TechnoTypeClass* type) {
    if(!type)return true;
    FactoryClass* factory=nullptr;
    switch(type->WhatAmI()) {
    case AbstractType::Unit:case AbstractType::UnitType:
        factory=type->Naval?Primary_ForShips:Primary_ForVehicles;break;
    case AbstractType::Aircraft:case AbstractType::AircraftType:factory=Primary_ForAircraft;break;
    case AbstractType::Building:case AbstractType::BuildingType:factory=Primary_ForBuildings;break;
    case AbstractType::Infantry:case AbstractType::InfantryType:factory=Primary_ForInfantry;break;
    default:break;
    }
    const int queued=factory?factory->CountTotal(type):0;
    const auto add=[](int a,int b){return std::bit_cast<int>(DWORD(a)+DWORD(b));};
    const int limit=type->BuildLimit;
    const int absolute=limit<0?std::bit_cast<int>(0u-DWORD(limit)):limit;
    switch(type->WhatAmI()) {
    case AbstractType::AircraftType:
        if(static_cast<AircraftTypeClass*>(type)->AirportBound) {
            int active=0,products=0;
            for(auto* aircraft:RulesClass::Instance->PadAircraft) {
                active=add(active,ActiveAircraftTypes.GetItemCount(aircraft->GetArrayIndex()));
                if(factory)products=add(products,factory->CountTotal(aircraft));
            }
            return add(active,products)>=AirportDocks;
        }
        return add(queued,limit>0?OwnedAircraftTypes.GetItemCount(type->GetArrayIndex()):
            FactoryProducedAircraftTypes.GetItemCount(type->GetArrayIndex()))>=(limit>0?limit:absolute);
    case AbstractType::BuildingType:
        return add(queued,limit>0?OwnedBuildingTypes.GetItemCount(type->GetArrayIndex()):
            FactoryProducedBuildingTypes.GetItemCount(type->GetArrayIndex()))>=(limit>0?limit:absolute);
    case AbstractType::UnitType:
        return add(queued,limit>0?OwnedUnitTypes.GetItemCount(type->GetArrayIndex()):
            FactoryProducedUnitTypes.GetItemCount(type->GetArrayIndex()))>=(limit>0?limit:absolute);
    case AbstractType::InfantryType: {
        if(limit<=0&&add(queued,FactoryProducedInfantryTypes.GetItemCount(type->GetArrayIndex()))>=absolute)return true;
        int owned=OwnedInfantryTypes.GetItemCount(type->GetArrayIndex());
        if(static_cast<InfantryTypeClass*>(type)->VehicleThief) {
            for(auto* unit:UnitClass::Array)
                if(unit->Owner==this&&unit->HijackerInfantryType==type->GetArrayIndex())owned=add(owned,1);
        }
        return limit>0&&add(queued,owned)>=limit;
    }
    default:return false;
    }
}
