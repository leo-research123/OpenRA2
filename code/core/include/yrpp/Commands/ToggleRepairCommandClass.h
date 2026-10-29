#pragma once

#include "yrpp/CommandClass.h"

class ToggleRepairCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00536F30
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00536F30); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00536F40
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00536F40); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00536F60
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00536F60); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00536F80
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00536F80); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00536FA0
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00536FA0); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
