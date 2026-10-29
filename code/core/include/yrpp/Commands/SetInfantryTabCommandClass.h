#pragma once

#include "yrpp/CommandClass.h"

class SetInfantryTabCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00535EA0
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00535EA0); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00535EB0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00535EB0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00535ED0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00535ED0); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00535EF0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00535EF0); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00535F10
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00535F10); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
