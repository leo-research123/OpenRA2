// Original delayed-deletion traversal, 0x725C70.
#include "yrpp/AbstractClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include <typeinfo>

void YRPP_FASTCALL AbstractClass::RemoveAllInactive() {
    int index = 0;
    while (index < PendingDeletes.Count) {
        auto* object = PendingDeletes[index];
        if (!object->IsDead()) { ++index; continue; }
        // Remove every duplicate before Release/destruction can mutate the list.
        while (PendingDeletes.Remove(object)) {}
        if (!object->Release()) continue;
        const auto& type = typeid(*object);
        if (type == typeid(BuildingClass) || type == typeid(UnitClass) ||
            type == typeid(InfantryClass) || type == typeid(AircraftClass))
            static_cast<ObjectClass*>(object)->IsAlive = true;
        delete object;
    }
}
