// TechnoClass::GetRemapColour, YR 0x00705D70.
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"

#if !defined(RA2_YRPP_GAME)
LightConvertClass* TechnoClass::GetRemapColour() const {
    auto* palettes = GetTechnoType()->Palette;
    const int index = Owner->ColorSchemeIndex;
    if (palettes) return palettes->Items[index]->LightConvert;
    if (IsClearlyVisibleTo(HouseClass::CurrentPlayer) || !GetDisguiseHouse(true))
        return ColorScheme::Array.Items[index]->LightConvert;
    return ColorScheme::Array.Items[GetDisguiseHouse(true)->ColorSchemeIndex]->LightConvert;
}
#endif
