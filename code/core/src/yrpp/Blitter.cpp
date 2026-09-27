// Fixed YRpp 9402d7da Blitter base implementations.
#include "Blitter.hpp"
#include "yrpp/Drawing.h"

Blitter::~Blitter() = default;
RLEBlitter::~RLEBlitter() = default;

WORD* Blitter::Lookup_Alpha_Remapper(int alvl, AlphaLightingRemapClass* remapper)
{
    // convert alvl from [0, 2000] into [0, 254]
    int level = std::min(254, 261* std::max(0, alvl) >> 11);
    return remapper->Table[level];
}

WORD* RLEBlitter::Lookup_Alpha_Remapper(int alvl, AlphaLightingRemapClass* remapper)
{
    // convert alvl from [0, 2000] into [0, 254]
    int level = std::min(254, 261* std::max(0, alvl) >> 11);
    return remapper->Table[level];
}

WORD Blitter::BlendAlphaRGB(WORD source, WORD destination, WORD alpha) {
    BYTE r, g, b, dr, dg, db;
    Drawing::Int_To_RGB(source, r, g, b);
    Drawing::Int_To_RGB(destination, dr, dg, db);
    const WORD inverse = WORD(255 - alpha);
    if (alpha == 255) alpha = 256;
    return Drawing::RGB_To_Int(BYTE((r * alpha + dr * inverse) >> 8),
        BYTE((g * alpha + dg * inverse) >> 8), BYTE((b * alpha + db * inverse) >> 8));
}
