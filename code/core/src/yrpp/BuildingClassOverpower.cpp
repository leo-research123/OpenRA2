// YR 0x452820 and the charging phase of BuildingClass::Update.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"

void BuildingClass::AddOverpowerer(InfantryClass* infantry) {
    // Refresh the order as well as removing duplicates.
    Overpowerers.Remove(infantry);
    try {
        Overpowerers.AddItem(infantry);
    } catch (const std::bad_alloc&) {
        Overpowerers.IsInitialized = true;
    }
}
void BuildingClass::UpdateOverpowerers() {
    if (!Type->Overpowerable) return;
    for (int i = Overpowerers.Count - 1; i >= 0; --i)
        if (Overpowerers[i]->Target != this) Overpowerers.RemoveItem(i);
    const int count = Overpowerers.Count;
    IsOverpowered = count >= 3 || (Owner->GetPowerPercentage() == 1.0 && HasPower && count >= 1);
}
