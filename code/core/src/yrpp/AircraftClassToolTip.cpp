// YR 0x0041C1D0, the original UI-name virtual slot.
#include "yrpp/AircraftClass.h"
const wchar_t* AircraftClass::GetUIName() const { return Type?Type->UIName:nullptr; }
