// Drawing.h color operations; fixed YRpp 9402d7da and calibrated RGB shifts.
// The standalone optional software renderer starts in explicit RGB565 mode.
// EXE storage aliases are supplied only by compat; no backend/provider is called.
#include "yrpp/Drawing.h"
#ifndef RA2_IMAGE_GAME
#include "yrpp/ConvertClass.h"
#endif
#include <stdexcept>
#include <limits>


bool Drawing::SetColorMode(RGBMode mode) noexcept {
    unsigned rbits, gbits, bbits;
    switch (static_cast<unsigned>(mode)) {
    case 0: rbits = gbits = bbits = 5; break;
    case 1: rbits = gbits = 5; bbits = 6; break;
    case 2: rbits = bbits = 5; gbits = 6; break;
    case 3: rbits = 6; gbits = bbits = 5; break;
    default: return false;
    }
#ifndef RA2_IMAGE_GAME
    // The EXE captures Drawing mode only while 829D20 == -1 (555E3D..555E4B).
    // Standalone hosts may change formats between sessions after destroying old
    // Converts; let the next LightConvert capture the newly selected Drawing mode.
    if (ColorMode != mode) LightConvertClass::LightMode = -1;
#endif
    ColorMode = mode;
    RedShiftLeft = int(gbits + bbits); RedShiftRight = int(8 - rbits);
    GreenShiftLeft = int(bbits); GreenShiftRight = int(8 - gbits);
    BlueShiftLeft = 0; BlueShiftRight = int(8 - bbits);
    const auto mask = [=](unsigned shift) {
        return short((((1u << (rbits - shift)) - 1) << (gbits + bbits)) |
            (((1u << (gbits - shift)) - 1) << bbits) | ((1u << (bbits - shift)) - 1));
    };
    HalfbrightMask = mask(1); QuarterbrightMask = mask(2); EighthbrightMask = mask(3);
    return true;
}

const int* Drawing::GetZGradient(int index) {
#ifdef RA2_IMAGE_GAME
    // Preserve the original 32-bit displacement for the original unchecked ABI.
    return reinterpret_cast<const int*>(reinterpret_cast<std::uintptr_t>(ZGradientTable) +
        (std::uint32_t(index) + 1u) * 24u);
#else
    if (!ZGradientTable || index < -1 || index > 3)
        throw std::logic_error("SHP Z rendering requires a calibrated Drawing::ZGradientTable (-1..3)");
    const int* values = ZGradientTable[index + 1];
    if (!values[2] || !values[3] || (values[3] == std::numeric_limits<int>::min() && values[2] == -1) ||
        (values[2] && std::int64_t(values[3]) / values[2] == 0) || (BYTE(values[5]) && !values[1]))
        throw std::invalid_argument("Invalid SHP Z-gradient descriptor");
    return values;
#endif
}
