// YRpp 9402d7da; only exact trivial bodies from the supplied function exports.
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "RulesClassReaders.hpp"
#include "yrpp/RulesClass.h"

// OpenTS techtype.cpp Max_Pips; YR removes the ammo/passenger cap and
// replaces Charge with the primary mind-control weapon's capacity.
int TechnoTypeClass::GetPipMax() const {
 switch(PipScale){
 case PipScale::Ammo:return Ammo;
 case PipScale::Tiberium:return WhatAmI()==AbstractType::InfantryType?3:5;
 case PipScale::Passengers:return Passengers;
 case PipScale::Power:return 10;
 case PipScale::MindControl:return Weapon[0].WeaponType?Weapon[0].WeaponType->Damage:0;
 default:return 0;
 }
}

// 0x00711EE0: the base production duration before owner/power multipliers.
int TechnoTypeClass::GetBuildSpeed() const {
 return rule_integer(double(Cost)*RulesClass::Instance->BuildSpeed*0.9);
}

// 0x00711E80: confirmed constant return, not an unimplemented stub.
bool TechnoTypeClass::CanUseWaypoint() const { return true; }

// OpenTS techtype.cpp::Cost_Of, YR 0x711F00 / 0x50BDF0 / 0x50BEB0.
int TechnoTypeClass::GetActualCost(HouseClass* house) const {
 if(!house)return GetCost();
 float country=1.0f,production=1.0f;
 switch(WhatAmI()){
 case AbstractType::InfantryType:country=house->Type->CostInfantryMult;production=house->CostInfantryMult;break;
 case AbstractType::UnitType:country=house->Type->CostUnitsMult;production=house->CostUnitsMult;break;
 case AbstractType::AircraftType:country=house->Type->CostAircraftMult;production=house->CostAircraftMult;break;
 case AbstractType::BuildingType:
  if(static_cast<const BuildingTypeClass*>(this)->BuildCat==BuildCat::Combat){country=house->Type->CostDefensesMult;production=house->CostDefensesMult;}
  else {country=house->Type->CostBuildingsMult;production=house->CostBuildingsMult;}
  break;
 default:break;
 }
 return rule_integer(double(GetCost())*production*country);
}

// OpenTS Repair_Cost/Repair_Step/Refund_Amount, YR 0x7120D0/0x712120/0x711F60.
#if !defined(RA2_YRPP_GAME)
int TechnoTypeClass::GetRepairStep() const {return RulesClass::Instance->RepairStep;}
int TechnoTypeClass::GetRepairStepCost() const {
 const int step=RulesClass::Instance->RepairStep;
 // Invalid mod data must not make the host divide by zero.
 const int installments=step>0?Strength/step:0;
 return installments>0?std::max(1,rule_integer(double(GetCost()/installments)*RulesClass::Instance->RepairPercent)):1;
}
int TechnoTypeClass::GetRefund(HouseClass* house,bool full) const {
 const float percent=full?1.0f:float(RulesClass::Instance->RefundPercent);
 if(!house)return rule_integer(double(GetCost())*percent);
 float country=1.0f;
 switch(WhatAmI()) {
 case AbstractType::InfantryType:country=house->Type->CostInfantryMult;break;
 case AbstractType::UnitType:country=house->Type->CostUnitsMult;break;
 case AbstractType::AircraftType:country=house->Type->CostAircraftMult;break;
 case AbstractType::BuildingType:country=static_cast<const BuildingTypeClass*>(this)->BuildCat==BuildCat::Combat?
     house->Type->CostDefensesMult:house->Type->CostBuildingsMult;break;
 default:break;
 }
 if(Soylent)return rule_integer(double(Soylent)*country);
 const int cost=TechnoTypeClass::GetActualCost(house);
 return house->IsControlledByHuman()?rule_integer(double(cost)*percent):cost;
}
#endif
