// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Tracking_Add/Tracking_Remove; YR 0x502A80/0x5025F0.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/Unsorted.h"
#include "RulesClassReaders.hpp"
#include <algorithm>

FactoryClass* HouseClass::GetPrimaryFactory(AbstractType kind,bool naval,BuildCat category) const {
    switch(kind) {
    case AbstractType::Unit:case AbstractType::UnitType:return naval?Primary_ForShips:Primary_ForVehicles;
    case AbstractType::Aircraft:case AbstractType::AircraftType:return Primary_ForAircraft;
    case AbstractType::Infantry:case AbstractType::InfantryType:return Primary_ForInfantry;
    case AbstractType::Building:case AbstractType::BuildingType:return category==BuildCat::Combat?Primary_ForDefenses:Primary_ForBuildings;
    default:return nullptr;
    }
}

// OpenTS Tracking_Add/Remove (total ownership), YR 0x4FF700/0x4FF550.
void HouseClass::AddTracking(TechnoClass* object) {
 object->CountedAsOwned=true;auto* type=object->GetTechnoType();
 if(type->Insignificant||type->DontScore)return;
 const int index=type->GetArrayIndex();UnitTrackerClass* built=nullptr;
 switch(object->WhatAmI()){
 case AbstractType::Infantry:
  if(!object->CountedAsOwnedSpecial&&!object->Absorbed){++OwnedInfantry;object->CountedAsOwnedSpecial=true;}
  ActiveInfantryTypes.Increment(index);built=&BuiltInfantryTypes;break;
 case AbstractType::Unit:
  ++OwnedUnits;if(type->Naval&&!type->Passengers)++OwnedNavy;
  ActiveUnitTypes.Increment(index);built=&BuiltUnitTypes;break;
 case AbstractType::Aircraft:++OwnedAircraft;ActiveAircraftTypes.Increment(index);built=&BuiltAircraftTypes;break;
 case AbstractType::Building:{
  auto* building=static_cast<BuildingClass*>(object);
  if(building->IsStrange()||(building->Type->UndeploysInto&&building->Type->UndeploysInto->Harvester))++OwnedUnits;else ++OwnedBuildings;
  ActiveBuildingTypes.Increment(index);built=&BuiltBuildingTypes;break;
 }
 default:break;
 }
 if(built&&index>=0&&index<built->UnitCount)++built->UnitTotals[index];
}
void HouseClass::RemoveTracking(TechnoClass* object) {
 object->CountedAsOwned=false;auto* type=object->GetTechnoType();
 if(type->Insignificant||type->DontScore)return;
 const int index=type->GetArrayIndex();
 switch(object->WhatAmI()){
 case AbstractType::Infantry:
  if(object->CountedAsOwnedSpecial){--OwnedInfantry;object->CountedAsOwnedSpecial=false;}
  ActiveInfantryTypes.Decrement(index);break;
 case AbstractType::Unit:
  --OwnedUnits;if(type->Naval&&!type->Passengers)--OwnedNavy;ActiveUnitTypes.Decrement(index);break;
 case AbstractType::Aircraft:--OwnedAircraft;ActiveAircraftTypes.Decrement(index);break;
 case AbstractType::Building:{
  auto* building=static_cast<BuildingClass*>(object);
  if(building->IsStrange()||(building->Type->UndeploysInto&&building->Type->UndeploysInto->Harvester))--OwnedUnits;else --OwnedBuildings;
  ActiveBuildingTypes.Decrement(index);break;
 }
 default:break;
 }
}

void HouseClass::RegisterGain(TechnoClass* object,bool) {
    auto* type=object->GetTechnoType();const int index=type->GetArrayIndex();
    if(type->ResourceGatherer)++CountResourceGatherers;
    if(type->ResourceDestination)++CountResourceDestinations;
    switch(object->WhatAmI()) {
    case AbstractType::Unit:
        (type->ConsideredAircraft||type->Spawns?TotalOwnedAircraftCost:TotalOwnedVehicleCost)+=type->GetActualCost(this);
        OwnedUnitTypes.Increment(index);break;
    case AbstractType::Aircraft:
        TotalOwnedAircraftCost+=type->GetActualCost(this);
        if(!type->DontScore)OwnedAircraftTypes.Increment(index);break;
    case AbstractType::Infantry:
        (type->ConsideredAircraft?TotalOwnedAircraftCost:TotalOwnedInfantryCost)+=type->GetActualCost(this);
        if(!static_cast<InfantryClass*>(object)->Technician&&!type->DontScore)OwnedInfantryTypes.Increment(index);break;
    case AbstractType::Building: {
        if(!type->DontScore)OwnedBuildingTypes.Increment(index);
        auto* building=static_cast<BuildingClass*>(object);
        if(building->Type->WeaponsFactory&&!type->Naval)++CountWarfactories;
        if(building->Type->Factory==AbstractType::InfantryType)++NumBarracks;
        if(building->Type->Factory==AbstractType::UnitType)++(type->Naval?NumShipyards:NumWarFactories);
        RecheckTechTree=true;
        AddPowerDrain(building->GetPowerDrain());TotalStorage+=type->Storage;
        OwnedTiberium.Tiberium1+=object->Tiberium.Tiberium1;OwnedTiberium.Tiberium2+=object->Tiberium.Tiberium2;
        OwnedTiberium.Tiberium3+=object->Tiberium.Tiberium3;OwnedTiberium.Tiberium4+=object->Tiberium.Tiberium4;
        break;
    }
    default:break;
    }
}
void HouseClass::RegisterLoss(TechnoClass* object,bool keepTiberium) {
    auto* type=object->GetTechnoType();const int index=type->GetArrayIndex();
    if(type->ResourceGatherer)--CountResourceGatherers;
    if(type->ResourceDestination)--CountResourceDestinations;
    SidebarClass::Instance.OnTechnoDestroyed(object);
    if(type->PowersUnit && object->WhatAmI()==AbstractType::Building) {
        auto* building=static_cast<BuildingClass*>(object);
        if(building->RegisteredAsPoweredUnitSource) {
            building->RegisteredAsPoweredUnitSource=false;
            if(!--PoweredUnitCenters) {
                bool changed=false;
                for(int i=TechnoClass::Array.Count-1;i>=0;--i) {
                    auto* unit=TechnoClass::Array[i];
                    if(unit->Owner==this&&unit->GetTechnoType()==type->PowersUnit&&!unit->InLimbo){unit->Deactivate();changed=true;}
                }
                if(changed&&Game::IsActive&&IsControlledByCurrentPlayer()&&!Unsorted::ScenarioInit)VoxClass::Play("EVA_RobotTanksOffline");
            }
        }
    }
    switch(object->WhatAmI()) {
    case AbstractType::Unit:
        (type->ConsideredAircraft||type->Spawns?TotalOwnedAircraftCost:TotalOwnedVehicleCost)-=type->GetActualCost(this);
        if(!type->DontScore)OwnedUnitTypes.Decrement(index);break;
    case AbstractType::Aircraft:
        TotalOwnedAircraftCost-=type->GetActualCost(this);
        if(!type->DontScore)OwnedAircraftTypes.Decrement(index);break;
    case AbstractType::Infantry:
        (type->ConsideredAircraft?TotalOwnedAircraftCost:TotalOwnedInfantryCost)-=type->GetActualCost(this);
        if(!type->DontScore)OwnedInfantryTypes.Decrement(index);break;
    case AbstractType::Building: {
        if(!type->DontScore)OwnedBuildingTypes.Decrement(index);
        auto* building=static_cast<BuildingClass*>(object);
        if(building->Type->WeaponsFactory&&!type->Naval)--CountWarfactories;
        if(building->Type->Factory==AbstractType::InfantryType)--NumBarracks;
        if(building->Type->Factory==AbstractType::UnitType)--(type->Naval?NumShipyards:NumWarFactories);
        RecheckTechTree=true;
        RecheckPower=true;TotalStorage-=type->Storage;
        if(keepTiberium) {
            float* stores[]{&object->Tiberium.Tiberium1,&object->Tiberium.Tiberium2,&object->Tiberium.Tiberium3,&object->Tiberium.Tiberium4};
            for(int i=0;i<4;++i)while(*stores[i]>0) {
                const float removed=std::min(*stores[i],2147483648.0f);*stores[i]-=removed;
                const int amount=rule_integer(removed);
                PointTotal=rule_integer(double(PointTotal)+amount*5.0);
                Balance=rule_integer(double(Balance)+amount*double(TiberiumClass::Array[i]->Value)*Type->IncomeMult);
            }
        }else {
            OwnedTiberium.Tiberium1-=object->Tiberium.Tiberium1;OwnedTiberium.Tiberium2-=object->Tiberium.Tiberium2;
            OwnedTiberium.Tiberium3-=object->Tiberium.Tiberium3;OwnedTiberium.Tiberium4-=object->Tiberium.Tiberium4;
        }
        break;
    }
    default:break;
    }
}
