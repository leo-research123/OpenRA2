#pragma once

#include "yrpp/CommandClass.h"

class GuardCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00536C90
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00536C90); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00536CA0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00536CA0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00536CC0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00536CC0); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00536CE0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00536CE0); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00536D00
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00536D00); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
