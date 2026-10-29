#pragma once

#include "yrpp/CommandClass.h"

class TypeSelectCommandClass : public CommandClass {
public:
    // CommandClass
    /// VA: 0x00536810
#if defined(RA2_YRPP_GAME)
    const char* GetName() const override { JMP_THIS(0x00536810); }
#else
    const char* GetName() const override;
#endif

    /// VA: 0x00536820
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00536820); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00536840
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUICategory() const override { JMP_THIS(0x00536840); }
#else
    const wchar_t* GetUICategory() const override;
#endif

    /// VA: 0x00536860
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIDescription() const override { JMP_THIS(0x00536860); }
#else
    const wchar_t* GetUIDescription() const override;
#endif

    /// VA: 0x00536880
#if defined(RA2_YRPP_GAME)
    bool PreventCombinationOverride(WWKey eInput) const override { JMP_THIS(0x00536880); }
#else
    bool PreventCombinationOverride(WWKey eInput) const override;
#endif

    /// VA: 0x00536890
#if defined(RA2_YRPP_GAME)
    bool ExtraTriggerCondition(WWKey eInput) const override { JMP_THIS(0x00536890); }
#else
    bool ExtraTriggerCondition(WWKey eInput) const override;
#endif

    /// VA: 0x005368A0
#if defined(RA2_YRPP_GAME)
    bool CheckLoop55E020(WWKey eInput) const override { JMP_THIS(0x005368A0); }
#else
    bool CheckLoop55E020(WWKey eInput) const override;
#endif

    /// VA: 0x005368B0
#if defined(RA2_YRPP_GAME)
    void Execute(WWKey eInput) const override { JMP_THIS(0x005368B0); }
#else
    void Execute(WWKey eInput) const override;
#endif
};
