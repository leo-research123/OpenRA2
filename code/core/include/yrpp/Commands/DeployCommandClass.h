#pragma once

#include "yrpp/CommandClass.h"

class DeployCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00536C10
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00536C10); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00536C20
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00536C20); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00536C40
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00536C40); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00536C60
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00536C60); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00536C80
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x00536C80); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
