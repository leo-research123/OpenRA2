#pragma once

#include "yrpp/Helpers/CompileTime.h"

class Powerups {
    public:
    // all these actually point to arrays with 0x13 items, see ePowerup for their numbering
    /**
     * e.g. Powerups::Weights[pow_Unit] is the weight of the free unit crate
     */

    // the name of the effect, for INI reading purposes
    static const char* const (&Effects)[19];

    // the weight of the effect
    static int (&Weights)[19];

    // the effect-specific argument
    static double (&Arguments)[19];

    // can this crate appear on water?
    static bool (&Naval)[19];

    // index into AnimTypeClass::Array
    static int (&Anims)[19];
};
