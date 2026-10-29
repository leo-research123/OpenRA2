// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 scheme.cpp named constructor; YR 0x0068C710 / 0x0068C3B0.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
// The core owns the original converter; hosts consume its prepared tables.
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "type_resources.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
BYTE* scheme_indexes() noexcept {
    // Original immutable mask at 0x0083E1AC is shared by constructors/rebuilds.
    static std::array<BYTE, 256> indexes = [] {
        std::array<BYTE, 256> value{};
        for (int i = 0; i < 256; ++i)
            value[i] = BYTE(i < 240 || i == 255);
        return value;
    }();
    return indexes.data();
}
}
LightConvertClass* ColorScheme::UpdatePalette(const BytePalette& art, const BytePalette& screen, int red, int green,
                                              int blue) noexcept {
    delete LightConvert;
    LightConvert = nullptr;
    try {
        BuildPalette(BaseColor, art, Colors);
        LightConvert = new (std::nothrow) LightConvertClass(&Colors, const_cast<BytePalette*>(&screen), 2, red, green,
                                                            blue, false, scheme_indexes(), ShadeCount);
        if (!LightConvert) game::type_resource_failure(ID);
    } catch (...) {
        game::type_resource_failure(ID);
    }
    return LightConvert;
}
int YRPP_FASTCALL ColorScheme::FindByName(const char* id, const ColorStruct& baseColor, const BytePalette& palette1,
                                          const BytePalette& palette2, int shades) {
    const int existing = FindIndex(id, shades);
    if (existing >= 0) return existing;
    // The EXE retries the lookup after construction. Native allocation failure
    // unwinds to the resource loader instead of retrying forever.
    new ColorScheme(id, baseColor, palette1, palette2, shades, true);
    return FindIndex(id, shades);
}

ColorScheme::ColorScheme(const char* id, const ColorStruct& baseColor, const BytePalette& palette1,
                         const BytePalette& palette2, int shades, bool add)
    : Colors{}, ID(nullptr), BaseColor(baseColor), LightConvert(nullptr), ShadeCount(shades) {
    const auto length = std::strlen(id) + 1;
    ID = static_cast<char*>(std::malloc(length));
    if (!ID) throw std::bad_alloc();
    std::memcpy(ID, id, length);
    const int first[]{16, 15, 25, 24, 22, 16, 19};
    const int last[]{16, 21};
    std::memcpy(unknown_314, first, sizeof(first));
    MainShadeIndex = 16;
    std::memcpy(unknown_334, last, sizeof(last));
    try {
        BuildPalette(BaseColor, palette1, Colors);
        LightConvert = new (std::nothrow) LightConvertClass(&Colors, const_cast<BytePalette*>(&palette2), 2, 1000, 1000,
                                                            1000, false, scheme_indexes(), shades);
        if (add) {
            if (!Array.AddItem(this)) throw std::bad_alloc();
            ArrayIndex = Array.Count;
        }
    } catch (...) {
        delete LightConvert;
        std::free(ID);
        throw;
    }
}
#endif
