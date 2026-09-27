#pragma once
#include "yrpp/platform/ABI.h"

#include "yrpp/MapClass.h"

class CCINIClass;
class ObjectTypeClass;
class WaypointClass;

class NOVTABLE DisplayClass : public MapClass
{
public:
    // Static
    /// Global VA: 0x0087F7E8.
    static DisplayClass& Instance;

    // WIP: DisplayClass::TacticalClass goes HERE

    /// VA: 0x00692300.
    bool ProcessClickCoords(Point2D *src, CellStruct *XYdst, CoordStruct *XYZdst, ObjectClass **Target, BYTE *a5, BYTE *a6)
        { JMP_THIS(0x692300); }

    // the foundation for placement with green/red
    // Original source spans: 120 cells for the live cursor, 50 for its pending
    // placement copy, each including an EOL (0x7FFF,0x7FFF) entry. Borrowed input
    // is copied into the shared original scratch buffers; null clears it.
    /// VA: 0x004A8BF0
#if defined(RA2_YRPP_GAME)
    void  SetActiveFoundation(const CellStruct *Coords)
        { JMP_THIS(0x4A8BF0); }
#else
    void SetActiveFoundation(const CellStruct* coords) noexcept;
#endif
    /// VA: 0x004A8D50
#if defined(RA2_YRPP_GAME)
    void SetActiveFoundationCopy(const CellStruct* coords) { JMP_THIS(0x4A8D50); }
#else
    void SetActiveFoundationCopy(const CellStruct* coords) noexcept;
#endif
    /// Global VA: 0x008A041C.
    static CellStruct (&ActiveFoundationBuffer)[120];
    /// Global VA: 0x008A0298.
    static CellStruct (&PendingFoundationBuffer)[50];

    // Building Adjacent etc. check. Pretty much always called with:
    // foundationData = DisplayClass::CurrentFoundation_Data
    // currentPosition = DisplayClass::CurrentFoundation_CenterCell + DisplayClass::CurrentFoundation_TopLeftOffset
    /// VA: 0x004A8EB0
#if defined(RA2_YRPP_GAME)
    bool PassesProximityCheck(ObjectTypeClass* pType, int houseArrayIndex, CellStruct* foundationData, CellStruct* currentPosition)
        { JMP_THIS(0x4A8EB0); }
#else
    bool PassesProximityCheck(ObjectTypeClass* type, int houseArrayIndex,
                              CellStruct* foundationData, CellStruct* position);
#endif

    // Original independent shroud check; does not set the active foundation.
    /// VA: 0x004A9070
#if defined(RA2_YRPP_GAME)
    bool PassesShroudCheck(ObjectTypeClass* type, int houseArrayIndex,
                           CellStruct* foundationData, CellStruct* position) { JMP_THIS(0x4A9070); }
#else
    bool PassesShroudCheck(ObjectTypeClass* type, int houseArrayIndex,
                           CellStruct* foundationData, CellStruct* position);
#endif

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~DisplayClass();

    // GScreenClass
    // MapClass
    // DisplayClass
    /// VA: 0x004AE6F0
#if defined(RA2_YRPP_GAME)
    virtual HRESULT Load(IStream* pStm) { JMP_THIS(0x4AE6F0); }
#else
    virtual HRESULT Load(IStream* pStm);
#endif
    /// VA: 0x004AE720
#if defined(RA2_YRPP_GAME)
    virtual HRESULT Save(IStream* pStm) { JMP_THIS(0x4AE720); }
#else
    virtual HRESULT Save(IStream* pStm);
#endif
    /// VA: unknown (legacy placeholder).
    virtual void LoadFromINI(CCINIClass* pINI) RX; //Loads the map from a map file.
    // Fresh terrain portion of 4ACE70. Caller supplies the Scenario scope and
    // mounted theater packages. Loads the catalog after constructing the cells;
    // all five pack formats apply in original order.
    // Failure releases this map's cells; full entity/UI startup remains separate.
    bool LoadTerrainFromINI(CCINIClass& ini) noexcept;
    /// VA: 0x004AE4F0
#if defined(RA2_YRPP_GAME)
    virtual const wchar_t* GetToolTip(UINT nDlgID) { JMP_THIS(0x4AE4F0); }
#else
    virtual const wchar_t* GetToolTip(UINT nDlgID);
#endif
    /// VA: 0x004AE6B0
#if defined(RA2_YRPP_GAME)
    virtual void InitGUI() { JMP_THIS(0x004AE6B0); }
#else
    virtual void InitGUI();
#endif
    /// VA: unknown (legacy placeholder).
    virtual void ClearDragBand() RX;
    /// VA: 0x004A9890
#if defined(RA2_YRPP_GAME)
    virtual bool MapCell(CellStruct* cell, HouseClass* house) { JMP_THIS(0x004A9890); }
#else
    virtual bool MapCell(CellStruct* cell, HouseClass* house);
#endif
    /// VA: 0x004A9CA0
#if defined(RA2_YRPP_GAME)
    virtual bool RevealFogShroud(CellStruct* cell, HouseClass* house, bool increaseShroudCounter) { JMP_THIS(0x004A9CA0); }
#else
    virtual bool RevealFogShroud(CellStruct* cell, HouseClass* house, bool increaseShroudCounter);
#endif
    /// VA: 0x004A9DD0
#if defined(RA2_YRPP_GAME)
    virtual bool MapCellFoggedness(CellStruct* cell, HouseClass* house) { JMP_THIS(0x004A9DD0); }
#else
    virtual bool MapCellFoggedness(CellStruct* cell, HouseClass* house);
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool MapCellVisibility(CellStruct* pMapCoord, HouseClass* pHouse) R0;
    /// VA: implementation-defined (pure virtual).
    virtual MouseCursorType GetLastMouseCursor() = 0;
    /// VA: unknown (legacy placeholder).
    virtual bool ScrollMap(DWORD dwUnk1, DWORD dwUnk2, DWORD dwUnk3) R0;
    /// VA: 0x004A8960
#if defined(RA2_YRPP_GAME)
    virtual void Set_View_Dimensions(const RectangleStruct& rect) { JMP_THIS(0x4A8960); }
#else
    virtual void Set_View_Dimensions(const RectangleStruct& rect);
#endif
    /// VA: 0x004AEBD0
#if defined(RA2_YRPP_GAME)
    void Draw(DWORD force) override { JMP_THIS(0x4AEBD0); }
#else
    void Draw(DWORD force) override;
#endif
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_AC(DWORD dwUnk) RX;
    /// VA: unknown (legacy placeholder).
    virtual void RightMouseButtonClick(Point2D* pPoint) RX;
    /// VA: unknown (legacy placeholder).
    virtual void LeftMouseButtonClick(Point2D* pPoint) RX;

    // Decides which mouse pointer to set and then does it.
    // Mouse is over cell pMapCoords which is bShrouded and holds pObject.
    // Requires the current House and loaded MOUSE_PAL; virtual cursor/actor
    // calls must not propagate exceptions across the original input boundary.
    /// VA: 0x004AAE90
#if defined(RA2_YRPP_GAME)
    virtual bool ConvertAction(const CellStruct& cell, bool bShrouded, ObjectClass* pObject, Action action, bool dwUnk) { JMP_THIS(0x4AAE90); }
#else
    virtual bool ConvertAction(const CellStruct& cell, bool bShrouded, ObjectClass* pObject, Action action, bool dwUnk);
#endif
    /// VA: unknown (legacy placeholder).
    virtual void LeftMouseButtonDown(const Point2D& point) RX;
    /// VA: 0x004AB9B0
#if defined(RA2_YRPP_GAME)
    virtual void LeftMouseButtonUp(const CoordStruct& coords, const CellStruct& cell, ObjectClass* pObject, Action action, DWORD dwUnk2) { JMP_THIS(0x4AB9B0); }
#else
    // Requires initialized input, player, tactical view and cursor services.
    // Shared virtual/audio callbacks must not propagate exceptions; a failed
    // native allocation remains fatal at this original input boundary.
    virtual void LeftMouseButtonUp(const CoordStruct& coords, const CellStruct& cell,
                                  ObjectClass* object, Action action, DWORD miniMap) noexcept;
#endif
    /// VA: 0x004AAD30
    virtual void RightMouseButtonUp(DWORD dwUnk) { JMP_THIS(0x4AAD30); }

    // Non-virtual
    /// VA: 0x004AE750
    void ActiveClick(ObjectClass* object,CellStruct cell,Action action);

    /// VA: 0x004AC2B0
    static void YRPP_FASTCALL BandboxSelectionCallback(ObjectClass* object) noexcept;

    /// VA: 0x004AC960
#if defined(RA2_YRPP_GAME)
    void SetBeaconMode(int mode) { JMP_THIS(0x4AC960); }
#else
    void SetBeaconMode(int mode) noexcept;
#endif

    /// VA: 0x004AC660
#if defined(RA2_YRPP_GAME)
    void SetSellMode(int mode) { reinterpret_cast<void (YRPP_THISCALL*)(DisplayClass*,int)>(0x4AC660)(this,mode); }
#else
    void SetSellMode(int mode) noexcept;
#endif
    /// VA: 0x004AC8C0
#if defined(RA2_YRPP_GAME)
    void SetRepairMode(int mode) { reinterpret_cast<void (YRPP_THISCALL*)(DisplayClass*,int)>(0x4AC8C0)(this,mode); }
#else
    void SetRepairMode(int mode) noexcept;
#endif

    /// VA: 0x00692610.
    Action DecideAction(const CellStruct& cell, ObjectClass* pObject, DWORD dwUnk)
        { JMP_THIS(0x692610); }

    /* pass in CurrentFoundationData and receive the width/height of a bounding rectangle in cells */
    /// VA: 0x004A94F0.
    CellStruct* FoundationBoundsSize(CellStruct& outBuffer, CellStruct const* const pFoundationData) const;

    /// VA: unknown.
    CellStruct FoundationBoundsSize(CellStruct const* const pFoundationData) const;

    /* marks or unmarks the cells pointed to by CurrentFoundationData as containing a building */
    /// VA: 0x004A95A0
#if defined(RA2_YRPP_GAME)
    void MarkFoundation(CellStruct * BaseCell, bool Mark)
        { JMP_THIS(0x4A95A0); }
#else
    void MarkFoundation(CellStruct* baseCell, bool mark) noexcept;
#endif
    /// VA: 0x004A9650
#if defined(RA2_YRPP_GAME)
    void MarkFoundationCopy(CellStruct* baseCell, bool mark) { JMP_THIS(0x4A9650); }
#else
    void MarkFoundationCopy(CellStruct* baseCell, bool mark) noexcept;
#endif

    // Submit object to layer.
    /// VA: 0x004A9720.
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_STDCALL Submit(ObjectClass* pObject) { JMP_STD(0x4A9720); }
#else
    static void YRPP_STDCALL Submit(ObjectClass* pObject);
#endif

    // Remove object from layer.
    /// VA: 0x004A9770.
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) static void YRPP_STDCALL Remove(ObjectClass* pObject) { JMP_STD(0x4A9770); }
#else
    static void YRPP_STDCALL Remove(ObjectClass* pObject);
#endif

protected:
    // Constructor
    /// VA: unknown.
    DisplayClass();	//don't need this

    // Properties

public:
    CellStruct CurrentFoundation_CenterCell;	//Currently placing the building here
    CellStruct CurrentFoundation_TopLeftOffset;		// offset from center cell of the current foundation (under the mouse) to the top left cell
    CellStruct* CurrentFoundation_Data;	//Foundation data of the building we're currently placing (note: limited to 120 cells)
    bool unknown_1180;
    bool unknown_1181;
    CellStruct CurrentFoundationCopy_CenterCell; // All the Copies are used in the time between clicking and actual execution
    CellStruct CurrentFoundationCopy_TopLeftOffset;
    CellStruct * CurrentFoundationCopy_Data; // (note: limited to 50 [!] cells)
    ObjectClass *CurrentBuildingCopy;
    ObjectTypeClass *CurrentBuildingTypeCopy;
    int CurrentBuildingOwnerArrayIndexCopy;
    bool FollowObject;
    ObjectClass* ObjectToFollow;
    ObjectClass* CurrentBuilding;		//Building we're currently placing
    ObjectTypeClass* CurrentBuildingType;	//Type of that building
    int CurrentBuildingOwnerArrayIndex;
    bool RepairMode;
    bool SellMode;
    bool PowerToggleMode;
    bool PlanningMode;
    bool PlaceBeaconMode;
    int CurrentSWTypeIndex;	//Index of the SuperWeaponType we have currently selected
    WaypointClass* DraggedWaypoint;
    CoordStruct DraggedWaypointCoords;
    // Original cached cursor color is RGB bytes, not three bools.
    BYTE WaypointColorRed;
    BYTE WaypointColorGreen;
    BYTE WaypointColorBlue;
    bool LeftPressAndDraggingRectangle;
    bool unknown_bool_11D0;
    bool unknown_bool_11D1;
    Point2D LeftDownPosition;
    Point2D unknown_11DC;
    PROTECTED_PROPERTY(DWORD, padding_11E4);
};
