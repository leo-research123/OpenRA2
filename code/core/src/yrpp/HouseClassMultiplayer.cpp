#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
UnitTypeClass* HouseClass::FirstOwnedUnitFromArray(const DynamicVectorClass<UnitTypeClass*>& items) {
    const unsigned owners = 1u << (unsigned(Type->FindParentCountryIndex()) & 31u);
    for (auto* type : items) if (type->OwnerFlags & owners) return type;
    return nullptr;
}
void HouseClass::SDDTORAllAndTriggers() {
    for (int i = 0; i < TechnoClass::Array.Count; ++i)
        if (TechnoClass::Array[i]->Owner == this) { delete TechnoClass::Array[i]; --i; }
    for (int i = 0; i < TriggerClass::Array.Count; ++i)
        if (TriggerClass::Array[i]->Type->House == Type) { delete TriggerClass::Array[i]; --i; }
    delete this;
}

void HouseClass::UpdateFlagCoords(UnitClass* carrier, DWORD clearCell) {
    if (!carrier) return;
    auto* object = static_cast<AbstractClass*>(carrier);
    bool removed = false;
    if (object->WhatAmI() == AbstractType::Unit) removed = carrier->FlagRemove();
    else {
        const auto coords = object->GetCoords();
        const CellStruct at{short(coords.X / 256), short(coords.Y / 256)};
        if (MapClass::Instance.CoordinatesLegal(at)) removed = MapClass::Instance.GetCellAt(at)->FlagRemove();
    }
    if (removed && OurFlagCarrier == carrier) OurFlagCarrier = nullptr;
    if ((clearCell & 0xFF) && OurFlagCoords != CellStruct{0, 0}) {
        MapClass::Instance.GetCellAt(OurFlagCoords)->OverlayTypeIndex = -1;
        OurFlagCoords = {0, 0};
    }
}
char HouseClass::PickedUpFlag(UnitClass* unit, DWORD clearCell) {
    if (!unit || unit->InLimbo) return false;
    UpdateFlagCoords(OurFlagCarrier, clearCell);
    unit->FlagAttach(Type->ArrayIndex2);
    OurFlagCarrier = unit;
    return true;
}
