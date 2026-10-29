#pragma once

#include "yrpp/CommandClass.h"

class ScatterCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00536D10
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00536D10); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00536D20
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00536D20); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00536D40
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00536D40); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00536D60
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00536D60); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00536D80
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00536D80); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
