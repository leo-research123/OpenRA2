#pragma once

#include "yrpp/YRPPCore.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/IndexClass.h"

struct ToolTip
{
    /// VA: 0x006D1810.
#if defined(RA2_YRPP_GAME)
    ToolTip() { JMP_THIS(0x6D1810); }
#else
    ToolTip() noexcept : GadgetID(0), Bounds{}, Text(nullptr), field_18(false) {}
#endif

    unsigned int GadgetID;
    RectangleStruct Bounds;
    const char* Text;
    bool field_18;
};

struct ToolTipManagerData
{
    RectangleStruct Dimension;
    wchar_t HelpText[0x100];
};

class NOVTABLE ToolTipManager
{
public:

    // Destructor
    /// VA: 0x007240B0
#if defined(RA2_YRPP_GAME)
    virtual ~ToolTipManager() { JMP_THIS(0x7240B0); }
#else
    virtual ~ToolTipManager();
#endif

    // ToolTipManager
    /// VA: 0x00724AA0
#if defined(RA2_YRPP_GAME)
    virtual bool Update(ToolTipManagerData& from) { JMP_THIS(0x724AA0); }
#else
    virtual bool Update(ToolTipManagerData& from);
#endif
    /// VA: 0x00724AB0
#if defined(RA2_YRPP_GAME)
    virtual void MarkToRedraw(ToolTipManagerData& from) { JMP_THIS(0x724AB0); }
#else
    virtual void MarkToRedraw(ToolTipManagerData& from);
#endif
    /// VA: 0x00724B80
#if defined(RA2_YRPP_GAME)
    virtual void Draw(bool bOnSidebar) { JMP_THIS(0x724B80); }
#else
    virtual void Draw(bool bOnSidebar);
#endif
    /// VA: 0x00724BB0
#if defined(RA2_YRPP_GAME)
    virtual void DrawText(ToolTipManagerData& from) { JMP_THIS(0x724BB0); }
#else
    virtual void DrawText(ToolTipManagerData& from);
#endif
    /// VA: 0x00724BD0
#if defined(RA2_YRPP_GAME)
    virtual wchar_t* GetToolTipText(unsigned int ID) { JMP_THIS(0x724BD0); }
#else
    virtual wchar_t* GetToolTipText(unsigned int ID);
#endif

    // Non virtual
    /// VA: 0x007241A0.
#if defined(RA2_YRPP_GAME)
    void SetState(bool bState) { JMP_THIS(0x7241A0); }
#else
    void SetState(bool bState);
#endif
    /// VA: 0x00724200.
#if defined(RA2_YRPP_GAME)
    void ProcessMessage(MSG* pMSG) { JMP_THIS(0x724200); }
#else
    void ProcessMessage(MSG* pMSG);
#endif
    /// VA: 0x00724510.
#if defined(RA2_YRPP_GAME)
    int GetTimerDelay() { JMP_THIS(0x724510); }
#else
    int GetTimerDelay();
#endif
    /// VA: 0x00724520.
#if defined(RA2_YRPP_GAME)
    void SetTimerDelay(int value) { JMP_THIS(0x724520); }
#else
    void SetTimerDelay(int value);
#endif
    /// VA: 0x00724530.
#if defined(RA2_YRPP_GAME)
    void SaveTimerDelay() { JMP_THIS(0x724530); }
#else
    void SaveTimerDelay();
#endif
    /// VA: 0x00724540.
#if defined(RA2_YRPP_GAME)
    void RestoreTimeDelay() { JMP_THIS(0x724540); }
#else
    void RestoreTimeDelay();
#endif
    /// VA: 0x00724550.
#if defined(RA2_YRPP_GAME)
    int GetLifeTime() { JMP_THIS(0x724550); }
#else
    int GetLifeTime();
#endif
    /// VA: 0x00724560.
#if defined(RA2_YRPP_GAME)
    void SetLifeTime(int value) { JMP_THIS(0x724560); }
#else
    void SetLifeTime(int value);
#endif
    /// VA: 0x00724570.
#if defined(RA2_YRPP_GAME)
    int GetToolTipCount() { JMP_THIS(0x724570); }
#else
    int GetToolTipCount();
#endif
    /// VA: 0x00724580.
#if defined(RA2_YRPP_GAME)
    bool Add(ToolTip& tooltip) { JMP_THIS(0x724580); }
#else
    bool Add(ToolTip& tooltip);
#endif
    /// VA: 0x00724730.
#if defined(RA2_YRPP_GAME)
    void Remove(unsigned int ID) { JMP_THIS(0x724730); }
#else
    void Remove(unsigned int ID);
#endif
    /// VA: 0x007248C0.
#if defined(RA2_YRPP_GAME)
    bool Find(unsigned int ID, ToolTip& tooltip) { JMP_THIS(0x7248C0); }
#else
    bool Find(unsigned int ID, ToolTip& tooltip);
#endif
    /// VA: 0x00724A30.
#if defined(RA2_YRPP_GAME)
    ToolTip* FindFromPosition(Point2D& point) { JMP_THIS(0x724A30); }
#else
    ToolTip* FindFromPosition(Point2D& point);
#endif
    /// VA: 0x00724AD0.
#if defined(RA2_YRPP_GAME)
    bool Process() { JMP_THIS(0x724AD0); }
#else
    bool Process();
#endif
    /// VA: 0x00724BE0.
#if defined(RA2_YRPP_GAME)
    void Hide() { JMP_THIS(0x724BE0); }
#else
    void Hide();
#endif
    /// VA: 0x00724C00.
#if defined(RA2_YRPP_GAME)
    bool IsToolTipShowing() { JMP_THIS(0x724C00); }
#else
    bool IsToolTipShowing();
#endif

    // Statics

    // Constructors
    /// VA: 0x00724000.
#if defined(RA2_YRPP_GAME)
    ToolTipManager(HWND hWnd) noexcept
        : ToolTipManager(noinit_t()) { JMP_THIS(0x724000); }
#else
    explicit ToolTipManager(HWND window) noexcept;
#endif

protected:
    explicit __forceinline ToolTipManager(noinit_t)  noexcept
    {
    }

    // Properties
public:
    ToolTip* CurrentToolTip;
    HWND hWnd;
    bool IsActive;
    Point2D CurrentMousePosition;
    ToolTipManagerData CurrentToolTipData;
    int ToolTipDelay;
    int LastToolTipDelay;
    int ToolTipLifeTime;
    DynamicVectorClass<ToolTip*> ToolTips;
    IndexClass<int, ToolTip*> ToolTipIndex;
};
