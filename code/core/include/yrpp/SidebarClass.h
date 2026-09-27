#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/PowerClass.h"
#include "yrpp/StageClass.h"
#include "yrpp/ShapeButtonClass.h"

class ColorScheme;
class FactoryClass;

// SidebarClass::StripClass::BuildType
struct BuildType
{
    int               ItemIndex{ -1 };
    AbstractType      ItemType{ AbstractType::None };
    BYTE              IsAlt{ 0 }; // the BuildCat of buildings, 0 for everything else
    FactoryClass*     CurrentFactory{ nullptr };
    DWORD             unknown_10{ 0 };
    StageClass        Progress{}; // 0 to 54, how much of this object is constructed (gclock anim level)
    int               FlashEndFrame{ 0 };

    BuildType() = default;

    BuildType(int itemIndex, AbstractType itemType) :
        ItemIndex(itemIndex),
        ItemType(itemType)
    { /*JMP_THIS(0x6AC7C0);*/ }

    bool operator == (const BuildType& rhs) const {
        return ItemIndex == rhs.ItemIndex && ItemType == rhs.ItemType;
    }

    bool operator != (const BuildType& rhs) const {
        return ItemIndex != rhs.ItemIndex || ItemType != rhs.ItemType;
    }

    bool operator < (const BuildType& rhs) const {
        return SortsBefore(this->ItemType, this->ItemIndex, rhs.ItemType, rhs.ItemIndex);
    }

    /// VA: 0x006A8420.
    static bool YRPP_STDCALL SortsBefore(AbstractType leftType, int leftIndex, AbstractType rightType, int rightIndex);
};

struct StripClass;

// the per-cameo button; an array of 4 strips times 60 buttons, stride 0x38
struct SelectClass : public ControlClass
{
    SelectClass() noexcept;
    bool Action(GadgetFlag,DWORD*,KeyModifier) override;
    static SelectClass* Array() noexcept;
    static int CameoPitchX() noexcept;
    static int CameoPitchY() noexcept;
    enum { ButtonID = 202, CameoWidth = 60, CameoHeight = 48 };
    StripClass* Strip;
    int Index;
    DWORD unknown_34;
};

// These pointer-bearing objects have the original layout only on the actual
// Microsoft x86 target; a native host uses its own pointer width.
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SelectClass) == 0x38, "SelectClass must match the game's array stride");
static_assert(offsetof(SelectClass, X) == 0x0C, "SelectClass layout slipped");
static_assert(offsetof(SelectClass, Width) == 0x14, "SelectClass layout slipped");
static_assert(offsetof(SelectClass, ID) == 0x24, "SelectClass layout slipped");
static_assert(offsetof(SelectClass, Strip) == 0x2C, "SelectClass layout slipped");
static_assert(offsetof(SelectClass, Index) == 0x30, "SelectClass layout slipped");
#endif

// SidebarClass::StripClass
struct StripClass
{
    StageClass        Progress;
    bool              AllowedToDraw; // prevents redrawing when layouting the list
    PROTECTED_PROPERTY(BYTE, align_1D[3]);
    Point2D           Location;
    RectangleStruct   Bounds;
    int               Index; // the index of this tab
    bool              NeedsRedraw;
    bool              IsBuilding;
    bool              IsScrollingDown;
    bool              IsScrolling;
    int               Flasher;
    int               TopRowIndex; // scroll position, which row is topmost visible
    int               Scroller;
    int               Slid;
    int               LastSlid;
    int               CameoCount; // filled cameos
    BuildType         Cameos[75];

    /// VA: 0x006A8220.
    void Initialize(int index);

    /// VA: 0x006A8330.
    void Activate()
        { JMP_THIS(0x6A8330); }

    /// VA: 0x006A83E0.
    void Deactivate()
        { JMP_THIS(0x6A83E0); }

    /// VA: 0x006A93F0.
    void AddButtons()
        { JMP_THIS(0x6A93F0); }

    /// VA: 0x006A94B0.
    void RemoveButtons()
        { JMP_THIS(0x6A94B0); }
};

class NOVTABLE SidebarClass : public PowerClass
{
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static SidebarClass& Instance;

    // Original layout globals B0B4DC..B0B514. Each target supplies storage;
    // these references do not add fields to the original sidebar object.
    static Point2D& RepairPosition;
    static int& RepairPitch;
    static Point2D& TabPosition;
    static int& TabPitch;
    static Point2D& CameoPosition;
    static Point2D& CameoPitch;
    static int& CameoHeight;
    static Point2D& ScrollPosition;
    static Point2D& ScrollPitch;
    // Layout portions of 6A5090/6A5130 and 6ABD30, right-sidebar mode.
    // Requires the active Scenario and a published DSurface::ViewBounds.
    /// VA: 0x006A5030
#if defined(RA2_YRPP_GAME)
    void Init_Clear() override { JMP_THIS(0x6A5030); }
#else
    void Init_Clear() override;
#endif
    void UpdateLayout() noexcept;
    int GetUsableCameoCount() noexcept;

    DEFINE_ARRAY_REFERENCE(wchar_t, [0x42u], TooltipBuffer, 0xB07BC4u);
    /// Global VA: 0x00B0B3A0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(ShapeButtonClass, ToggleRepairButton, 0xB0B3A0);
    /// Global VA: 0x00B07DF8.
    DEFINE_REFERENCE(ShapeButtonClass, ToggleSellButton, 0xB07DF8);
#else
    static ShapeButtonClass& ToggleRepairButton;
    /// Global VA: 0x00B07DF8.
    static ShapeButtonClass& ToggleSellButton;
#endif
    void UpdateCommandButtons() noexcept;
    /// Global VA: 0x00B07C48.
    static ShapeButtonClass (&TabButtons)[4];

    void SidebarNeedsRepaint(int mode = 0) {
        this->SidebarNeedsRedraw = true;
        this->SidebarBackgroundNeedsRedraw = true;
        this->Tabs[this->ActiveTabIndex].AllowedToDraw = true;
        this->Tabs[this->ActiveTabIndex].NeedsRedraw = true;
        this->RedrawSidebar(mode);
        SidebarClass::Draw(1);
    }

    /// VA: 0x006A60A0.
#if defined(RA2_YRPP_GAME)
    void RepaintSidebar(int tab = 0)
        { JMP_THIS(0x6A60A0); }
#else
    void RepaintSidebar(int tab = 0);
#endif

    /// VA: 0x006A6300.
    bool AddCameo(AbstractType absType, int idxType);
    /// VA: 0x6A6140
    bool LinkFactory(FactoryClass* factory,AbstractType kind,int index);
    /// VA: 0x6ABAD0
    static bool YRPP_STDCALL UnlinkFactory(AbstractType kind,int index,FactoryClass* factory);
    // Native session invokes the original Strip production-completion path.
    void UpdateProduction();
    void RefreshBuildables();

    /// VA: 0x006A6C30.
#if defined(RA2_YRPP_GAME)
    void Draw(DWORD force) override { JMP_THIS(0x6A6C30); }
#else
    void Draw(DWORD force) override;
#endif

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~SidebarClass();

    // SidebarClass
    /// VA: 0x006A7D70.
    virtual bool Activate(int control) R0;

    // Non-virtual

    // which tab does the 'th object of that type belong in?
    /// VA: 0x006ABC60.
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL GetObjectTabIdx(AbstractType abs, int idxType, int unused)
        { JMP_STD(0x6ABC60); }
#else
    static int YRPP_FASTCALL GetObjectTabIdx(AbstractType abs, int idxType, int unused);
#endif

    // which tab does the 'th object of that type belong in?
    /// VA: 0x006ABCD0.
    static int YRPP_FASTCALL GetObjectTabIdx(AbstractType abs, BuildCat buildCat, bool isNaval)
        { JMP_STD(0x6ABCD0); }

    /// VA: 0x006A6A00.
    bool Scroll(bool up, int column)
        { JMP_THIS(0x6A6A00); }

    /// VA: 0x006A7590.
    int SetTab(int tabIndex);
    void InitializeButtons() noexcept;
    void ProcessButtonKey(DWORD key) noexcept;

    /// VA: 0x006A5F20.
    void OnTechnoDestroyed(TechnoClass* pTechno);

    /// VA: 0x006A70E0.
    void BlitSidebar(bool force)
        { JMP_THIS(0x6A70E0); }

    // enables or disables the two scroll buttons
    /// VA: 0x006A6610.
    void UpdateScrollButtons()
        { JMP_THIS(0x6A6610); }

    // how many cameo buttons fit on the strip at the current resolution
    /// VA: 0x006AC430.
    int GetVisibleCameoCount();

protected:
    // Constructor
    SidebarClass();

    // Properties

public:
    StripClass Tabs[0x4];
    DWORD unknown_5394;
    DWORD unknown_5398;
    int ActiveTabIndex;
    DWORD unknown_53A0;
    bool HideObjectNameInTooltip; // see 0x6A9343
    bool IsSidebarActive;
    bool SidebarNeedsRedraw;
    bool SidebarBackgroundNeedsRedraw;
    bool unknown_bool_53A8;

    // Information for the Diplomacy menu, I believe
    HouseClass* DiplomacyHouses[0x8];		//8 players max!
    int DiplomacyKills[0x8];		//total amount of kills per house
    int DiplomacyOwned[0x8];		//total amount of currently owned unit/buildings per house
    int DiplomacyPowerDrain[0x8];	//current power drain per house
    ColorScheme* DiplomacyColors[0x8];		//color scheme per house
    DWORD unknown_544C[0x8];			//??? per house - unused
    DWORD unknown_546C[0x8];			//??? per house - unused
    DWORD unknown_548C[0x8];			//??? per house - unused
    DWORD unknown_54AC[0x8];			//??? per house - unused
    DWORD unknown_54CC[0x8];			//??? per house - unused
    DWORD unknown_54EC[0x8];			//??? per house - unused
    BYTE unknown_550C;
    int DiplomacyNumHouses;			//possibly?

    bool unknown_bool_5514;
    bool unknown_bool_5515;
    PROTECTED_PROPERTY(BYTE, padding_5516[2]);
};
