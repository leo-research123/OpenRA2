#pragma once
#include "yrpp/platform/ABI.h"

#include "yrpp/SidebarClass.h"

class MouseCursor {
public:
    /// Global VA: 0x0082D028.
    static MouseCursor (&Cursors)[86];

    static MouseCursor& GetCursor(MouseCursorType cursor) { return Cursors[static_cast<int>(cursor)]; }

    MouseCursor() = default;

    MouseCursor(int frame, int count, int interval, int miniFrame, int miniCount, MouseHotSpotX hotX,
                MouseHotSpotY hotY)
        : Frame(frame), Count(count), Interval(interval), MiniFrame(miniFrame), MiniCount(miniCount), HotX(hotX),
          HotY(hotY) {}

    int Frame{0};
    int Count{1};
    int Interval{1};
    int MiniFrame{-1};
    int MiniCount{0};
    MouseHotSpotX HotX{MouseHotSpotX::Center};
    MouseHotSpotY HotY{MouseHotSpotY::Middle};
};

struct TabDataClass {
    /// VA: 0x004A2600
#if defined(RA2_YRPP_GAME)
    void Update(bool force) { JMP_THIS(0x4A2600); }
#else
    void Update(bool force);
#endif
    /// Global VA: 0x0089F95C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, LastUpdateFrame, 0x89F95C)
    /// Global VA: 0x00884B90.
    DEFINE_REFERENCE(bool, CreditsNeedsRedraw, 0x884B90)
#else
    static int& LastUpdateFrame;
    /// Global VA: 0x00884B90.
    static bool& CreditsNeedsRedraw;
#endif
    int TargetValue;
    int LastValue;
    bool NeedsRedraw;
    bool ValueIncreased;
    bool ValueChanged;
    PROTECTED_PROPERTY(BYTE, align_B);
    int ValueDelta;
};

class TabClass : public SidebarClass, public INoticeSink {
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static TabClass& Instance;
    TabClass();
    bool INoticeSink_Unknown(DWORD code, DWORD parameter = 0) override;
    // 72FC60's bottom command surface, consumed by TabClass::Draw (6D0A20).
    RectangleStruct GetCommandBarBounds() const noexcept;
    static int (&CommandPositions)[25];             // B0CB78.
    static int& CommandCount;                       // B0CB54.
    static ShapeButtonClass (&CommandButtons)[25];  // B0C1C0, indexed by slot.
    static ShapeButtonClass& CollapseButton;        // B0CCB0.
    static ShapeButtonClass& ExpandButton;          // B0CC40.
    void InitializeCommandButtons() noexcept;
    void ProcessButtonKey(DWORD key) noexcept;
    /// VA: 0x006D0680
#if defined(RA2_YRPP_GAME)
    void Update(const int& key, const Point2D& point) override { JMP_THIS(0x6D0680); }
#else
    void Update(const int& key, const Point2D& point) override;
#endif
    /// VA: 0x006D0A20
#if defined(RA2_YRPP_GAME)
    void Draw(DWORD force) override { JMP_THIS(0x6D0A20); }
#else
    void Draw(DWORD force) override;
#endif

    /// VA: 0x006D0F00
#if defined(RA2_YRPP_GAME)
    void RefreshMissionTimer() { JMP_THIS(0x6D0F00); }
#else
    void RefreshMissionTimer() noexcept;
#endif

    TabDataClass TabData;
    CDTimerClass unknown_timer_552C;
    CDTimerClass InsufficientFundsBlinkTimer;
    bool ThumbActive;
    bool MissionTimerPinged;
    BYTE unknown_byte_5546;
    PROTECTED_PROPERTY(BYTE, padding_5547);
};

class ScrollClass : public TabClass {
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static ScrollClass& Instance;
    ScrollClass();
    /// VA: 0x006922E0
#if defined(RA2_YRPP_GAME)
    void Update(const int& key, const Point2D& point) override { JMP_THIS(0x6922E0); }
#else
    void Update(const int& key, const Point2D& point) override;
#endif
    /// Global VA: 0x00B05638.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(SysTimerClass, ScrollTimer, 0xB05638)
#else
    static SysTimerClass& ScrollTimer;
#endif
    /// Global VA: 0x00B0565C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, ScrollDirection, 0xB0565C)
#else
    static int& ScrollDirection;
#endif
    void ResetScrollInput() noexcept;
    // Device-independent normal-button portion of the Win32 message entry.
    // Coordinates are map-local; OS capture is owned by the native adapter.
    // Native input and virtual callbacks must not propagate exceptions.
    // This covers normal dispatch; PlanMgr node interception remains separate.
    /// VA: 0x006930A0
    bool ProcessMouseButton(unsigned button, bool pressed, Point2D point) noexcept;
    /// VA: 0x006938C0
#if defined(RA2_YRPP_GAME)
    void ClearDragBand() override { JMP_THIS(0x6938C0); }
#else
    void ClearDragBand() override;
#endif
    /// VA: 0x00693880
#if defined(RA2_YRPP_GAME)
    void RightMouseButtonClick(Point2D* point) override { JMP_THIS(0x693880); }
#else
    void RightMouseButtonClick(Point2D* point) override;
#endif
    /// VA: 0x00693840
#if defined(RA2_YRPP_GAME)
    void RightMouseButtonUp(DWORD miniMap) override { JMP_THIS(0x693840); }
#else
    void RightMouseButtonUp(DWORD miniMap) override;
#endif
    /// VA: 0x00692F30
    void PollMouse();
    /// VA: 0x00693440
    bool DragScroll(const Point2D& position, Point2D& warp, bool& should_warp) noexcept;
    /// VA: 0x00692B60
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

class MouseClass : public ScrollClass {
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

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MouseClass();

    // GScreenClass
    /// VA: 0x005BDDC0
#if defined(RA2_YRPP_GAME)
    void Update(const int& key, const Point2D& point) override { JMP_THIS(0x5BDDC0); }
#else
    void Update(const int& key, const Point2D& point) override;
#endif
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
