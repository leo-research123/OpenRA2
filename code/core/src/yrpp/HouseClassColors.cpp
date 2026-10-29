// HouseClass color derivation, YR 0x0050B840 and 0x0050BA00. Both are called
// by ScenarioClass::AssignHouses (0x00687F10).
#include "yrpp/HouseClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/YRMath.h"
#include "x87_double.hpp"
#include "x87_integer.hpp"
#include <stdexcept>

void HouseClass::UpdateColor() {
    if (ColorSchemeIndex < 0) ColorSchemeIndex = 5;
    auto* scheme = ColorScheme::Array.GetItemOrDefault(ColorSchemeIndex);
    if (!scheme) {
        // The original logs "Forcing House ... to color WHITE" through a no-op.
        ColorSchemeIndex = 5;
        scheme = ColorScheme::Array.GetItemOrDefault(5);
    }
    // The original dereferences these unchecked; a native host reports the missing resource instead.
    if (!scheme || !scheme->LightConvert || !scheme->LightConvert->PaletteData)
        throw std::runtime_error("House color scheme is unavailable");
    const auto& convert = *scheme->LightConvert;
    const unsigned pixel = convert.BytesPerPixel == 1
                               ? static_cast<const BYTE*>(convert.PaletteData)[scheme->MainShadeIndex]
                               : static_cast<const WORD*>(convert.PaletteData)[scheme->MainShadeIndex];
    Color = Drawing::Int_To_RGB(static_cast<int>(pixel));
}

void HouseClass::UpdateLaserColor() {
    // The arithmetic runs under the 0x0E7F control word that 0x007C5F00 leaves active (53-bit, toward zero).
    const auto multiply = [](double a, double b) { return game::x87_multiply(a, b); };
    const auto length = [&](double a, double b, double c) {
        return Math::sqrt(game::x87_add(game::x87_add(multiply(a, a), multiply(b, b)), multiply(c, c)));
    };
    double red = Color.R, green = Color.G, blue = Color.B;
    const double first = length(blue, green, red);
    if (first == 0.0) {
        red = green = blue = 255.0;
    } else {
        const auto scale = [&](double value) {
            const double scaled = game::x87_divide(multiply(value, 240.0), first);
            return scaled > 255.0 ? 255.0 : scaled;
        };
        red = scale(red);
        green = scale(green);
        blue = scale(blue);
        if (red < 96.0) red = 0.0;
        if (green < 96.0) green = 0.0;
        if (blue < 96.0) blue = 0.0;
    }
    const double second = length(blue, green, red);
    if (second == 0.0) {
        red = green = blue = 255.0;
    } else {
        const auto scale = [&](double value) {
            const double scaled = game::x87_divide(multiply(value, 240.0), second);
            return scaled > 255.0 ? 255.0 : scaled;
        };
        red = scale(red);
        green = scale(green);
        blue = scale(blue);
    }
    // Red, green, then blue; each conversion goes through 0x007C5F00.
    LaserColor.R = static_cast<BYTE>(game::x87_integer(red));
    LaserColor.G = static_cast<BYTE>(game::x87_integer(green));
    LaserColor.B = static_cast<BYTE>(game::x87_integer(blue));
}
