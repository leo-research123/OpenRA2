// Original YR action 131, 0x6E4560. Script counts bypass click/disguise feedback.
#include "yrpp/TActionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingClass.h"
#include <cstring>

bool TActionClass::FlashBuildingsOfType(HouseClass* house, ObjectClass*, TriggerClass*, const CellStruct&) {
    if (!house) return false;
    for (int i = house->Buildings.Count - 1; i >= 0; --i) {
        auto* building = house->Buildings[i];
        if (!std::strcmp(building->Type->ID, TechnoID)) building->Flashing.DurationRemaining = Value;
    }
    return true;
}
