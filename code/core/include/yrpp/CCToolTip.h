#pragma once

#include "yrpp/ToolTipManager.h"
#include "yrpp/Drawing.h"

#include "yrpp/Helpers/CompileTime.h"

class NOVTABLE CCToolTip : public ToolTipManager
{
public:
    // It's also used in MoneyFormat at 6A934A, not sure what side effect it might leads
    /// Global VA: 0x00884B8C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, HideName, 0x884B8C)
#else
    static bool& HideName;
#endif
    /// Global VA: 0x00887368.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(CCToolTip*, Instance, 0x887368)
#else
    static CCToolTip*& Instance;
#endif
    /// Global VA: 0x00B0FA1C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(RGBClass, ToolTipTextColor, 0xB0FA1C)
#else
    static RGBClass& ToolTipTextColor;
#endif

    // Constructor is inlined at 0x007777A3; no new virtual slot or data field.
#if !defined(RA2_YRPP_GAME)
    explicit CCToolTip(HWND window = nullptr) noexcept;
#endif
    /// VA: 0x00478BA0
#if defined(RA2_YRPP_GAME)
    bool Update(ToolTipManagerData& data) override { JMP_THIS(0x00478BA0); }
#else
    bool Update(ToolTipManagerData& data) override;
#endif
    /// VA: 0x00478DB0
#if defined(RA2_YRPP_GAME)
    void MarkToRedraw(ToolTipManagerData& data) override { JMP_THIS(0x00478DB0); }
#else
    void MarkToRedraw(ToolTipManagerData& data) override;
#endif
    /// VA: 0x00478E10
#if defined(RA2_YRPP_GAME)
    void Draw(bool onSidebar) override { JMP_THIS(0x00478E10); }
#else
    void Draw(bool onSidebar) override;
#endif
    /// VA: 0x00478E30
#if defined(RA2_YRPP_GAME)
    void DrawText(ToolTipManagerData& data) override { JMP_THIS(0x00478E30); }
#else
    void DrawText(ToolTipManagerData& data) override;
#endif
    /// VA: 0x00479050
#if defined(RA2_YRPP_GAME)
    wchar_t* GetToolTipText(unsigned int id) override { JMP_THIS(0x00479050); }
#else
    wchar_t* GetToolTipText(unsigned int id) override;
#endif

    // Properties
public:
    bool FullRedraw; // YRpp name; actually selects the Sidebar surface in DrawText.
    int Delay;
};
