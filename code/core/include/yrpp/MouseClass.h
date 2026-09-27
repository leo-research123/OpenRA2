#pragma once
#include "yrpp/platform/ABI.h"

#include "yrpp/SidebarClass.h"

namespace game { struct GameInputEvent; struct GameInputResult; }

class MouseCursor {
public:
    /// Global VA: 0x0082D028.
    static MouseCursor (&Cursors)[86];

    static MouseCursor& GetCursor(MouseCursorType cursor) {
        return Cursors[static_cast<int>(cursor)];
    }

    MouseCursor() = default;

    MouseCursor(
        int frame, int count, int interval, int miniFrame, int miniCount,
        MouseHotSpotX hotX, MouseHotSpotY hotY)
        : Frame(frame), Count(count), Interval(interval), MiniFrame(miniFrame),
        MiniCount(miniCount), HotX(hotX), HotY(hotY)
    { }

    int Frame{ 0 };
    int Count{ 1 };
    int Interval{ 1 };
    int MiniFrame{ -1 };
    int MiniCount{ 0 };
    MouseHotSpotX HotX{ MouseHotSpotX::Center };
    MouseHotSpotY HotY{ MouseHotSpotY::Middle };
};

struct TabDataClass
{
    int TargetValue;
    int LastValue;
    bool NeedsRedraw;
    bool ValueIncreased;
    bool ValueChanged;
    PROTECTED_PROPERTY(BYTE, align_B);
    int ValueDelta;
};

class TabClass : public SidebarClass, public INoticeSink
{
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static TabClass& Instance;
    TabClass();
    bool YRPP_STDCALL INoticeSink_Unknown(DWORD code) override;
    // 72FC60's bottom command surface, consumed by TabClass::Draw (6D0A20).
    RectangleStruct GetCommandBarBounds() const noexcept;
    static int (&CommandPositions)[25]; // B0CB78.
    static int& CommandCount; // B0CB54.
    static ShapeButtonClass (&CommandButtons)[25]; // B0C1C0, indexed by slot.
    static ShapeButtonClass& CollapseButton; // B0CCB0.
    static ShapeButtonClass& ExpandButton; // B0CC40.
    void InitializeCommandButtons() noexcept;
    void ProcessButtonKey(DWORD key) noexcept;
    /// VA: 0x006D0A20
#if defined(RA2_YRPP_GAME)
    void Draw(DWORD force) override { JMP_THIS(0x6D0A20); }
#else
    void Draw(DWORD force) override;
#endif

    TabDataClass TabData;
    CDTimerClass unknown_timer_552C;
    CDTimerClass InsufficientFundsBlinkTimer;
    bool ThumbActive;
    bool MissionTimerPinged;
    BYTE unknown_byte_5546;
    PROTECTED_PROPERTY(BYTE, padding_5547);
};

class ScrollClass : public TabClass
{
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static ScrollClass& Instance;
    ScrollClass();
    void ResetScrollInput() noexcept;
    bool DragScroll(const Point2D& position,Point2D& warp,bool& should_warp) noexcept;
    bool ScrollAtEdge(const Point2D& position) noexcept;

    DWORD unknown_int_5548;
    BYTE unknown_byte_554C;
    PROTECTED_PROPERTY(BYTE, align_554D[3]);
    DWORD unknown_int_5550;
    DWORD unknown_int_5554;
    BYTE unknown_byte_5548;
    BYTE unknown_byte_5549;
    BYTE unknown_byte_554A;
    PROTECTED_PROPERTY(BYTE, padding_554B);
};

class MouseClass : public ScrollClass
{
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static MouseClass& Instance;
    /// Global VA: 0x00ABF294.
    static SHPStruct*& CursorShape;
    /// Global VA: 0x00ABF2A0.
    static SysTimerClass& CursorTimer;
    /// Global VA: 0x00ABF2DD.
    static bool& CursorInitialized;
    MouseClass();
    void ProcessInput(const game::GameInputEvent&,game::GameInputResult&) noexcept;
    void ResetInput() noexcept;
    void UpdateInput(const Point2D& point,bool inside) noexcept;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MouseClass();

    // GScreenClass
    // Original precondition: cursor art and WWMouse driver are initialized.
    // Driver callbacks must not propagate exceptions across this boundary.
    /// VA: 0x005BDA80
    bool SetCursor(MouseCursorType idxCursor, bool miniMap) override;
    /// VA: 0x005BDC80
    bool UpdateCursor(MouseCursorType idxCursor, bool miniMap) override;
    /// VA: 0x005BDAA0
    bool RestoreCursor() override;
    /// VA: 0x005BDAB0
    void UpdateCursorMinimapState(bool miniMap) override;

    // DisplayClass
    /// VA: 0x0040D280
    MouseCursorType GetLastMouseCursor() override;

    bool MouseCursorIsMini;
    PROTECTED_PROPERTY(BYTE, unknown_byte_5559[3]);
    MouseCursorType MouseCursorIndex;
    MouseCursorType MouseCursorLastIndex;
    int MouseCursorCurrentFrame;
};
