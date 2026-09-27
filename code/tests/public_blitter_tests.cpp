#include "support/test_support.hpp"
// Separately compiled with only core/include, linked to the image test backend.
#include "yrpp/Blitters/BlitPlain.h"
#include "yrpp/Blitters/BlitPlainXlat.h"
#include "yrpp/Blitters/BlitPlainXlatAlpha.h"
#include "yrpp/Blitters/RLEBlitTransXlat.h"
#include <array>
#include <stdexcept>

void public_blitter_tests() {
    BYTE input[] = {1, 2};
    BYTE bytes[] = {99, 99};
    BlitPlain<BYTE> plain;
    Blitter& virtual_plain = plain;
    virtual_plain.Blit_Copy(bytes, input, 2, 0, nullptr, nullptr, 1000, 0);
    EXPECT_FALSE((bytes[0] != 1 || bytes[1] != 2)) << "public BYTE blitter linkage";

    std::array<WORD, 256> palette{};
    palette[1] = 100; palette[2] = 200;
    WORD words[] = {99, 99};
    BlitPlainXlat<WORD> xlat(palette.data());
    Blitter& virtual_xlat = xlat;
    virtual_xlat.Blit_Copy(words, input, 2, 0, nullptr, nullptr, 1000, 0);
    EXPECT_FALSE((words[0] != 100 || words[1] != 200)) << "public WORD blitter linkage";

    BYTE encoded[] = {1, 0, 1, 2};
    WORD decoded[] = {99, 99, 99};
    RLEBlitTransXlat<WORD> rle(palette.data());
    RLEBlitter& virtual_rle = rle;
    virtual_rle.Blit_Copy(decoded, encoded, 3, 0, 0, nullptr, nullptr, 1000, 0, nullptr);
    EXPECT_FALSE((decoded[0] != 100 || decoded[1] != 99 || decoded[2] != 200)) << "public RLE blitter linkage";
    // The fixture checks Alpha acquisition/release balance after this returns.
    BlitPlainXlatAlpha<WORD> alpha(palette.data(), 1);
}
