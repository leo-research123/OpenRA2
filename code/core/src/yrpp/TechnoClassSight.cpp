// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Look; YR 0x0070ADC0.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/SessionClass.h"
#include "scenario_runtime.hpp"
#include "x87_integer.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
// The original _ftol leaves 53-bit/toward-zero arithmetic active. Respect
// the caller's mode before the first conversion, then retain its side effect.
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif
void TechnoClass::See(DWORD incremental,DWORD dontMap) {
    if(!IsInPlayfield)return;
    if(Owner->Type->MultiplayPassive) {
#if defined(RA2_YRPP_GAME)
        if(!SessionClass::IsCampaign())return;
#else
        const auto* houses=game::scenario_runtime().houses;
        if(houses && houses->session && houses->session->GameMode!=GameMode::Campaign)return;
#endif
    }
    const int increase=std::bit_cast<int>(10u*unsigned(Location.Z/RulesClass::Instance->LeptonsPerSightIncrease));
    if(increase>SightIncrease)incremental=0;
    SightIncrease=std::bit_cast<char>(static_cast<unsigned char>(increase));
    const auto* type=GetTechnoType();
    // YR's veteran factor is a multiplier, unlike OpenTS's factor + 1.
    const double percent=double(SightIncrease)*0.01;
    int sight=game::x87_integer(double(type->Sight)*(percent+1.0));
    if(Veterancy.IsVeteran() || Veterancy.IsElite()) {
        type=GetTechnoType();
        if((Veterancy.IsVeteran() && type->VeteranAbilities.SIGHT) ||
           (Veterancy.IsElite() && (type->VeteranAbilities.SIGHT || type->EliteAbilities.SIGHT))) {
            const double factor=RulesClass::Instance->VeteranSight;
            // Original x87 unordered comparison skips a NaN factor as well.
            if(factor!=0.0 && !std::isnan(factor))sight=game::x87_integer(double(sight)*factor);
        }
    }
    if(sight) {
        auto position=Location;
        MapClass::Instance.RevealArea1(&position,sight,Owner,BYTE(incremental),BYTE(dontMap),0,1,1);
    }
}

// YR's paired sight entry 0x0070AF50. The cached coordinates/radius belong
// to the first acquisition; override-house acquisitions do not replace them.
void TechnoClass::UpdateSight(bool incremental,int unused,bool useHouse,HouseClass* house,int sightOverride) {
    if(!IsInPlayfield || Owner->Type->MultiplayPassive)return;
    if(!GetTechnoType()->Sight && !sightOverride)return;
    const int increase=std::bit_cast<int>(10u*unsigned(Location.Z/RulesClass::Instance->LeptonsPerSightIncrease));
    if(increase>SightIncrease)incremental=false;
    SightIncrease=std::bit_cast<char>(static_cast<unsigned char>(increase));
    const auto* type=GetTechnoType();
    const double percent=double(SightIncrease)*0.01;
    int sight=game::x87_integer(double(type->Sight)*(percent+1.0));
    if(Veterancy.IsVeteran() || Veterancy.IsElite()) {
        type=GetTechnoType();
        if((Veterancy.IsVeteran() && type->VeteranAbilities.SIGHT) ||
           (Veterancy.IsElite() && (type->VeteranAbilities.SIGHT || type->EliteAbilities.SIGHT))) {
            const double factor=RulesClass::Instance->VeteranSight;
            if(factor!=0.0 && !std::isnan(factor))sight=game::x87_integer(double(sight)*factor);
        }
    }
    if(sightOverride)sight=sightOverride;
    if(unknown_bool_250) {
        if(!useHouse || !house || !sight)return;
    }else {
        unknown_bool_250=true;LastSightCoords=Location;LastSightRange=sight;
        if(!sight)return;
    }
    auto at=Location;
    MapClass::Instance.RevealArea2(&at,sight,useHouse&&house?house:Owner,incremental,unused,0,1,0);
}

// Paired release, YR 0x0070B1D0; always uses the saved sight coordinates.
void TechnoClass::vt_entry_48C(bool keep,int unknown,bool useHouse,HouseClass* house) {
 if(!IsInPlayfield || Owner->Type->MultiplayPassive)return;
 if(unknown_bool_250) {
  unknown_bool_250=false;
  if(LastSightRange)MapClass::Instance.RevealArea2(&LastSightCoords,LastSightRange,
      useHouse&&house?house:Owner,keep,unknown,false,true,true);
 }else if(useHouse && house && LastSightRange)
  MapClass::Instance.RevealArea2(&LastSightCoords,LastSightRange,house,keep,unknown,false,true,true);
}
