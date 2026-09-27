// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 aircraft.cpp Look; YR 0x0041ADF0 uses paired counters.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
#include "yrpp/AircraftClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include <bit>
void AircraftClass::See(DWORD incremental,DWORD dontMap) {
    int sight=Type->Sight;
    if(!GetHeight())sight=1;
    if(sight) {
        auto at=Location;
        MapClass::Instance.RevealArea2(&at,sight,Owner,BYTE(incremental),std::bit_cast<int>(dontMap),0,1,0);
        at=Location;
        MapClass::Instance.RevealArea2(&at,sight,Owner,BYTE(incremental),std::bit_cast<int>(dontMap),0,1,1);
    }else if(ScenarioClass::Instance->SpecialFlags.FogOfWar) {
        sight=RulesClass::Instance->AircraftFogReveal;
        auto at=Location;
        MapClass::Instance.RevealArea2(&at,sight,Owner,0,0,1,GetHeight()<RulesClass::Instance->FlightLevel/2,0);
        at=Location;
        MapClass::Instance.RevealArea2(&at,sight,Owner,0,0,1,GetHeight()<RulesClass::Instance->FlightLevel/2,1);
    }
}
