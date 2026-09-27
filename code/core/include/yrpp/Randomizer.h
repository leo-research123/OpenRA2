#pragma once

#include "yrpp/platform/ABI.h"
#include <climits>

class Randomizer
{
public:
    // for any randomization happening inside a match (odds of a survivor, crate, etc), use the ScenarioClass::Random object instead!
    // this object should only be used for RMG and other randomness outside a match
    static Randomizer& Global;
    static DWORD& DefaultSeed; // Original storage: 0xA8ED94.

    int Random();

    int RandomRanged(int nMin, int nMax);

    Randomizer(DWORD dwSeed = DefaultSeed);

    // helper methods
    double RandomDouble()
        { return this->RandomRanged(1, INT_MAX) / (double)((unsigned int)INT_MAX + 1); }

    int operator()()
        { return Random(); }
    int operator()(int nMin, int nMax)
        { return RandomRanged(nMin, nMax); }

    // Properties

public:

    bool unknown_00;
    int Next1; //from Table
    int Next2; //from Table
    DWORD Table [0xFA];
};
