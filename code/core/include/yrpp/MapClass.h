#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GScreenClass.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/CellClass.h"
#include "yrpp/Timer.h"
class AnimTypeClass;
class Straw;
class Pipe;
template<typename TKey, typename TValue> class HashTable;

class BulletTypeClass;
class ObjectClass;
class WarheadTypeClass;
class WeaponTypeClass;

// Terrain ground type
class GroundType
{
public:
    static GroundType (&Array)[12]; // Host storage or original slot 0x89EA40.

    static LandType YRPP_FASTCALL GetLandTypeFromName(const char* name);

    // Properties
    float Cost[8];  // Terrain speed multipliers.
    bool Buildable; // Can build on this terrain?
};

// Powerup crates
class Crate
{
public:
    // Properties
    CDTimerClass CrateTimer;
    CellStruct Location;
};

struct CellLevelPassabilityStruct
{
    char CellPassability;
    char CellLevel;
    unsigned short ZoneArrayIndex;
};

struct LevelAndPassabilityStruct2
{
    std::int16_t SubzoneIDs[3];
    std::int16_t ZoneID;
    char CellLevel;
    char Passability;
};

// ZoneConnectionClass - Holding zone connection info from tubes or bridges (probably used for pathfinding)
struct ZoneConnectionClass
{
    CellStruct	FromMapCoords;
    CellStruct	ToMapCoords;
    bool		IsPassable;
    int		ConnectionType; // 0 = bridge; not a CellClass pointer.

    // need to define a == operator so it can be used in array classes
    bool operator==(const ZoneConnectionClass &other) const {
        return (FromMapCoords == other.FromMapCoords
            && ToMapCoords == other.ToMapCoords
            && IsPassable == other.IsPassable
            && ConnectionType == other.ConnectionType);
    }
};

struct SubzoneConnectionStruct
{
    DWORD SubzoneID;
    BYTE IsCrossBlock;

    // need to define a == operator so it can be used in array classes
    bool operator==(const SubzoneConnectionStruct &other) const {
        return (SubzoneID == other.SubzoneID
            && IsCrossBlock == other.IsCrossBlock);
    }
};

struct SubzoneTrackingStruct
{
public:
    DynamicVectorClass<SubzoneConnectionStruct> SubzoneConnections;
    WORD ParentSubzoneID;
    DWORD Passability;
    DWORD ThreatRegion;

    // need to define a == operator so it can be used in array classes
    bool operator==(const SubzoneTrackingStruct &other) const {
        return (ParentSubzoneID != other.ParentSubzoneID
            && Passability == other.Passability
            && ThreatRegion == other.ThreatRegion);
    }
};

// helper class with static methods to detect projectile collisions
class TrajectoryHelper
{
public:
    // whether the bullet hit a cliff when moving from pBefore to pAfter
    /// VA: 0x004CC680.
    static bool YRPP_FASTCALL IsCliffHit(
        CellClass const* pSource, CellClass const* pBefore,
        CellClass const* pAfter)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x4CC680); }
#else
    ;
#endif

    // whether the bullet hit a wall when traversing through pCheck
    /// VA: 0x004CC6D0.
    static bool YRPP_FASTCALL IsWallHit(
        CellClass const* pSource, CellClass const* pCheck,
        CellClass const* pTarget, HouseClass const* pOwner)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x4CC6D0); }
#else
    ;
#endif

    // returns the cell at crdCur if it contains an obstacle, nullptr otherwise
    /// VA: 0x004CC360.
    static CellClass* YRPP_FASTCALL GetObstacle(
        CellClass const* pCellSource, CellClass const* pCellTarget,
        CellClass const* pCellBullet, CoordStruct crdCur,
        BulletTypeClass const* pType, HouseClass const* pOwner)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x4CC360); }
#else
    ;
#endif

    // assumes linear movement, returns the first cell that has a cliff or wall
    // in it, a nullptr otherwise.
    /// VA: 0x004CC100.
    static CellClass* YRPP_FASTCALL FindFirstObstacle(
        CoordStruct const& crdSrc, CoordStruct const& crdTarget,
        BulletTypeClass const* pType, HouseClass const* pOwner)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x4CC100); }
#else
    ;
#endif

    // if the warhead can destroy walls, walls don't count as obstacle
    /// VA: 0x004CC310.
    static CellClass* YRPP_FASTCALL FindFirstImpenetrableObstacle(
        CoordStruct const& crdSrc, CoordStruct const& crdTarget,
        WeaponTypeClass const* pWeapon, HouseClass const* pOwner)
#if defined(RA2_YRPP_GAME)
    { JMP_STD(0x4CC310); }
#else
    ;
#endif
};

class LayerClass : public DynamicVectorClass<ObjectClass*>
{
public:
    /// VA: 0x005519B0
    virtual bool AddObject(ObjectClass* pObject, bool sorted);

    virtual void RemoveAll()
        { this->Clear(); }

    virtual void vt_entry_24()
        { }

    /// VA: 0x00551B90
#if defined(RA2_YRPP_GAME)
    HRESULT Load(IStream* pStm)
        { JMP_THIS(0x551B90); }
#else
    HRESULT Load(IStream* pStm);
#endif

    /// VA: 0x00551B20
#if defined(RA2_YRPP_GAME)
    HRESULT Save(IStream* pStm)
        { JMP_THIS(0x551B20); }
#else
    HRESULT Save(IStream* pStm);
#endif

    /// VA: 0x00551A30
    void Sort();
};

class LogicClass : public LayerClass
{
public:
    // Separate from TagClass::Array: original deferred logic worklist.
    static DynamicVectorClass<TagClass*>& PendingTags; // 0x008B40C8
    /// Global VA: 0x0087F778.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(LogicClass, Instance, 0x87F778u)
#else
    static LogicClass& Instance;
#endif

    /// VA: 0x0055BAA0
    virtual bool AddObject(ObjectClass* pObject, bool sorted) override;

    /// VA: 0x0055B880.
#if defined(RA2_YRPP_GAME)
    virtual void PointerGotInvalid(AbstractClass* pInvalid, bool removed) { JMP_THIS(0x55B880); }
#else
    virtual void PointerGotInvalid(AbstractClass* pInvalid, bool removed);
#endif

    /// VA: 0x0055BAE0
    void RemoveObject(ObjectClass* pObject);

    /// VA: 0x0055AFB0.
#if defined(RA2_YRPP_GAME)
    void Update()
        { JMP_THIS(0x55AFB0); }
#else
    void Update();
#endif
};

class NOVTABLE MapClass : public GScreenClass
{
public:
    static DynamicVectorClass<TagClass*>& PendingTags; // 0x008B41A8, distinct from Logic
    // Static
    /// Global VA: 0x0087F7E8.
    static MapClass& Instance;

    /// Global VA: 0x00ABDC50.
    static CellClass& InvalidCell;

    static const int MaxCells = 0x40000;

    // this actually points to 5 vectors, one for each layer
    /// Global VA: 0x008A0360.
    static LayerClass (&ObjectsInLayers)[5];

    /// <summary>
    /// Some sort of hardcoded constant lookup matrix with rows (0-8) representing CellClass Passability(Type) and columns are MovementZones, used to determine pathfinding behaviour.
    /// </summary>
    /// Global VA: 0x0082A594.
#if defined(RA2_YRPP_GAME)
    DEFINE_ARRAY_REFERENCE(int, [13][8], MovementAdjustArray, 0x82A594u)
#else
    static int (&MovementAdjustArray)[13][8];
#endif

    static LayerClass* GetLayer(Layer lyr);

    /// VA: 0x0056BC50
    static int YRPP_STDCALL CellRegion(const CellStruct& cell)
        { return cell.X/4+130*(cell.Y/4)+131; }

    // IGameMap
    /// VA: unknown (legacy placeholder).
    virtual long YRPP_STDCALL Is_Visible(CellStruct cell) override R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~MapClass();

    // MapClass
    /// VA: 0x00565AA0; One_Time configured MaxNumCells, no live Cells.
    virtual void AllocateCells();
    /// VA: 0x00565B00; retains table, geometry and navigation buffers.
    virtual void DestructCells();
    /// VA: 0x00565BC0; reconstruct only after per-cell resources were cleared.
    virtual void ConstructCells();
    /// VA: 0x00577920
#if defined(RA2_YRPP_GAME)
    virtual void PointerGotInvalid(AbstractClass* ptr, bool removed) { JMP_THIS(0x577920); }
#else
    virtual void PointerGotInvalid(AbstractClass* ptr, bool removed);
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool DraggingInProgress() R0;
    /// VA: unknown (legacy placeholder).
    virtual void UpdateCrates() RX;
    /// VA: unknown (legacy placeholder).
    virtual void CreateEmptyMap(const RectangleStruct& mapRect, bool reuse, char nLevel, bool bUnk2) RX;
    /// VA: 0x00567230
#if defined(RA2_YRPP_GAME)
    virtual void SetVisibleRect(const RectangleStruct& mapRect) { JMP_THIS(0x00567230); }
#else
    virtual void SetVisibleRect(const RectangleStruct& mapRect);
#endif

    // Non-virtual
    // Fresh-map storage portion of 565C10, for loading a new world. Does not
    // resize a live world with entities. Failure leaves an empty map; rejects
    // dimensions that cannot fit the original 512-stride pointer table.
    bool CreateEmptyCells(const RectangleStruct& bounds, char level) noexcept;
    bool AllocateCellStorage() noexcept;
    // Owns the normally created Cells. Clears geometry and releases the table;
    // unlike DestructCells, this shutdown path does not allocate a replacement.
    void ReleaseCellStorage() noexcept;
    MapClass(const MapClass&) = delete;
    MapClass& operator=(const MapClass&) = delete;
    CellClass* TryGetCellAt(const CellStruct& MapCoords) const;

    CellClass* TryGetCellAt(const CoordStruct& Crd) const;

    CellClass* GetCellAt(const CellStruct &MapCoords) const;

    CellClass* GetCellAt(const CoordStruct &Crd) const;
    /// VA: 0x00586E50
    CellStruct* ClipToMap(CellStruct* output,const CellStruct& cell) const;

    /// VA: 0x00565730.
    CellClass* GetTargetCell(Point2D& location)
        { JMP_THIS(0x565730); }

    bool CellExists(const CellStruct &MapCoords) const;

    /// VA: 0x0056BCD0.
#if defined(RA2_YRPP_GAME)
    int GetThreatPosed(const CellStruct& cell, HouseClass* pHouse) const
        { JMP_THIS(0x56BCD0); }
#else
    int GetThreatPosed(const CellStruct& cell, HouseClass* pHouse) const;
#endif

    /// VA: 0x00586360.
    bool IsLocationShrouded(const CoordStruct &crd) const;
    bool IsLocationGapped(const CoordStruct& crd) const; // 5864A0, radar blackout/dimming.

    static int GetCellIndex(const CellStruct &MapCoords);

    // gets a coordinate in a random direction a fixed distance in leptons away from coords
    /// VA: 0x0049F420
    static CoordStruct* YRPP_FASTCALL GetRandomCoordsNear(CoordStruct &outBuffer, const CoordStruct &coords, int distance, bool center)
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x49F420);
    }
#else
    ;
#endif

    // gets a coordinate in a random direction a fixed distance in leptons away from coords
    static CoordStruct GetRandomCoordsNear(const CoordStruct &coords, int distance, bool center);

    /// VA: 0x004ACA10.
#if defined(RA2_YRPP_GAME)
    static CoordStruct* YRPP_STDCALL PickInfantrySublocation(CoordStruct &outBuffer, const CoordStruct &coords, bool ignoreContents = false)
        { JMP_STD(0x4ACA10); }
#else
    static CoordStruct* YRPP_STDCALL PickInfantrySublocation(CoordStruct& output,const CoordStruct& coords,bool ignoreContents=false);
#endif

    static CoordStruct PickInfantrySublocation(const CoordStruct &coords, bool ignoreContents = false);

    /// VA: 0x0048DC90.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL UnselectAll()
        { JMP_STD(0x48DC90); }
#else
    static void YRPP_FASTCALL UnselectAll();
#endif

    /// VA: 0x004AE290.
    void CenterMap()
        { JMP_THIS(0x4AE290); }

    void CellIteratorReset();

    CellClass* CellIteratorNext();

    // Resource stream portions of 56BAC0/56B3F0. Cells must already contain
    // live original cells; TileInsertions must match the selected tile catalog.
    // These do not construct a map or reset the game's global InvalidCell.
    // Read failure may leave preceding complete records applied. Write returns
    // compressed byte count, or -1 on compression failure.
    bool ReadIsoMapPack5(Straw& source);
    int WriteIsoMapPack5(Pipe& destination) const;
    // Earlier resource formats consumed in order by Display::LoadFromINI:
    // 128x128 LCW planes; LCW records; raw records; LZO records without ice.
    bool ReadIsoMapPack(Straw& source);  // 56B5A0
    bool ReadIsoMapPack2(Straw& source); // 56B780
    bool ReadIsoMapPack3(Straw& source); // 56B8A0
    bool ReadIsoMapPack4(Straw& source); // 56B9A0

    /// VA: 0x0056D230.
#if defined(RA2_YRPP_GAME)
    int GetMovementZoneType(const CellStruct& MapCoords, MovementZone movementZone, bool isBridge)
        { JMP_THIS(0x56D230); }
#else
    int GetMovementZoneType(const CellStruct& cell, MovementZone movementZone, bool isBridge);
#endif
    /// VA: 0x0056D100
    static bool YRPP_STDCALL IsSameCellZone(const CellStruct& from, const CellStruct& to,
        MovementZone movementZone, bool fromBridge, bool toBridge, bool allowLeavingMap);
    /// VA: 0x0056D3F0
    int GetCellZoneIndex(const CellStruct& cell) const;
    /// VA: 0x0056DA10
    int ZoneConnectionIndex(const CellStruct& cell, int distance, int start) const;
    /// VA: 0x00583180
    static CellStruct* YRPP_STDCALL GetBridgeZoneConnectionCell(CellStruct* output, CellClass* cell, bool bridge);
    /// VA: 0x005835D0
    static CellStruct* YRPP_STDCALL FindBridgeSpanEndCell(CellStruct* output, const CellStruct& cell, const CellStruct& reference);
    /// VA: 0x00583820
    CellStruct* FindBridgeEndCellForSubzone(CellStruct* output, const CellStruct& cell, int level, int subzone);
    /// VA: 0x00585F40
    static int YRPP_STDCALL RegionThreat(HouseClass* house, int level, int fromSubzone, int toSubzone);
    /// VA: 0x005840C0
    bool BuildReachableSubzones(CellClass* cell, int level, DynamicVectorClass<unsigned short>& unreachable, const FootClass* foot);
    /// VA: 0x0056C510
    int ResetAllZones();
    /// VA: 0x0056CB90
    int ZoneSpan(CellLevelPassabilityStruct* seed, int zone, int& skip);
    /// VA: 0x0056D6E0
    void ComputeZoneConnections();
    /// VA: 0x00581F50
    void ResetAllSubzones();
    /// VA: 0x00581F90
    void ResetSubzone(int level);
    /// VA: 0x005824A0
    int SubzoneSpan(LevelAndPassabilityStruct2* seed, int level, int subzone, const RectangleStruct& bounds, const CellStruct& cell);
    /// VA: 0x00582D30
    void RegisterSubzoneZoneConnections(int level);
    /// VA: 0x00582D70
    void RegisterZoneConnectionEntries(const ZoneConnectionClass& connection, int level);

// the key damage delivery
/*! The key damage delivery function.
    \param Coords Location of the impact/center of damage.
    \param Damage Amount of damage to deal.
    \param SourceObject The object which caused the damage to be delivered (iow, the shooter).
    \param WH The warhead to use to apply the damage.
    \param AffectsTiberium If this is false, Tiberium=yes is ignored.
    \param SourceHouse The house to which SourceObject belongs, the owner/bringer of damage.
*/
    /// VA: 0x00489280
    static DamageAreaResult YRPP_FASTCALL DamageArea(
        const CoordStruct& Coords,
        int Damage,
        TechnoClass* SourceObject,
        WarheadTypeClass *WH,
        bool AffectsTiberium,
        HouseClass* SourceHouse)
#if defined(RA2_YRPP_GAME)
            { JMP_STD(0x489280); }
#else
            ;
#endif

    /*
     * Picks the appropriate anim from WH's AnimList= based on damage dealt and land type (Conventional= )
     * so after DamageArea:
     * if(AnimTypeClass *damageAnimType = SelectDamageAnimation(...)) {
     * 	GameCreate<AnimClass>(damageAnimType, location);
     * }
     */
    /// VA: 0x0048A4F0
    static AnimTypeClass * YRPP_FASTCALL SelectDamageAnimation
        (int Damage, WarheadTypeClass *WH, LandType LandType, const CoordStruct& coords)
#if defined(RA2_YRPP_GAME)
            { JMP_STD(0x48A4F0); }
#else
            ;
#endif

    /// VA: 0x00587180
    bool DamageBridgeAt(const CellStruct& cell) { JMP_THIS(0x587180); }
    /// VA: 0x0057BAA0
    bool DamageLowBridgeAt(const CellStruct& cell) { JMP_THIS(0x57BAA0); }
    /// VA: 0x0057CCF0
    bool DamageLowWoodBridgeAt(const CellStruct& cell) { JMP_THIS(0x57CCF0); }

    /// VA: 0x0048A620
    static void YRPP_FASTCALL FlashbangWarheadAt
        (int Damage, WarheadTypeClass *WH, CoordStruct coords, bool Force = 0, SpotlightFlags CLDisableFlags = SpotlightFlags::None)
#if defined(RA2_YRPP_GAME)
            {JMP_STD(0x48A620); }
#else
            ;
#endif

    // get the damage a warhead causes to specific armor
    /// VA: 0x00489180.
#if defined(RA2_YRPP_GAME)
    static int YRPP_FASTCALL GetTotalDamage(int damage, const WarheadTypeClass* pWarhead, Armor armor, int distance) { JMP_STD(0x489180); }
#else
    static int YRPP_FASTCALL GetTotalDamage(int damage, const WarheadTypeClass* pWarhead, Armor armor, int distance);
#endif

    /// VA: 0x00578080.
#if defined(RA2_YRPP_GAME)
    int GetCellFloorHeight(const CoordStruct& crd) const
        { JMP_THIS(0x578080); }
#else
    int GetCellFloorHeight(const CoordStruct& crd) const;
#endif

    /// VA: 0x004AA440.
#if defined(RA2_YRPP_GAME)
    CellStruct * PickCellOnEdge(CellStruct &buffer, Edge Edge, const CellStruct &CurrentLocation, const CellStruct &Fallback,
        SpeedType SpeedType, bool ValidateReachability, MovementZone MovZone) const
            { JMP_THIS(0x4AA440); }
#else
    CellStruct* PickCellOnEdge(CellStruct& buffer, Edge edge, const CellStruct& current, const CellStruct& fallback,
        SpeedType speed, bool validate, MovementZone zone) const;
#endif

    CellStruct PickCellOnEdge(Edge Edge, const CellStruct &CurrentLocation, const CellStruct &Fallback,
        SpeedType SpeedType, bool ValidateReachability, MovementZone MovZone) const;

// Pathfinding voodoo
// do not touch them, mmkay, they trigger ZoneConnection recalc which is a MUST for firestorm to work

    /// VA: 0x0056C510.
    void Update_Pathfinding_1()
        { JMP_THIS(0x56C510); }

    /// VA: 0x00586990.
    void Update_Pathfinding_2(const DynamicVectorClass<CellStruct> &where)
        { JMP_THIS(0x586990); }

    /// VA: 0x00586FC0
#if defined(RA2_YRPP_GAME)
    CellStruct* ClosestPassableCell(CellStruct* output,const CellStruct& target,const CellStruct& reference) { JMP_THIS(0x586FC0); }
#else
    CellStruct* ClosestPassableCell(CellStruct* output,const CellStruct& target,const CellStruct& reference);
#endif

    // Find nearest spot
    /// VA: 0x0056DC20.
#if defined(RA2_YRPP_GAME)
    CellStruct* NearByLocation(CellStruct &outBuffer, const CellStruct &position, SpeedType SpeedType, int a5, MovementZone MovementZone, bool alt, int SpaceSizeX, int SpaceSizeY, bool disallowOverlay, bool a11, bool requireBurrowable, bool allowBridge, const CellStruct &closeTo, bool a15, bool buildable)
        { JMP_THIS(0x56DC20); }
#else
    CellStruct* NearByLocation(CellStruct& output, const CellStruct& position,
        SpeedType speed, int zone, MovementZone movement, bool checkBridge,
        int width, int height, bool disallowOverlay, bool checkHeight,
        bool requireBurrowable, bool allowBridge, const CellStruct& closeTo,
        bool southeastOnly, bool buildable);
#endif

    CellStruct NearByLocation(const CellStruct &position, SpeedType SpeedType, int a5, MovementZone MovementZone, bool alt, int SpaceSizeX, int SpaceSizeY, bool disallowOverlay, bool a11, bool requireBurrowable, bool allowBridge, const CellStruct &closeTo, bool a15, bool buildable);

    /// VA: 0x005683C0.
    void AddContentAt(CellStruct* coords, ObjectClass* content);

    /// VA: 0x005687F0.
    void RemoveContentAt(CellStruct* coords, ObjectClass* content);

    /// VA: 0x00578460.
    bool IsWithinUsableArea(const CellStruct& cell, bool checkLevel) const;
    // Convenience wrapper for 578460's checkLevel=false branch.
    bool IsWithinUsableArea2D(const CellStruct& cell) const;

    /// VA: 0x00578540.
#if defined(RA2_YRPP_GAME)
    bool IsWithinUsableArea(CellClass* pCell, bool checkLevel) const
        { JMP_THIS(0x578540); }
#else
    bool IsWithinUsableArea(CellClass* pCell, bool checkLevel) const;
#endif

    /// VA: 0x005785F0.
    bool IsWithinUsableArea(const CoordStruct& coords) const
        { return IsWithinUsableArea(CellStruct{short(coords.X/256),short(coords.Y/256)},true); }

    /// VA: 0x00568300.
#if defined(RA2_YRPP_GAME)
    bool CoordinatesLegal(const CellStruct& cell) const
        { JMP_THIS(0x568300); }
#else
    bool CoordinatesLegal(const CellStruct& cell) const;
#endif

    // In_Map_Coord: the same bounds test, in leptons
    /// VA: 0x00568350.
#if defined(RA2_YRPP_GAME)
    bool CoordinatesLegal(const CoordStruct& coords) const
        { JMP_THIS(0x568350); }
#else
    bool CoordinatesLegal(const CoordStruct& coords) const;
#endif

    /// VA: 0x00587410.
    bool IsLinkedBridgeDestroyed(const CellStruct& cell) const
        { JMP_THIS(0x587410); }

    /// VA: 0x0056BEC0.
    bool PlacePowerupCrate(CellStruct cell, Powerup type)
        { JMP_THIS(0x56BEC0); }

    // Called from Nearby_Location and AI build location evaluation.
    // Checks bunch of stuff like do cells in rectangle have terrain/buildings, overlays, ramp etc.
    /// VA: 0x00586780.
#if defined(RA2_YRPP_GAME)
    bool IsAreaFree(RectangleStruct* pRect, int houseID)
        { JMP_THIS(0x586780); }
#else
    bool IsAreaFree(RectangleStruct* rect, int houseID);
#endif

    // Checks if the area is in the visible portion of map.
    /// VA: 0x00578390.
#if defined(RA2_YRPP_GAME)
    bool InLocalRadar(RectangleStruct* pRect, bool checkLevel)
        { JMP_THIS(0x578390); }
#else
    bool InLocalRadar(RectangleStruct* rect, bool checkLevel);
#endif

// ====================================
//         FIRESTORM RELATED
// ====================================

    /// VA: 0x005880A0
    CoordStruct* FindFirstFirestorm(
        CoordStruct* pOutBuffer, const CoordStruct& start,
        const CoordStruct& end, HouseClass const* pHouse = nullptr) const
#if defined(RA2_YRPP_GAME)
    { JMP_THIS(0x5880A0); }
#else
    ;
#endif

    CoordStruct FindFirstFirestorm(
        const CoordStruct& start, const CoordStruct& end,
        HouseClass const* pHouse = nullptr) const;

// ====================================
//        MAP REVEAL BRAINDAMAGE
// ====================================

/*
 * TechnoClass::Fire uses this for RevealOnFire on player's own units (radius = 3)
 * TechnoClass::See uses this on all (singleCampaign || !MultiplayPassive) units
 * TalkBubble uses this to display the unit to the player
 */
    /// VA: 0x005673A0
    void RevealArea1(CoordStruct* coords, int radius, HouseClass* house,
                     BYTE incremental, BYTE dontMap, BYTE unfog,
                     BYTE byHeight, BYTE reveal)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x5673A0); }
#else
        ;
#endif

/*
 * these come in pairs - first the last argument is 0 and then 1

 * AircraftClass::Fire - reveal the target area to the owner (0,0,0,1,x)
 * AircraftClass::See - reveal shroud when on the ground (arg,arg,0,1,x), and fog always (0,0,1,(height < flightlevel/2),x)
 * AnimClass::AnimClass - reveal area to player if anim->Type = [General]DropZoneAnim= (radius = Rules->DropZoneRadius /256) (0,0,0,1,x)
 * BuildingClass::Place - reveal (r = 1) to player if this is ToTile and owned by player (0,0,0,1,x)
 * BuildingClass::Unlimbo - reveal (radius = this->Type->Sight ) to owner (0,0,0,1,x)
 * PsychicReveal launch - reveal to user (0,0,0,0,x)
 * ActionClass::RevealWaypoint - reveal RevealTriggerRadius= to player (0,0,0,1,x)
 * ActionClass::RevealZoneOfWaypoint - reveal (r = 2) to player (0,0,0,1,x)
 */
    /// VA: 0x005678E0
    void RevealArea2(CoordStruct* coords, int radius, HouseClass* house,
                     BYTE incremental, int unused, BYTE unfog,
                     BYTE byHeight, BYTE increaseShroudCounter)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x5678E0); }
#else
        ;
#endif

/*
 * AircraftClass::SpyPlaneApproach
 * AircraftClass::SpyPlaneOverfly
 * AircraftClass::Carryall_Unload
 * BuildingClass::Place - RevealToAll
 * Foot/Infantry Class::Update/UpdatePosition
 * MapClass::RevealArea0 calls this to do the work
 * ParasiteClass::Infect/PointerGotInvalid
 * TechnoClass::Unlimbo
 * TechnoClass::Fire uses this (r = 4) right after using RevealArea0, wtfcock
 */
    /// VA: 0x00567DA0.
#if defined(RA2_YRPP_GAME)
    void RevealArea3(CoordStruct *Coords, int Height, int Radius, bool SkipReveal)
        { JMP_THIS(0x567DA0); }
#else
    void RevealArea3(CoordStruct* coords,int startRadius,int radius,bool skipReveal);
#endif

    /// VA: 0x00577D90.
    void Reveal(HouseClass* pHouse)
        { JMP_THIS(0x577D90); }

    /// VA: 0x00577AB0.
    void Reshroud(HouseClass* pHouse)
        { JMP_THIS(0x577AB0); }

    /// VA: 0x00578080.
    int GetZPos(CoordStruct *Coords)
        { JMP_THIS(0x578080); }

    // these two VERY slowly reprocess the map after gapgen state changes
    /// VA: 0x00657CE0.
    void sub_657CE0()
        { JMP_THIS(0x657CE0); }

    /// VA: 0x004F42F0.
    void RedrawSidebar(int mode)
        { JMP_THIS(0x4F42F0); }

    /// VA: 0x004AA2B0.
    ObjectClass* NextObject(ObjectClass* pCurrentObject)
        { JMP_THIS(0x4AA2B0); }

    /// VA: 0x004AC820.
    void SetTogglePowerMode(int mode)
        { JMP_THIS(0x4AC820); }

    /// VA: 0x004AC960.
    void SetPlaceBeaconMode(int mode)
        { JMP_THIS(0x4AC960); }

    /// VA: 0x004AC700.
    void SetWaypointMode(int mode, bool somebool)
        { JMP_THIS(0x4AC700); }

    /// VA: 0x00581140.
    void DestroyCliff(CellClass *Cell)
        { JMP_THIS(0x581140); }

    /// VA: 0x005865E0.
    bool IsLocationFogged(const CoordStruct& coord)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x5865E0); }
#else
        ;
#endif
    bool IsLocationFogged(CoordStruct&& coord);

    // Original entry ignores ECX and pops all three arguments (ret 0x0C).
    /// VA: 0x005865F0
    static void YRPP_STDCALL RevealCheck(CellClass* cell, HouseClass* house, bool newlyMapped)
#if defined(RA2_YRPP_GAME)
        { JMP_STD(0x5865F0); }
#else
        ;
#endif

    // returns false if visitor should wait for a gate to open, true otherwise
    /// VA: 0x00578AD0.
    bool MakeTraversable(ObjectClass const* pVisitor, CellStruct const& cell) const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x578AD0); }
#else
        ;
#endif

    /// VA: 0x00588570.
    void BuildingToFirestormWall(CellStruct const& cell,HouseClass* pHouse,BuildingTypeClass* pBldType)
        { JMP_THIS(0x588570); }

    /// VA: 0x00588750.
    void BuildingToWall(CellStruct const& cell, HouseClass* pHouse, BuildingTypeClass* pBldType)
        { JMP_THIS(0x588750); }

    // Called on wall state updates etc. when the wall hasn't been removed.
    /// VA: 0x0056D5A0
    void RecalculateZones(CellStruct const& cell)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x56D5A0);}
#else
        ;
#endif

    // Called on wall state updates etc. when the wall HAS been removed.
    /// VA: 0x0056D460
    void ResetZones(CellStruct const& cell)
#if defined(RA2_YRPP_GAME)
    { JMP_THIS(0x56D460); }
#else
        ;
#endif

    // Called on wall state updates etc
    /// VA: 0x00584550
    void RecalculateSubZones(CellStruct const& cell)
#if defined(RA2_YRPP_GAME)
    { JMP_THIS(0x584550); }
#else
        ;
#endif

    /// VA: 0x00570050.
    void RepairWoodBridgeAt(CellStruct const& cell)
    { JMP_THIS(0x570050); }

    /// VA: 0x00573540.
    void RepairConcreteBridgeAt(CellStruct const& cell)
    { JMP_THIS(0x573540); }

    /// VA: 0x00574C20.
    void DestroyWoodBridgeAt(CellStruct const& cell)
    { JMP_THIS(0x574C20); }

    /// VA: 0x00574000.
    void DestroyConcreteBridgeAt(CellStruct const& cell)
    { JMP_THIS(0x574000); }

protected:
    // Constructor
    MapClass();	//don't need this

    // Properties

public:
    DWORD unknown_10;
    HashTable<DWORD,DWORD>* unknown_pointer_14;
    void* MovementZones [13];
    DWORD somecount_4C;
    DynamicVectorClass<ZoneConnectionClass> ZoneConnections;
    CellLevelPassabilityStruct* LevelAndPassability;
    int ValidMapCellCount;
    LevelAndPassabilityStruct2* LevelAndPassabilityStruct2pointer_70;
    int SubzoneTrackingCounts[3];
    HashTable<DWORD, SubzoneConnectionStruct>* unknown_80[3]; // somehow connected to the 3 vectors below
    DynamicVectorClass<SubzoneTrackingStruct> SubzoneTracking[3];
    DynamicVectorClass<CellStruct> CellStructs1;
    RectangleStruct MapRect;
    RectangleStruct VisibleRect;
    int CellIterator_NextX;
    int CellIterator_NextY;
    int CellIterator_CurrentY;
    CellClass** CellIterator_NextCell; // pointer to a slot in Cells.Items (0x578290)
    int ZoneIterator_X;
    int ZoneIterator_Y;
    LTRBStruct MapCoordBounds; // the minimum and maximum cell struct values
    int TotalValue;
    VectorClass<CellClass*> Cells;
    int MaxLevel;
    int MaxWidth;
    int MaxHeight;
    int MaxNumCells;
    Crate Crates [0x100];
    BOOL Redraws;
    DynamicVectorClass<CellStruct> TaggedCells;
};
