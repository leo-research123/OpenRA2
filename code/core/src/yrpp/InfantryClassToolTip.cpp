// YR 0x0051F2C0. Technician precedes disguise; allied spies show their real name.
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/StringTable.h"
const wchar_t* InfantryClass::GetUIName() const {
    if (Technician) return StringTable::LoadString("TXT_TECHNICIAN");
    const auto* type =
        IsDisguised() && !(Owner && Owner->IsAlliedWith(HouseClass::CurrentPlayer)) ? GetDisguise(true) : Type;
    return type ? type->UIName : nullptr;
}
