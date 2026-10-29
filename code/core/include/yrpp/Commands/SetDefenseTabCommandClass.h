#pragma once

#include "yrpp/CommandClass.h"

class SetDefenseTabCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00535DE0
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00535DE0); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00535DF0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00535DF0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00535E10
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00535E10); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00535E30
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00535E30); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00535E50
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00535E50); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
