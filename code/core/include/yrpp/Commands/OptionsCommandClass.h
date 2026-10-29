#pragma once

#include "yrpp/CommandClass.h"

class OptionsCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00537260
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00537260); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00537270
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00537270); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00537290
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00537290); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x005372B0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x005372B0); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x005372D0
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x005372D0); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
