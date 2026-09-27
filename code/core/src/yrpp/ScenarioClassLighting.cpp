// YR 4AE4C0 / 53C280 / 53AC80 / 555AC0 / 53AD00.
// Registries, virtual dispatch, effect precedence and update order are kept.
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/SuperClass.h"
#include "scenario_runtime.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace {
const game::ScenarioRenderServices& rendering() {
    const auto* services = game::scenario_runtime().render;
    if (!services) throw std::logic_error("Scenario lighting requires existing renderer objects");
    return *services;
}
int scale(int value) { return std::bit_cast<std::int32_t>(static_cast<DWORD>(value) * 1000u) / 100; }
}
void YRPP_FASTCALL ScenarioClass::ScenarioLighting(int* red, int* green, int* blue) {
    const auto& services = rendering();
    if (!services.quality) throw std::logic_error("Scenario lighting requires the renderer quality setting");
    const int quality = *services.quality;
    const int mask = quality == 0 ? ~127 : quality == 1 ? ~63 : quality == 2 ? ~31 : ~0;
    // Preserve pointer alias behavior: clamp all upper bounds, then lower
    // bounds, then quantize, in the same order as the original entry.
    *red = std::min(*red, 1000); *green = std::min(*green, 1000); *blue = std::min(*blue, 1000);
    *red = std::max(*red, 0); *green = std::max(*green, 0); *blue = std::max(*blue, 0);
    *red &= mask; *green &= mask; *blue &= mask;
}
void YRPP_FASTCALL ScenarioClass::UpdateCellLighting() {
    auto* map = rendering().map;
    if (!map) throw std::logic_error("Scenario lighting requires the existing map");
    map->CellIteratorReset();
    while (auto* cell = map->CellIteratorNext()) cell->UpdateCellLighting();
}
void YRPP_FASTCALL ScenarioClass::UpdateHashPalLighting(int red, int green, int blue, bool tint) {
    const auto& services = rendering();
    if (!services.next_palette || !services.palette_scheme_count)
        throw std::logic_error("Scenario lighting requires the palette registry");
    HashIterator iterator{0, 0, false};
    DynamicVectorClass<ColorScheme*>* schemes = nullptr;
    while (services.next_palette(services.context, &iterator, schemes)) {
        const int count = services.palette_scheme_count(services.context);
        for (int i = 0; i < count; ++i) schemes->Items[i]->LightConvert->UpdateColors(red, green, blue, tint);
    }
}
void YRPP_FASTCALL ScenarioClass::RecalcLighting(int red, int green, int blue, bool tint) {
    const auto& services = rendering();
    if (!services.light_converts || !services.color_schemes || !services.redraw_sidebar)
        throw std::logic_error("Scenario lighting requires the original light and color registries");
    for (int i = 0; i < services.light_converts->Count; ++i)
        services.light_converts->Items[i]->UpdateColors(red, green, blue, tint);
    for (int i = 0; i < services.color_schemes->Count; ++i) {
        const auto* scheme = services.color_schemes->Items[i];
        if (scheme->ShadeCount > 1) scheme->LightConvert->UpdateColors(red, green, blue, tint);
    }
    UpdateHashPalLighting(red, green, blue, tint);
    UpdateCellLighting();
    services.redraw_sidebar(services.context, 1);
}
void YRPP_FASTCALL ScenarioClass::UpdateLighting() {
    auto* scenario = Instance;
    if (!scenario) throw std::logic_error("Lighting update requires the active Scenario");
    const LightingStruct* lighting = nullptr;
    if (NukeFlash::IsFadingIn() || ChronoScreenEffect::Active()) {
        scenario->AmbientTarget = scenario->NukeAmbient;
        lighting = &scenario->NukeLighting;
    } else if (LightningStorm::Active) {
        scenario->AmbientTarget = scenario->IonAmbient;
        lighting = &scenario->IonLighting;
    } else if (PsyDom::Status != PsychicDominatorStatus::Inactive && PsyDom::Status != PsychicDominatorStatus::Over) {
        scenario->AmbientTarget = scenario->DominatorAmbient;
        lighting = &scenario->DominatorLighting;
    } else {
        scenario->AmbientTarget = scenario->AmbientOriginal;
    }
    if (lighting) RecalcLighting(scale(lighting->Tint.Red), scale(lighting->Tint.Green), scale(lighting->Tint.Blue), true);
    else RecalcLighting(-1, -1, -1, false);
}
