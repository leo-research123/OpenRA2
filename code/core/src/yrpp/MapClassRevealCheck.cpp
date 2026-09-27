// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 map.cpp Reveal_Nearby_Technos; YR 0x005865F0.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/TechnoClass.h"
void YRPP_STDCALL MapClass::RevealCheck(CellClass* start,HouseClass* house,bool newlyMapped) {
    auto at=start->MapCoords;
    for(int level=1;level<15;level+=2) {
        auto* cell=Instance.GetCellAt(at);
        if(cell->Level>=level-2 && cell->Level<=level) {
            if(newlyMapped)RadarClass::Instance.RadarCell(cell->MapCoords);
            if(auto* object=cell->FindTechnoNearestTo({0,0},false,nullptr))object->DiscoveredBy(house);
        }
        at.X=short(at.X+1);at.Y=short(at.Y+1);
    }
}
