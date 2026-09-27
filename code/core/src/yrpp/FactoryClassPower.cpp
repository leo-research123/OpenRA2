// YR 0x004CA6E0. Refresh the stage rate without restarting its timer.
#include "yrpp/FactoryClass.h"
#include "yrpp/TechnoClass.h"
#include <algorithm>

namespace { DynamicVectorClass<FactoryClass*> factories; }
DynamicVectorClass<FactoryClass*>& FactoryClass::Array=factories;

void FactoryClass::UpdateBuildSpeed(HouseClass* owner) {
    for(int i=0;i<Array.Count;++i) {
        auto* factory=Array[i];
        if(factory->Owner==owner) {
            const int rate=std::clamp((factory->Object?factory->Object->TimeToBuild():0)/54,1,255);
            if(rate!=factory->Production.Rate)factory->Production.Rate=rate;
        }
    }
}
