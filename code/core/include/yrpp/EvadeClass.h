#pragma once

#include "yrpp/YRPP.h"
#include "yrpp/Helpers/CompileTime.h"

class NOVTABLE EvadeClass
{
public:
    /// Global VA: 0x008A38E0.
    DEFINE_REFERENCE(EvadeClass, Instance, 0x8A38E0)

    /// VA: 0x004C6210.
    void Do() { JMP_THIS(0x4C6210); }

    // Properties
public:
    char  CarryOverGlobals[50];
    int   CarryOverMoney;
    int   CarryOverTimer;
    int   CarryOverDifficulty;
    short CarryOverStage;
};
