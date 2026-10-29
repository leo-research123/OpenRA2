#pragma once

#include "yrpp/CommandClass.h"

class ToggleSellCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00536FB0
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00536FB0); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00536FC0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00536FC0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00536FE0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00536FE0); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00537000
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00537000); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00537020
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00537020); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
