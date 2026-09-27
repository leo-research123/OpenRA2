// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 house.cpp Spend_Money/Refund_Money; YR 0x4F6990/0x4F9790/0x4F9950.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/HouseClass.h"
#include "yrpp/TiberiumClass.h"
#include "RulesClassReaders.hpp"
#include <algorithm>
long YRPP_STDCALL HouseClass::Available_Money() const {
 int ore=0;const float stores[]{OwnedTiberium.Tiberium1,OwnedTiberium.Tiberium2,OwnedTiberium.Tiberium3,OwnedTiberium.Tiberium4};
 for(int i=0;i<4&&i<TiberiumClass::Array.Count;++i)if(stores[i]>0)ore=rule_integer(double(ore)+double(TiberiumClass::Array[i]->Value)*stores[i]);
 return rule_integer(double(ore)*Type->IncomeMult+Balance);
}
void HouseClass::GiveMoney(int amount){Balance=std::bit_cast<int>(DWORD(Balance)+DWORD(amount));}
// YR 0x4F9610 converts delivered ore straight into cash; the TS silo-storage
// path is not used here. Score advances by five points per delivered bail.
void HouseClass::GiveTiberium(float amount,int type){
 auto* resource=TiberiumClass::Array.GetItemOrDefault(type);if(!resource)return;
 PointTotal=rule_integer(double(amount)*5.0+PointTotal);
 Balance=rule_integer(double(resource->Value)*Type->IncomeMult*amount+Balance);
}
void HouseClass::TakeMoney(int amount){
 int spent=amount;
 if(amount<=Balance)Balance-=amount;
 else{
  int remaining=amount-Balance;spent=Balance;Balance=0;
  float* total[]{&OwnedTiberium.Tiberium1,&OwnedTiberium.Tiberium2,&OwnedTiberium.Tiberium3,&OwnedTiberium.Tiberium4};
  for(auto* building:Buildings){
   float* stores[]{&building->Tiberium.Tiberium1,&building->Tiberium.Tiberium2,&building->Tiberium.Tiberium3,&building->Tiberium.Tiberium4};
   for(int i=0;i<4&&i<TiberiumClass::Array.Count;++i)while(remaining>0&&*stores[i]>0){
    const float removed=std::min(*stores[i],1.0f);*stores[i]-=removed;*total[i]-=std::min(*total[i],removed);
    const int value=rule_integer(double(TiberiumClass::Array[i]->Value)*Type->IncomeMult*removed);
    remaining-=value;spent+=value;
    if(remaining<0){Balance-=remaining;spent+=remaining;remaining=0;}
   }
   if(remaining<=0)break;
  }
 }
 CreditsSpent=std::bit_cast<int>(DWORD(CreditsSpent)+DWORD(spent));
}
