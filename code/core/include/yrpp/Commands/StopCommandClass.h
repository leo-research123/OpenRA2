#pragma once

#include "yrpp/CommandClass.h"

class StopCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00536B10
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00536B10); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00536B20
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00536B20); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00536B40
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00536B40); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00536B60
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00536B60); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00536B80
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00536B80); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
