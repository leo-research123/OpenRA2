#include "yrpp/HouseClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include <bit>
#include <algorithm>
#include "x87_integer.hpp"

double HouseClass::GetBuildTimeMultiplier(TechnoTypeClass* type) const {
    switch (type->WhatAmI()) {
        case AbstractType::AircraftType:
            return Type->BuildtimeAircraftMult;
        case AbstractType::InfantryType:
            return Type->BuildtimeInfantryMult;
        case AbstractType::UnitType:
            return Type->BuildtimeUnitsMult;
        case AbstractType::BuildingType:
            return static_cast<BuildingTypeClass*>(type)->BuildCat == BuildCat::Combat ? Type->BuildtimeDefensesMult
                                                                                       : Type->BuildtimeBuildingsMult;
        default:
            return 1.0;
    }
}

int HouseClass::CountFactories(AbstractType type, bool naval) const {
    switch (type) {
        case AbstractType::Aircraft:
        case AbstractType::AircraftType:
            return NumAirpads;
        case AbstractType::Infantry:
        case AbstractType::InfantryType:
            return NumBarracks;
        case AbstractType::Unit:
        case AbstractType::UnitType:
            return naval ? NumShipyards : NumWarFactories;
        case AbstractType::Building:
        case AbstractType::BuildingType:
            return NumConYards;
        default:
            return 0;
    }
}

// 0x0050B370: queried by the active sidebar when a techno is lost. Preserve
// positive (currently owned), nonpositive (ever produced), airport capacity
// and hijacker branches; this is also reached by ordinary map garrisons.
bool HouseClass::HasReachedBuildLimit(TechnoTypeClass* type) {
    if (!type) return true;
    FactoryClass* factory = nullptr;
    switch (type->WhatAmI()) {
        case AbstractType::Unit:
        case AbstractType::UnitType:
            factory = type->Naval ? Primary_ForShips : Primary_ForVehicles;
            break;
        case AbstractType::Aircraft:
        case AbstractType::AircraftType:
            factory = Primary_ForAircraft;
            break;
        case AbstractType::Building:
        case AbstractType::BuildingType:
            factory = Primary_ForBuildings;
            break;
        case AbstractType::Infantry:
        case AbstractType::InfantryType:
            factory = Primary_ForInfantry;
            break;
        default:
            break;
    }
    const int queued = factory ? factory->CountTotal(type) : 0;
    const auto add = [](int a, int b) { return std::bit_cast<int>(DWORD(a) + DWORD(b)); };
    const int limit = type->BuildLimit;
    const int absolute = limit < 0 ? std::bit_cast<int>(0u - DWORD(limit)) : limit;
    switch (type->WhatAmI()) {
        case AbstractType::AircraftType:
            if (static_cast<AircraftTypeClass*>(type)->AirportBound) {
                int active = 0, products = 0;
                for (auto* aircraft : RulesClass::Instance->PadAircraft) {
                    active = add(active, ActiveAircraftTypes.GetItemCount(aircraft->GetArrayIndex()));
                    if (factory) products = add(products, factory->CountTotal(aircraft));
                }
                return add(active, products) >= AirportDocks;
            }
            return add(queued, limit > 0 ? OwnedAircraftTypes.GetItemCount(type->GetArrayIndex())
                                         : FactoryProducedAircraftTypes.GetItemCount(type->GetArrayIndex())) >=
                   (limit > 0 ? limit : absolute);
        case AbstractType::BuildingType:
            return add(queued, limit > 0 ? OwnedBuildingTypes.GetItemCount(type->GetArrayIndex())
                                         : FactoryProducedBuildingTypes.GetItemCount(type->GetArrayIndex())) >=
                   (limit > 0 ? limit : absolute);
        case AbstractType::UnitType:
            return add(queued, limit > 0 ? OwnedUnitTypes.GetItemCount(type->GetArrayIndex())
                                         : FactoryProducedUnitTypes.GetItemCount(type->GetArrayIndex())) >=
                   (limit > 0 ? limit : absolute);
        case AbstractType::InfantryType: {
            if (limit <= 0 && add(queued, FactoryProducedInfantryTypes.GetItemCount(type->GetArrayIndex())) >= absolute)
                return true;
            int owned = OwnedInfantryTypes.GetItemCount(type->GetArrayIndex());
            if (static_cast<InfantryTypeClass*>(type)->VehicleThief) {
                for (auto* unit : UnitClass::Array)
                    if (unit->Owner == this && unit->HijackerInfantryType == type->GetArrayIndex())
                        owned = add(owned, 1);
            }
            return limit > 0 && add(queued, owned) >= limit;
        }
        default:
            return false;
    }
}

// YR 0x4FFA50 / 0x4FF980: map factory counts are separate from ownership.
namespace {
int* factory_count(HouseClass& house, TechnoClass* object) {
    if (object->WhatAmI() != AbstractType::Building) return nullptr;
    auto* type = static_cast<BuildingClass*>(object)->Type;
    switch (type->Factory) {
        case AbstractType::Aircraft:
        case AbstractType::AircraftType:
            return &house.NumAirpads;
        case AbstractType::Infantry:
        case AbstractType::InfantryType:
            return &house.NumBarracks;
        case AbstractType::Unit:
        case AbstractType::UnitType:
            return type->Naval ? &house.NumShipyards : &house.NumWarFactories;
        case AbstractType::Building:
        case AbstractType::BuildingType:
            return &house.NumConYards;
        default:
            return nullptr;
    }
}
}
void HouseClass::AddFactory(TechnoClass* object) {
    if (auto* count = factory_count(*this, object)) ++*count;
}
void HouseClass::RemoveFactory(TechnoClass* object) {
    if (auto* count = factory_count(*this, object)) --*count;
}

CellStruct* HouseClass::WhereToGo(CellStruct* output, FootClass* object) {
    const unsigned value = unsigned(object->GetAntiInfantryValue()) + unsigned(object->GetAntiArmorValue()) +
                           unsigned(object->GetAntiAirValue());
    const int zone = value ? ScenarioClass::Instance->Random.RandomRanged(1, 4) : 0;
    const auto around = PickRandomCellInZone(zone);
    const int movement = MapClass::Instance.GetMovementZoneType(object->GetMapCoords(), MovementZone::Normal, false);
    *output = MapClass::Instance.NearByLocation(around, SpeedType::Track, movement, MovementZone::Normal, false, 1, 1,
                                                false, false, false, true, CellStruct::Empty, false, false);
    return output;
}

CellStruct* HouseClass::PickRandomCellInZone(CellStruct& output, int zone) const {
    auto& map = MapClass::Instance;
    auto& random = ScenarioClass::Instance->Random;
    const int radius = std::clamp(Radius, 768, 2048);
    const auto center = GetBaseCenter();
    const auto origin = [&] {
        return center == CellStruct::Empty ? CoordStruct::Empty : map.GetCellAt(center)->GetCoords();
    };
    CoordStruct coordinate = CoordStruct::Empty;
    if (zone == 0)
        coordinate = map.GetRandomCoordsNear(origin(), random.RandomRanged(0, radius), true);
    else if (zone >= 1 && zone <= 4) {
        const int distance = random.RandomRanged(radius, 2 * radius);
        unsigned short direction = static_cast<unsigned short>(random.Random());
        direction = static_cast<unsigned short>((direction & 0x3FFFu) | ((zone - 1) * 0x4000u));
        int signedDirection;
        if (zone == 4) {
            signedDirection =
                int(direction & 0x1FFFu) ^
                (int(std::bit_cast<short>(static_cast<unsigned short>((direction & 0xE000u) - 1))) & ~0x1FFF);
        } else {
            direction = static_cast<unsigned short>(
                ((direction ^ static_cast<unsigned short>((direction & 0xE000u) - 1)) & 0xE000u) ^ direction);
            signedDirection = std::bit_cast<short>(direction);
        }
        const double angle = (signedDirection - 0x3FFF) * -0.00009587672516830327;
        const auto at = origin();
        coordinate.Y = game::x87_integer(double(at.Y) - Math::sin(angle) * distance);
        coordinate.X = game::x87_integer(Math::cos(angle) * distance + double(at.X));
        coordinate.Z = at.Z;
    }
    CellStruct cell;
    if (coordinate == CoordStruct::Empty) {
        if (zone)
            PickRandomCellInZone(cell, 0);
        else
            cell = center;
    } else
        cell = CellClass::Coord2Cell(coordinate);
    output = map.IsWithinUsableArea(cell, true) ? cell : *map.ClipToMap(&output, cell);
    return &output;
}

CoordStruct* HouseClass::GetBaseCenterCoords(CoordStruct* output) const {
    const auto& cell = GetBaseCenter();
    *output = cell == CellStruct::Empty ? CoordStruct::Empty : MapClass::Instance.GetCellAt(cell)->GetCoords();
    return output;
}

void HouseClass::MarkScouted(HouseClass* house) {
    for (int i = 0; i < ScoutNodes.Count; ++i)
        if (ScoutNodes[i].House == house) ScoutNodes[i].IsPreferred = true;
}
