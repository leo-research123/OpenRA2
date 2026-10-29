#pragma once

#include "yrpp/CommandClass.h"

class SetStructureTabCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00535D20
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00535D20); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00535D30
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00535D30); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00535D50
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00535D50); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00535D70
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00535D70); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00535D90
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00535D90); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
