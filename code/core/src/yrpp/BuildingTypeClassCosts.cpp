// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 builtype.cpp Raw_Cost / Cost_Of; YR 0x45ED50/0x45EDD0.
// Copyright Electronic Arts Inc. / OpenTS contributors; EA Section 7 terms:
// code/third_party/opents/LICENSE.md. YR averages the first two pad aircraft.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/RulesClass.h"
#include <algorithm>
namespace {
bool bundled_aircraft(const BuildingTypeClass* type) {
 const auto& rules=*RulesClass::Instance;
 return !rules.SeparateAircraft && rules.PadAircraft.Count>=2 && rules.PadAircraft[0]
     && rules.PadAircraft[1] && rules.PadAircraft[0]->Dock.Count
     && rules.PadAircraft[0]->Dock[0]==type;
}
}
int BuildingTypeClass::GetCost() const {
 int cost=TechnoTypeClass::GetCost();
 if(bundled_aircraft(this)) {
  const auto& aircraft=RulesClass::Instance->PadAircraft;
  cost-=(aircraft[0]->GetCost()+aircraft[1]->GetCost())/2;
 }
 return FreeUnit?std::max(0,cost-FreeUnit->GetCost()):cost;
}
int BuildingTypeClass::GetActualCost(HouseClass* house) const {
 int cost=TechnoTypeClass::GetActualCost(house);
 if(bundled_aircraft(this)) {
  const auto& aircraft=RulesClass::Instance->PadAircraft;
  cost+=(aircraft[0]->GetActualCost(house)+aircraft[1]->GetActualCost(house))/2;
 }
 return FreeUnit?std::max(0,cost+FreeUnit->GetActualCost(house)):cost;
}
