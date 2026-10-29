// Original Theater::Init palette phase, 0x00534D31..0x00534DBE.
// Resource mounting and progress/device services are handled by the caller.
#include "yrpp/Theater.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/TechnoTypeClass.h"
#include "type_resources.hpp"

#if !defined(RA2_YRPP_GAME)
bool Theater::InitPaletteConverters(TheaterType theater, const BytePalette& unit, const BytePalette& screen) noexcept {
    if (theater < TheaterType::Temperate || theater > TheaterType::Lunar) {
        game::type_resource_failure("Theater");
        return false;
    }
    for (int i = 0; i < ColorScheme::Array.Count; ++i) {
        if (!ColorScheme::Array[i]->UpdatePalette(unit, screen, 1000, 1000, 1000)) return false;
    }
    ColorScheme::SetPaletteTheater(theater);
    // SetPaletteTheater can retire vectors still borrowed by the types. Rebind
    // every type before returning, even if an earlier cache allocation failed.
    TechnoTypeClass::ReloadPalettes();
    return game::last_type_resource_status() != game::TypeResourceStatus::failure;
}
#endif
