// YR 0x00459ED0, the original UI-name virtual slot.
#include "yrpp/BuildingClass.h"
const wchar_t* BuildingClass::GetUIName() const { return Type?Type->UIName:nullptr; }
