// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 htype.cpp Read_INI; YR 0x511850 country/side additions.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "RulesClassReaders.hpp"

bool HouseTypeClass::LoadFromINI(CCINIClass* ini){
 try{
  if(!ini||!AbstractTypeClass::LoadFromINI(ini))return false;
  char value[128]{};
  if(ini->ReadString(ID,"Suffix","",value,4))std::strcpy(Suffix,value);
  ini->ReadString(ID,"ParentCountry",ID,value,25);ParentCountry=value;
  if(ini->ReadString(ID,"Color","",value,sizeof(value)))if(auto* colors=game::rules_runtime().color_schemes){
   for(int i=0;i<colors->Count;++i)if(auto* color=(*colors)[i];color&&color->ShadeCount==1&&!_strcmpi(color->ID,value)){ColorSchemeIndex=i;break;}
  }
  char prefix[]{Prefix,0};ini->ReadString(ID,"Prefix",prefix,value,2);Prefix=value[0];
  FirepowerMult=ini->ReadDouble(ID,"Firepower",FirepowerMult);
  GroundspeedMult=ini->ReadDouble(ID,"Groundspeed",GroundspeedMult);
  AirspeedMult=ini->ReadDouble(ID,"Airspeed",AirspeedMult);
  ArmorMult=ini->ReadDouble(ID,"Armor",ArmorMult);ROFMult=ini->ReadDouble(ID,"ROF",ROFMult);
  CostMult=ini->ReadDouble(ID,"Cost",CostMult);BuildtimeMult=ini->ReadDouble(ID,"BuildTime",BuildtimeMult);
#define B(f) f=ini->ReadBool(ID,#f,f)
  B(Multiplay);B(MultiplayPassive);B(WallOwner);B(SmartAI);
#undef B
#define F(f) f=float(ini->ReadDouble(ID,#f,f))
  F(ArmorInfantryMult);F(ArmorUnitsMult);F(ArmorAircraftMult);F(ArmorBuildingsMult);F(ArmorDefensesMult);
  F(CostInfantryMult);F(CostUnitsMult);F(CostAircraftMult);F(CostBuildingsMult);F(CostDefensesMult);
  F(SpeedInfantryMult);F(SpeedUnitsMult);F(SpeedAircraftMult);
  F(BuildtimeInfantryMult);F(BuildtimeUnitsMult);F(BuildtimeAircraftMult);F(BuildtimeBuildingsMult);F(BuildtimeDefensesMult);F(IncomeMult);
#undef F
  read_rule_type_list(*ini,ID,"VeteranInfantry",AbstractType::InfantryType,VeteranInfantry);
  read_rule_type_list(*ini,ID,"VeteranUnits",AbstractType::UnitType,VeteranUnits);
  read_rule_type_list(*ini,ID,"VeteranAircraft",AbstractType::AircraftType,VeteranAircraft);
  if(ini->ReadString(ID,"Side","",value,sizeof(value))){
   const int next=SideClass::FindIndex(value),previous=SideIndex;SideIndex=next;
   if(next!=previous){
    if(auto* side=SideClass::Array.GetItemOrDefault(previous))side->HouseTypes.RemoveItem(ArrayIndex2);
    if(auto* side=SideClass::Array.GetItemOrDefault(next))side->HouseTypes.AddItem(ArrayIndex2);
   }
  }
  return true;
 }catch(...){return false;}
}
