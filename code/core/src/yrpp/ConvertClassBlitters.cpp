// YRpp interface.
#include "ConvertClassHelpers.hpp"
#include "images/original_abi.hpp"
#include "yrpp/Drawing.h"

#ifndef RA2_IMAGE_GAME
namespace { unsigned int plain_z_flags = 0x3000, rle_z_flags = 0x3000; }
unsigned int& ConvertClass::PlainZFlags = plain_z_flags;
unsigned int& ConvertClass::RLEZFlags = rle_z_flags;
#endif

Blitter* ConvertClass::SelectPlainBlitter(BlitterFlags flags) const {
    if (!Blitters[0]) game::initialize_blitters(const_cast<ConvertClass*>(this));
    const auto f = static_cast<unsigned>(flags);
    const bool alpha = (f & 0x800) != 0, z = (f & ConvertClass::PlainZFlags) != 0;
    unsigned slot;
    if (f & 0x10) slot = f & 0x4000 ? (alpha ? 48 : 22) : z ? (alpha ? 39 : 12) : alpha ? 28 : 5;
    else if (f & 6) {
        const unsigned t = (6 - (f & 6)) / 2;
        if (f & 0x4000) slot = (alpha ? 49 : 23) + t;
        else if (z) slot = ((f & 8) ? (alpha ? 43 : 16) : (alpha ? 40 : 13)) + t;
        else if (alpha) slot = 29 + t;
        else if ((f & 6) == 4 && (f & 0x10000)) slot = 36;
        else if ((f & 6) == 4 && (f & 0x20000)) slot = 35;
        else slot = 6 + t;
    } else if (f & 1) slot = f & 0x4000 ? 21 : z ? 11 : 4;
    else if (f & 0x4000) slot = alpha ? 47 : 20;
    else if (z) slot = alpha ? 38 : 10;
    else if (f & 0x20) slot = alpha ? 26 : 2;
    else if (f & 0x8000) slot = 32;
    else if (f & 0x100) slot = 33;
    else if (f & 0x40) slot = 34;
    else slot = alpha ? 27 : 3;
    return Blitters[slot - 2];
}

RLEBlitter* ConvertClass::SelectRLEBlitter(BlitterFlags flags) const {
    if (!RLEBlitters[0]) game::initialize_blitters(const_cast<ConvertClass*>(this));
    const auto f = static_cast<unsigned>(flags);
    const bool alpha = (f & 0x800) != 0, z = (f & ConvertClass::RLEZFlags) != 0;
    unsigned slot;
    if (f & 0x10) slot = f & 0x4000 ? (alpha ? 87 : 68) : z ? (alpha ? 79 : 59) : alpha ? 74 : 54;
    else if (f & 6) {
        const unsigned t = (6 - (f & 6)) / 2;
        if (f & 0x4000) slot = (alpha ? 88 : 70) + t;
        else if (z) slot = ((f & 8) ? (alpha ? 83 : 64) : (alpha ? 80 : 61)) + t;
        else slot = (alpha ? 75 : 55) + t;
    } else if (f & 1) slot = f & 0x4000 ? 69 : z ? 60 : 53;
    else if (f & 0x4000) slot = alpha ? 86 : 67;
    else if (z) slot = alpha ? 78 : 58;
    else slot = alpha ? 73 : 52;
    return RLEBlitters[slot - 52];
}
