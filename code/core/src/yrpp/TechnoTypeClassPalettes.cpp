// Original all-type palette attachment, 0x00717840.
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/ColorScheme.h"

#if !defined(RA2_YRPP_GAME)
int TechnoTypeClass::ReloadPalettes() noexcept {
    for (int i = 0; i < Array.Count; ++i) {
        auto* type = Array[i];
        if (*type->PaletteFile) type->Palette = ColorScheme::GeneratePalette(type->PaletteFile);
    }
    return Array.Count;
}
#endif
