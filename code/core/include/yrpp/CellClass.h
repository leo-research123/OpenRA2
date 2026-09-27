/*
    Cells
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

// forward declarations
class ObjectClass;
class TechnoClass;
class BuildingClass;
class BuildingTypeClass;
class UnitClass;
class InfantryClass;
class AircraftClass;
class TerrainClass;
class LightConvertClass;
class RadSiteClass;
class FootClass;
class TubeClass;
class FoggedObjectClass;
class TagClass;
class TiberiumClass;
class PixelFXClass;
class IsometricTileTypeClass;

class CellClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Cell;

    static constexpr int BridgeLevels = 4;

    // Controlled normal construction using the core's game allocator.
    // Returns null on allocation failure; release with GameDelete.
    static CellClass* Create() noexcept;

    // the height of a bridge in leptons
    // see ABC5DC, AC13BC
    static constexpr int BridgeHeight = BridgeLevels * Unsorted::LevelHeight;

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override R0;

    // Destructor
    /// VA: 0x0047BB60.
    virtual ~CellClass();

    // AbstractClass
    /// VA: 0x00485130
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x485130); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    /// VA: 0x004867E0
#if defined(RA2_YRPP_GAME)
    bool IsOnFloor() const override { JMP_THIS(0x4867E0); }
#else
    bool IsOnFloor() const override;
#endif
    using AbstractClass::GetCoords;
    using AbstractClass::GetCenterCoords;
    virtual CoordStruct* GetCoords(CoordStruct* dest) const override;
    virtual CoordStruct* GetCenterCoords(CoordStruct* dest) const override;

    // non-virtual

    // get content objects
    /// VA: 0x0047C3D0.
#if defined(RA2_YRPP_GAME)
    TechnoClass* FindTechnoNearestTo(Point2D const& offsetPixel, bool alt, TechnoClass const* pExcludeThis = nullptr) const
        { JMP_THIS(0x47C3D0); }
#else
    TechnoClass* FindTechnoNearestTo(Point2D const& offsetPixel, bool alt, TechnoClass const* pExcludeThis = nullptr) const;
#endif

    /// VA: 0x0047C4D0.
#if defined(RA2_YRPP_GAME)
    ObjectClass* FindObjectOfType(AbstractType abs, bool alt) const
        { JMP_THIS(0x47C4D0); }
#else
    ObjectClass* FindObjectOfType(AbstractType abs, bool alt) const;
#endif

    /// VA: 0x0047C520.
#if defined(RA2_YRPP_GAME)
    BuildingClass* GetBuilding() const
        { JMP_THIS(0x47C520); }
#else
    BuildingClass* GetBuilding() const;
#endif

    /// VA: 0x0047EBA0.
#if defined(RA2_YRPP_GAME)
    UnitClass* GetUnit(bool alt) const
        { JMP_THIS(0x47EBA0); }
#else
    UnitClass* GetUnit(bool alt) const;
#endif

    /// VA: 0x0047EC40.
#if defined(RA2_YRPP_GAME)
    InfantryClass* GetInfantry(bool alt) const
        { JMP_THIS(0x47EC40); }
#else
    InfantryClass* GetInfantry(bool alt) const;
#endif

    /// VA: 0x0047EBF0.
#if defined(RA2_YRPP_GAME)
    AircraftClass* GetAircraft(bool alt) const
        { JMP_THIS(0x47EBF0); }
#else
    AircraftClass* GetAircraft(bool alt) const;
#endif

    /// VA: 0x0047C550.
#if defined(RA2_YRPP_GAME)
    TerrainClass* GetTerrain(bool alt) const
        { JMP_THIS(0x47C550); }
#else
    TerrainClass* GetTerrain(bool alt) const;
#endif

    /* craziest thing... first iterates Content looking to Aircraft,
     * failing that, calls FindTechnoNearestTo,
     * if that fails too, reiterates Content looking for Terrain
     */
    /// VA: 0x0047C5A0.
#if defined(RA2_YRPP_GAME)
    ObjectClass* GetSomeObject(const Point2D& offset, bool alt) const
        { JMP_THIS(0x47C5A0); }
#else
    ObjectClass* GetSomeObject(const Point2D& offset, bool alt) const;
#endif

    // misc
    /// VA: 0x0047D210.
    void SetWallOwner()
        { JMP_THIS(0x47D210); }

    /// VA: 0x00487690.
#if defined(RA2_YRPP_GAME)
    void IncreaseShroudCounter() { JMP_THIS(0x487690); }
#else
    void IncreaseShroudCounter();
#endif

    /// VA: 0x00487630.
#if defined(RA2_YRPP_GAME)
    void ReduceShroudCounter() { JMP_THIS(0x487630); }
#else
    void ReduceShroudCounter();
#endif

    /// VA: 0x00487950.
    bool IsShrouded() const;

    /// VA: 0x004876F0.
#if defined(RA2_YRPP_GAME)
    void Unshroud() { JMP_THIS(0x4876F0); }
#else
    void Unshroud();
#endif

    /// VA: 0x0047FDE0.
    RectangleStruct* ShapeRect(RectangleStruct* pRet);

    /// VA: 0x004879B0.
    bool IsFogged();

    /// VA: 0x00486A70.
    void FogCell()
        { JMP_THIS(0x486A70); }

    /// VA: 0x00486BF0.
    void CleanFog()
        { JMP_THIS(0x486BF0); }

    /// VA: 0x00486C50.
    void ClearFoggedObjects()
        { JMP_THIS(0x486C50); }

    // adjusts LAT
    /// VA: 0x0047CA80.
    bool SetupLAT();

    // Recalculates cell attributes.
    // Checks for nearby cliff impassability, calls SetupLAT(), sets up TubeClass if tunnel, cell anim if attached etc.
    // Set cellLevel to -1 if you wish to not change it.
    /// VA: 0x0047D2B0.
    void RecalcAttributes(int cellLevel);
    /// VA: 0x00483C80
    void RecalcPassability();
    // Terrain geometry from 47D2B0. Requires a loaded tile catalog and a fresh
    // cell without overlays. LAT, tunnels, objects and animation instances are
    // later world passes; this does not replace full RecalcAttributes.
    bool RefreshTerrainGeometry() noexcept;
    // Initial 483E30/484180 field values when there are no light-source objects
    // or active superweapon effects. Palette allocation is backend-owned.
    bool InitializeTerrainLighting() noexcept;
    // VA: 0x004814F0. Pattern value before the caller reduces by variant count.
    int GetTileVariant(int tileTypeIndex, int variantCount) const noexcept;
    // 546DA0 resource selection; returns the base owner plus the reduced
    // variant index. Outputs are cleared on failure and borrowed from catalog.
    bool GetTerrainTile(IsometricTileTypeClass*& output, int& variant) const noexcept;
    // Terrain-only RGB branch of 47C060. TMP RadarLeft, theater brightness,
    // then half intensity; both halves have the same color in this branch.
    // Missing image/sub-tile returns the original (60,60,60) fallback.
    ColorStruct GetTerrainRadarColor() const noexcept;
    /// VA: 0x00480350
    void DrawIt(const Point2D& unraisedPosition,const RectangleStruct& clip,bool skip);
    /// VA: 0x004801F0
    void DrawShroudAndFog(const Point2D& point,const RectangleStruct& clip);
    /// VA: 0x004802A0
    void DrawShadowCast(const Point2D& point,const RectangleStruct& clip);
    // Original process-wide pattern cache; initialized lazily from Global RNG.
    static int (&TileVariantTable)[64]; // 0x89E620
    static bool& TileVariantTableInitialized; // 0x89E7C7

    /// VA: 0x0047DD70.
    void BlowUpBridge()
        { JMP_THIS(0x47DD70); }

    // Intact bridge placement during map loading; shares the state changes of
    // the concrete/wood routines. Does not implement destruction or repair.
    // direction is the original overlay direction (North or West).
    /// VA: 0x0047E040
    void InitializeBridge(FacingType direction) noexcept;

    /// VA: 0x00486FF0
#if defined(RA2_YRPP_GAME)
    bool CanBurrowHere() const { JMP_THIS(0x486FF0); }
#else
    bool CanBurrowHere() const;
#endif

    /// VA: 0x0047C620
#if defined(RA2_YRPP_GAME)
    bool CanThisExistHere(SpeedType SpeedType, BuildingTypeClass* pObject, HouseClass* pOwner) const
        { JMP_THIS(0x47C620); }
#else
    bool CanThisExistHere(SpeedType speed, BuildingTypeClass* type, HouseClass* owner) const;
#endif

    // those unks are passed to TechnoClass::Scatter in that same order
    /// VA: 0x00481670.
    void ScatterContent(const CoordStruct &crd, bool ignoreMission, bool ignoreDestination, bool alt)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x481670); }
#else
        ;
#endif

    /// VA: 0x00481810.
    CellClass* GetNeighbourCell(FacingType facing) const;

    // called whenever anything moves, first to remove threat from source cell, second time to add threat to dest cell
    /// VA: 0x00481870.
#if defined(RA2_YRPP_GAME)
    void UpdateThreat(unsigned int SourceHouse, int ThreatLevel)
        { JMP_THIS(0x481870); }
#else
    void UpdateThreat(unsigned int SourceHouse, int ThreatLevel);
#endif

    /// VA: 0x00481A00.
#if defined(RA2_YRPP_GAME)
    bool CollectCrate(FootClass* pCollector)
        { JMP_THIS(0x481A00); }
#else
    // Native non-crate branch restored; actual crate effects are not yet migrated.
    bool CollectCrate(FootClass* pCollector);
#endif

    /// VA: 0x00484180.
    void ProcessColourComponents(int* arg0, int* pIntensity, int* pAmbient, int* a5, int* a6, int* tintR, int* tintG, int* tintB)
        { JMP_THIS(0x484180); }

    /// VA: 0x00484F20.
#if defined(RA2_YRPP_GAME)
    TubeClass* GetTunnel()
        { JMP_THIS(0x484F20); }
#else
    TubeClass* GetTunnel();
#endif

    /// VA: 0x0047FB90.
    RectangleStruct* GetContainingRect(RectangleStruct* dest) const;

    // don't laugh, it returns the uiname of contained tiberium... which nobody ever sets
    /// VA: 0x00484FF0.
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const { JMP_THIS(0x484FF0); }
#else
    const wchar_t* GetUIName() const;
#endif

    // returns whether a cell behaves as if it contained overlay (for gates and wall towers)
    /// VA: 0x00480510
    bool ConnectsToOverlay(int idxOverlay = -1, int direction = -1) const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x480510); }
#else
        ;
#endif

    // returns the tiberium's index in OverlayTypes
    /// VA: 0x00485010.
#if defined(RA2_YRPP_GAME)
    int GetContainedTiberiumIndex() const
        { JMP_THIS(0x485010); }
#else
    int GetContainedTiberiumIndex() const;
#endif

    /// VA: 0x00485020.
#if defined(RA2_YRPP_GAME)
    int GetContainedTiberiumValue() const
        { JMP_THIS(0x485020); }
#else
    int GetContainedTiberiumValue() const;
#endif

    /// VA: 0x00483780.
#if defined(RA2_YRPP_GAME)
    bool SpreadTiberium(bool forced)
        { JMP_THIS(0x483780); }
#else
    bool SpreadTiberium(bool forced);
#endif

    // add or create tiberium of the specified type
    /// VA: 0x00487190.
#if defined(RA2_YRPP_GAME)
    bool IncreaseTiberium(int idxTiberium, int amount)
        { JMP_THIS(0x487190); }
#else
    bool IncreaseTiberium(int idxTiberium, int amount);
#endif

    // Returns credited resource units; clearing OverlayData == 0 returns zero.
    /// VA: 0x00480A80
#if defined(RA2_YRPP_GAME)
    int ReduceTiberium(int amount)
        { JMP_THIS(0x480A80); }
#else
    int ReduceTiberium(int amount);
#endif

    /// VA: 0x004838E0.
#if defined(RA2_YRPP_GAME)
    bool CanTiberiumGerminate(TiberiumClass* tib)
        { JMP_THIS(0x4838E0); }
#else
    bool CanTiberiumGerminate(TiberiumClass* tib);
#endif

    /// VA: 0x00483620
    bool CanTiberiumGrow() const;
    /// VA: 0x00483690
    bool CanTiberiumSpread() const;
    /// VA: 0x00483710
    bool GrowTiberium();

    // 0x485240 copies one CellStruct (4 bytes), not a lepton CoordStruct.
    void SetMapCoords(const CellStruct& coords);

    int GetFloorHeight(Point2D const& subcoords) const;

    // Factors in cell height from ramps, level etc.
    /// VA: 0x00480A30
    CoordStruct* GetCellCoords(CoordStruct* pOutBuffer) const;

    CoordStruct GetCellCoords() const
    {
        CoordStruct buffer;
        GetCellCoords(&buffer);
        return buffer;
    }

    /// VA: 0x00486920.
    void ActivateVeins();

    // cloak generators
    /// VA: 0x004870B0
    bool CloakGen_InclHouse(unsigned int idx) const
        { return ((1u << (idx & 31u)) & this->CloakedByHouses) != 0; }

    void CloakGen_AddHouse(unsigned int idx)
        { this->CloakedByHouses |= 1 << idx; }

    void CloakGen_RemHouse(unsigned int idx)
        { this->CloakedByHouses &= ~(1 << idx); }

    // unused, returns 0 if that house doesn't have cloakgens covering this cell or Player has sensors over this cell
    /// VA: 0x00486800.
    bool DrawObjectsCloaked(int OwnerHouseIdx) const
        { JMP_THIS(0x486800); }

    // sensors
    /// VA: 0x004870D0
    bool Sensors_InclHouse(unsigned int idx) const
        { return this->SensorsOfHouses[idx] > 0; }

    void Sensors_AddOfHouse(unsigned int idx)
        { ++this->SensorsOfHouses[idx]; }

    void Sensors_RemOfHouse(unsigned int idx)
        { --this->SensorsOfHouses[idx]; }

    // disguise sensors
    bool DisguiseSensors_InclHouse(unsigned int idx) const
        { return this->DisguiseSensorsOfHouses[idx] > 0; }

    void DisguiseSensors_AddOfHouse(unsigned int idx)
        { ++this->DisguiseSensorsOfHouses[idx]; }

    void DisguiseSensors_RemOfHouse(unsigned int idx)
        { --this->DisguiseSensorsOfHouses[idx]; }

    // Rad Sites
    void SetRadSite(RadSiteClass* pRad)
        { this->RadSite = pRad; }

    RadSiteClass* GetRadSite() const
        { return this->RadSite; }

    /// VA: 0x00487C10
#if defined(RA2_YRPP_GAME)
    bool CanBuildHere() const { JMP_THIS(0x487C10); }
#else
    bool CanBuildHere() const;
#endif

    /// VA: 0x00487C90.
    bool IsRadiated() const
        { JMP_THIS(0x487C90); }

    /// VA: 0x00487CB0.
#if defined(RA2_YRPP_GAME)
    int GetRadLevel() const
        { JMP_THIS(0x487CB0); }
#else
    int GetRadLevel() const;
#endif

    /// VA: 0x00487E00
#if defined(RA2_YRPP_GAME)
    bool IsCovered() const { JMP_THIS(0x487E00); }
#else
    bool IsCovered() const;
#endif

    /// VA: 0x00487CE0.
    void RadLevel_Increase(double amount)
        { JMP_THIS(0x487CE0); }

    /// VA: 0x00487D00.
    void RadLevel_Decrease(double amount)
        { JMP_THIS(0x487D00); }

    // helper
    bool ContainsBridge() const
    {
        return static_cast<bool>(this->Flags & CellFlags::BridgeHead);
    }
    bool ContainsBridgeEx() const
    {
        return static_cast<bool>(this->Flags & CellFlags::Bridge);
    }

    // helper mimicking game's behaviour
    ObjectClass* GetContent() const
        { return this->ContainsBridge() ? this->AltObject : this->FirstObject; }

    /// VA: 0x00487D50
    int GetLevel() const
        { return this->Level + (this->ContainsBridge() ? BridgeLevels : 0); }

    // tilesets
#define ISTILE(tileset, addr) \
    bool Tile_Is_ ## tileset() const \
        { JMP_THIS(addr); }

    /// VA: 0x00484AB0
    bool Tile_Is_Tunnel() const;
    /// VA: 0x00484AE0
    bool IsNearTunnelNW() const;
    /// VA: 0x00484D60
    bool IsNearTunnelES() const { JMP_THIS(0x484D60); }
    ISTILE(Water, 0x485060);
    ISTILE(Blank, 0x486380);
    ISTILE(Ramp, 0x4863A0);
    ISTILE(Cliff, 0x4863D0);
    ISTILE(Shore, 0x4865B0);
    ISTILE(Wet, 0x4865D0);
    ISTILE(MiscPave, 0x486650);
    ISTILE(Pave, 0x486670);
    ISTILE(DirtRoad, 0x486690);
    ISTILE(PavedRoad, 0x4866D0);
    ISTILE(PavedRoadEnd, 0x4866F0);
    ISTILE(PavedRoadSlope, 0x486710);
    ISTILE(Median, 0x486730);
    /// VA: 0x00486750
    bool Tile_Is_Bridge() const;
    /// VA: 0x00486770
    bool Tile_Is_WoodBridge() const;
    ISTILE(ClearToSandLAT, 0x486790);
    ISTILE(Green, 0x4867B0);
    ISTILE(NotWater, 0x4867E0);
    /// VA: 0x00486900
    bool Tile_Is_DestroyableCliff() const;

    static CoordStruct Cell2Coord(const CellStruct &cell, int z = 0)
    {
        CoordStruct ret;
        ret.X = cell.X * 256 + 128;
        ret.Y = cell.Y * 256 + 128;
        ret.Z = z;
        return ret;
    }

    static CellStruct Coord2Cell(const CoordStruct &crd)
    {
        CellStruct ret;
        ret.X = static_cast<short>(crd.X / 256);
        ret.Y = static_cast<short>(crd.Y / 256);
        return ret;
    }

    CoordStruct FixHeight(CoordStruct crd) const
    {
        if (this->ContainsBridge())
            crd.Z += BridgeHeight;

        return crd;
    }

    // helper - gets coords and fixes height for bridge
    CoordStruct GetCoordsWithBridge() const
    {
        CoordStruct buffer = this->GetCoords();
        return FixHeight(buffer);
    }

    /// VA: 0x00486E70.
    void MarkForRedraw()
        { JMP_THIS(0x486E70); }

    void ChainReaction()
    {
        CellStruct* cell = &this->MapCoords;
        SET_REG32(ecx, cell);
        CALL(0x489270);
    }

    /// VA: 0x00481180.
#if defined(RA2_YRPP_GAME)
    CoordStruct* FindInfantrySubposition(CoordStruct* pOutBuffer, const CoordStruct& coords, bool ignoreContents, bool alt, bool useCellCoords)
        { JMP_THIS(0x481180); }
#else
    CoordStruct* FindInfantrySubposition(CoordStruct* pOutBuffer, const CoordStruct& coords, bool ignoreContents, bool alt, bool useCellCoords);
#endif

    /// VA: 0x004810A0
    static unsigned char YRPP_FASTCALL InfantrySubpositionIndex(const CoordStruct& coords);

    CoordStruct FindInfantrySubposition(const CoordStruct& coords, bool ignoreContents, bool alt, bool useCellCoords)
    {
        CoordStruct outBuffer;
        this->FindInfantrySubposition(&outBuffer, coords, ignoreContents, alt, useCellCoords);
        return outBuffer;
    }

    /// VA: 0x00487D70.
    bool TryAssignJumpjet(FootClass* pObject)
        { JMP_THIS(0x487D70); }

    /// VA: 0x0047E8A0.
#if defined(RA2_YRPP_GAME)
    void  AddContent(ObjectClass* Content, bool onBridge)
        { JMP_THIS(0x47E8A0); }
#else
    void AddContent(ObjectClass* content, bool onBridge);
#endif

    /// VA: 0x0047EA90.
#if defined(RA2_YRPP_GAME)
    void  RemoveContent(ObjectClass* pContent, bool onBridge)
        { JMP_THIS(0x47EA90); }
#else
    void RemoveContent(ObjectClass* content, bool onBridge);
#endif

    /// VA: 0x00485250.
#if defined(RA2_YRPP_GAME)
    void ReplaceTag(TagClass* pTag)
        { JMP_THIS(0x485250) }
#else
    void ReplaceTag(TagClass* tag);
#endif

    void UpdateCellLighting(); // 484680; Scenario lighting and 16-bit cell intensities.

    /// VA: 0x00484180.
#if defined(RA2_YRPP_GAME)
    void CalculateLightSourceLighting(int& nIntensity, int& nAmbient, int& Red1, int& Green1, int& Blue1, int& Red2, int& Green2, int& Blue2)
        { JMP_THIS(0x484180); }
#else
    void CalculateLightSourceLighting(int& intensity, int& ambient, int& normal, int& terrain, int& bridge, int& red, int& green, int& blue);
#endif

    /// VA: 0x00483E30.
    void InitLightConvert(LightConvertClass* pDrawer = nullptr, int nIntensity = 0x10000,
        int nAmbient = 0, int Red1 = 1000, int Green1 = 1000, int Blue1 = 1000)
        { JMP_THIS(0x483E30); }

    /// VA: 0x00480110
    Point2D* GetOverlayDrawOffset(Point2D* output) const;

    /// VA: 0x0047F6A0
    void DrawOverlay(const Point2D& Location, const RectangleStruct& Bound);

    /// VA: 0x0047F510
    void DrawOverlayShadow(const Point2D& Location, const RectangleStruct& Bound);

    /// VA: 0x00483480.
    void RevealCellObjects();

    /// VA: 0x004834A0.
    bool IsClearToMove(SpeedType speedType, bool ignoreInfantry, bool ignoreVehicles, int zone, MovementZone movementZone, int level, bool isBridge);

    /// VA: 0x00480CB0
    void DamageWall(int damage)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x480CB0); }
#else
        ;
#endif

    /// VA: 0x00480630
    void UpdateWall(bool unchanged = false)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x480630); }
#else
        ;
#endif

protected:
    friend class MapClass;
    // Constructor
    /// VA: 0x0047BBF0.
    CellClass() noexcept;

    explicit __forceinline CellClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    CellStruct MapCoords;	//Where on the map does this Cell lie?
    DynamicVectorClass<FoggedObjectClass*>* FoggedObjects;
    CellClass*         BridgeOwnerCell;
    CellClass*         unknown_30; // original borrowed cell pointer, cleared by 0x00485130
    LightConvertClass* LightConvert;
    int                IsoTileTypeIndex;	//What tile is this Cell?
    TagClass*          AttachedTag;			// The cell tag
    BuildingTypeClass* Rubble;				// The building type that provides the rubble image
    int                OverlayTypeIndex;	//What Overlay lies on this Cell?
    int                SmudgeTypeIndex;	//What Smudge lies on this Cell?

    PassabilityType    Passability;
    int                WallOwnerIndex; // Which House owns the wall placed in this Cell?
    //                                 // Determined by finding the nearest BuildingType and taking its owner
    int                InfantryOwnerIndex;
    int                AltInfantryOwnerIndex;
    DWORD              unknown_5C;
    DWORD              unknown_60;
    DWORD              RedrawFrame;
    RectangleStruct    InViewportRect;
    DWORD              CloakedByHouses;	//Is this cell in a cloak generator's radius? One bit per House.

    // Is this cell in range of some SensorsSight= equipment? One Word(!) per House, ++ and -- per unit.
protected:
    unsigned short               SensorsOfHouses[0x18]; // ! 24 houses instead of 32 like cloakgen
    // use Sensors_ funcs above

    // Is this cell in range of some DetectDisguise= equipment? One Word(!) per House, ++ and -- per unit.
protected:
    unsigned short               DisguiseSensorsOfHouses[0x18]; // ! 24 houses instead of 32 like cloakgen
    // use DisguiseSensors_ funcs above

public:

    DWORD              BaseSpacerOfHouses; // & (1 << HouseX->ArrayIndex) == base spacing dummy for HouseX
    FootClass*         Jumpjet; // a jumpjet occupying this cell atm

    ObjectClass*       FirstObject;	//The first Object on this Cell. NextObject functions as a linked list.
    ObjectClass*       AltObject;

    LandType           LandType;	//What type of floor is this Cell?
    double             RadLevel;	//The level of radiation on this Cell.
    RadSiteClass*      RadSite;	//A pointer to the responsible RadSite.

    PixelFXClass*      PixelFX;
    int                OccupyHeightsCoveringMe;
    DWORD              Intensity;
    WORD               Ambient;
    WORD			   Intensity_Normal;
    WORD               Intensity_Terrain;
    WORD               Color1_Blue;
    // ColorStruct      Color2; //110-114
    WORD               Color2_Red;
    WORD               Color2_Green;
    WORD               Color2_Blue;
    signed short       TubeIndex; // !@#% Westwood braindamage, can't use > 127! (movsx eax, al)

    char               unknown_118;
    char               IsIceGrowthAllowed;
    char               Height;
    char               Level;

    BYTE               SlopeIndex;  // this + 2 == cell's slope shape as reflected by PLACE.SHP
    BYTE               unknown_11D;

    unsigned char      OverlayData;	//The crate type on this cell. Also indicates some other weird properties

    BYTE               SmudgeData;
    char               Visibility; // trust me, you don't wanna know... if you do, see 0x7F4194 and cry
    char               Foggedness; // same value as above: -2: Occluded completely, -1: Visible, 0...48: frame in fog.shp or shroud.shp
    BYTE               BlockedNeighbours; // OpenTS AdjacentObjectCount: placement reference count, NOT a blocked-direction mask
    PROTECTED_PROPERTY(BYTE, align_123);

    // SubOccupations - 0x1 Center 0x2 Top(Abandoned) 0x4 Right 0x8 Left 0x10 Down / Terrains
    // 0x20 Units 0x40 Aircrafts 0x80 Buildings
    DWORD              OccupationFlags;
    DWORD              AltOccupationFlags;

    AltCellFlags	   AltFlags;	// related to Flags below
    int                ShroudCounter;
    DWORD              GapsCoveringThisCell; // actual count of gapgens in this cell, no idea why they need a second layer
    bool               VisibilityChanged;
    PROTECTED_PROPERTY(BYTE,     align_139[0x3]);
    DWORD              unknown_13C;

    CellFlags          Flags;	//Various settings.
    PROTECTED_PROPERTY(BYTE,     padding_144[4]);
};
