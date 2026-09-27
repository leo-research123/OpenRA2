#include "support/test_support.hpp"
// Real software component smoke/regression: no image services, Alpha or pixels doubles.
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace {

struct Image {
    SHPStruct header;
    SHPFrame frame{};
    std::array<BYTE, 16> payload{};
    explicit Image(bool rle) {
        header.Width = 4; header.Height = 2; header.Frames = 1;
        frame.Left = 1; frame.Top = 0; frame.Width = 3; frame.Height = 2;
        frame.Flags = rle ? 3 : 0; frame.Offset = 32;
        if (rle) payload = {6,0,1,0,1,2, 6,0,3,4,0,1};
        else payload = {1,0,2,3,4,0};
    }
};
static_assert(sizeof(SHPStruct) == 8 && sizeof(SHPFrame) == 24);
void draw(bool rle, int bpp, bool remapped) {
    BytePalette palette{};
    palette.Entries[1] = {255,0,0}; palette.Entries[2] = {0,255,0};
    palette.Entries[3] = {0,0,255}; palette.Entries[4] = {255,255,255};
    std::array<BYTE, 256> remap{};
    for (unsigned i=0; i<256; ++i) remap[i] = BYTE(i);
    remap[1] = 4;
    BSurface target(8, 4, bpp);
    auto* pixels = static_cast<BYTE*>(target.Lock(0,0));
    std::fill_n(pixels, 8*4*bpp, BYTE(0x5a));
    target.Unlock();
    {
        ConvertClass convert(palette, palette, bpp, 1, false);
        Image image(rle);
        const Point2D point{1,1}; const RectangleStruct clip{0,0,8,4};
        CC_Draw_Shape(&target, &convert, &image.header, 0, &point, &clip,
            static_cast<BlitterFlags>(0), remapped ? remap.data() : nullptr,
            0, static_cast<ZGradient>(-1), 1000, 0, nullptr, 0, 0, 0);
        const auto pixel = [&](int x,int y) -> unsigned {
            if (bpp==1) return pixels[y*8+x];
            return reinterpret_cast<WORD*>(pixels)[y*8+x];
        };
        const unsigned first = remapped ? 4 : 1;
        const unsigned color = bpp==1 ? static_cast<BYTE*>(convert.PaletteData)[first]
            : static_cast<WORD*>(convert.PaletteData)[first];
        EXPECT_TRUE((pixel(2,1)==color)) << "first drawn pixel / remap";
        EXPECT_TRUE((pixel(3,1)==(bpp==1?0x5au:0x5a5au))) << "transparent pixel preserved";
        EXPECT_TRUE((!target.IsLocked())) << "draw must release destination locks";
    }
    EXPECT_TRUE((ConvertClass::Array.Count==0 && AlphaLightingRemapClass::Array.Count==0)) << "production Convert / shared Alpha lifetime";
}
void config() {
    EXPECT_TRUE((Drawing::SetColorMode(static_cast<RGBMode>(2)))) << "RGB565 mode";
    EXPECT_TRUE((Drawing::RGB_To_Int(255,0,0)==0xf800 && Drawing::RGB_To_Int(0,255,0)==0x07e0)) << "RGB packing";
    EXPECT_TRUE((!Drawing::SetColorMode(static_cast<RGBMode>(99)))) << "invalid mode does not mutate";
    bool rejected=false;
    try { Drawing::GetZGradient(0); } catch (const std::logic_error&) { rejected=true; }
    EXPECT_TRUE((rejected)) << "missing EXE gradient data must not silently use fabricated values";
    // Synthetic descriptor is an INPUT fixture, not an original-game golden table.
    static const int gradients[5][6] = {
        {0,1,1,1,0,1},{0,1,1,1,0,1},{0,1,1,1,0,1},{0,1,1,1,0,1},{0,1,1,1,0,1}};
    Drawing::ZGradientTable=gradients;
    EXPECT_TRUE((Drawing::GetZGradient(-1)==gradients[0] && Drawing::GetZGradient(3)==gradients[4])) << "gradient indexing";
    Drawing::ZGradientTable=nullptr;
}

void light_color_modes() {
    // Golden RGB555/556/565/655 primaries. These are independent of the
    // production packer, so a stale specialized LightMode cannot self-confirm.
    constexpr WORD colors[4][3] = {
        {0x7c00, 0x03e0, 0x001f}, {0xf800, 0x07c0, 0x003f},
        {0xf800, 0x07e0, 0x001f}, {0xfc00, 0x03e0, 0x001f}};
    BytePalette palette{};
    palette.Entries[1] = {255, 0, 0};
    palette.Entries[2] = {0, 255, 0};
    palette.Entries[3] = {0, 0, 255};
    const auto check_palette = [&](int mode) {
        LightConvertClass light(&palette, &palette, 2, 1000, 1000, 1000, false, nullptr, 1);
        const auto* pixels = static_cast<const WORD*>(light.FullColorData);
        for (int channel = 0; channel < 3; ++channel)
            EXPECT_TRUE((pixels[channel + 1] == colors[mode][channel])) << "LightConvert color mode after session turnover";
        EXPECT_TRUE((!Drawing::SetColorMode(static_cast<RGBMode>(99)))) << "reject invalid mode with a live LightConvert";
        light.UpdateColors(1000, 1000, 1000, false);
        for (int channel = 0; channel < 3; ++channel)
            EXPECT_TRUE((pixels[channel + 1] == colors[mode][channel])) << "invalid mode preserves LightConvert colors";
    };
    for (int before = 0; before < 4; ++before) {
        for (int after = 0; after < 4; ++after) {
            EXPECT_TRUE((Drawing::SetColorMode(static_cast<RGBMode>(before)))) << "initial light color mode";
            check_palette(before);
            EXPECT_TRUE((ConvertClass::Array.Count == 0 && AlphaLightingRemapClass::Array.Count == 0)) << "old Convert and Blitters must die before a color mode change";
            EXPECT_TRUE((Drawing::SetColorMode(static_cast<RGBMode>(after)))) << "next light color mode";
            check_palette(after);
        }
    }
    EXPECT_TRUE((Drawing::SetColorMode(static_cast<RGBMode>(2)))) << "restore RGB565 for drawing tests";
}
}

class SoftwareRender : public testing::TestWithParam<std::tuple<bool, int, bool>> {
public:
    static void SetUpTestSuite() { config(); light_color_modes(); }
};
TEST_P(SoftwareRender, RawAndRlePixels) {
    const auto [compressed, depth, remap] = GetParam();
    draw(compressed, depth, remap);
}
INSTANTIATE_TEST_SUITE_P(Formats, SoftwareRender,
    testing::Combine(testing::Values(false, true), testing::Values(1, 2),
        testing::Values(false, true)));
