#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Helpers/CompileTime.h"

class NOVTABLE EvadeClass {
public:
    /// Global VA: 0x008A38E0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(EvadeClass, Instance, 0x8A38E0)
#else
    static EvadeClass& Instance;
#endif

    /// VA: 0x004C6140
#if defined(RA2_YRPP_GAME)
    void Reset() { JMP_THIS(0x4C6140); }
#else
    void Reset() noexcept;
#endif
    /// VA: 0x004C6180
#if defined(RA2_YRPP_GAME)
    void Save() { JMP_THIS(0x4C6180); }
#else
    void Save();
#endif
    /// VA: 0x004C6210
#if defined(RA2_YRPP_GAME)
    void Do() { JMP_THIS(0x4C6210); }
#else
    void Do();
#endif

    // Properties
public:
    char CarryOverGlobals[50];
    int CarryOverMoney;
    int CarryOverTimer;
    int CarryOverDifficulty;
    short CarryOverStage;
};
