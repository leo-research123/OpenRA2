// YR 484680. Cell intensities use wrapping 16-bit intermediates and signed
// clamps; the ambient product is 32-bit before division.
#include "yrpp/CellClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SuperClass.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
namespace {
short narrow(int value) { return std::bit_cast<short>(static_cast<unsigned short>(value)); }
int wrapped_product(int a, int b) { return std::bit_cast<std::int32_t>(static_cast<DWORD>(a) * static_cast<DWORD>(b)); }
}
void CellClass::UpdateCellLighting() {
    const auto* scenario = ScenarioClass::Instance;
    if (!scenario) throw std::logic_error("Cell lighting requires the active Scenario");
    const LightingStruct& lighting = LightningStorm::Active ? scenario->IonLighting :
        PsyDom::Active() ? scenario->DominatorLighting :
        NukeFlash::IsFadingIn() ? scenario->NukeLighting : scenario->NormalLighting;
    const short base = narrow(Ambient + wrapped_product(1000, scenario->AmbientCurrent) / 100);
    const int ground = narrow(lighting.Ground), level = narrow(lighting.Level);
    const int height = static_cast<signed char>(Level);
    const short normal = narrow(base + height * level - ground);
    const short bridge = narrow(base + (height + 4) * level - ground);
    const short terrain = narrow(static_cast<int>((Intensity * static_cast<DWORD>(normal)) >> 16));
    const short bridge_scaled = narrow(static_cast<int>((Intensity * static_cast<DWORD>(bridge)) >> 16));
    Intensity_Normal = static_cast<WORD>(std::clamp<int>(normal, 0, 2000));
    Intensity_Terrain = static_cast<WORD>(std::clamp<int>(terrain, 0, 2000));
    Color1_Blue = static_cast<WORD>(std::clamp<int>(bridge_scaled, 0, 2000));
}
