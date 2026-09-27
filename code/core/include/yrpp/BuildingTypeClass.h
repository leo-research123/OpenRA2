#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TechnoTypeClass.h"
class OverlayTypeClass;
class IsometricTileTypeClass;

struct BuildingAnimStruct
{
    char Anim[0x10];
    char Damaged[0x10];
    char Garrisoned[0x10];
    Point2D Position;
    int ZAdjust;
    int YSort;
    bool Powered;
    bool PoweredLight;
    bool PoweredEffect;
    bool PoweredSpecial;
};

struct BuildingAnimFrameStruct
{
    DWORD dwUnknown;
    int FrameCount;
    int FrameDuration;
};

class BuildingTypeClass : public TechnoTypeClass
{
public:
    // Original entry reference; the local override is not declared here yet.
    // LoadFromINI: verified at the original main vtable's +0x64 slot.
    /// VA: 0x0045FE50.

    /// VA: 0x00465010; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x00465010); }
    /// VA: 0x00465300; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x00465300); }
    /// VA: 0x00464B30; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00464B30); }
    /// VA: 0x00465DB0; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;

    static const AbstractType AbsID = AbstractType::BuildingType;

    // Array
    static DynamicVectorClass<BuildingTypeClass*>& Array;
    static BuildingTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x004653C0.
#if defined(RA2_YRPP_GAME)
    static BuildingTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id) { JMP_STD(0x4653C0); }
#else
    static BuildingTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);
#endif

    // IPersist
    /// VA: 0x00465380.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~BuildingTypeClass();

    // AbstractClass
    /// VA: 0x00465D90.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00465DA0.
    virtual int Size() const;

    // AbstractTypeClass
    /// VA: 0x0045FE50; native map-display subset, full original reader retained.
#if defined(RA2_YRPP_GAME)
    bool LoadFromINI(CCINIClass* ini) override { JMP_THIS(0x0045FE50); }
#else
    bool LoadFromINI(CCINIClass* ini) override;
#endif
    // ObjectTypeClass
    /// VA: 0x00464AF0
#if defined(RA2_YRPP_GAME)
    CoordStruct* Dimension2(CoordStruct* output) override { JMP_THIS(0x464AF0); }
#else
    CoordStruct* Dimension2(CoordStruct* output) override;
#endif
    /// VA: 0x0045EC20
#if defined(RA2_YRPP_GAME)
    CellStruct* GetFoundationData(bool includeBib=false) const override { JMP_THIS(0x45EC20); }
#else
    CellStruct* GetFoundationData(bool includeBib=false) const override;
#endif
    /// VA: 0x00464A70
#if defined(RA2_YRPP_GAME)
    CoordStruct* vt_entry_6C(CoordStruct* dest, CoordStruct* source) const override
        { JMP_THIS(0x00464A70); }
#else
    CoordStruct* vt_entry_6C(CoordStruct* dest, CoordStruct* source) const override;
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner) R0;
    /// VA: unknown (legacy placeholder).
    virtual ObjectClass* CreateObject(HouseClass* pOwner) R0;

    // TechnoTypeClass
    // BuildingTypeClass
    /// VA: 0x00465960
    virtual SHPStruct* LoadBuildup() R0;
    /// VA: 0x0045ED50
    int GetCost() const override;
    /// VA: 0x0045EDD0
    int GetActualCost(HouseClass* house) const override;

    // non-virtual
    /// VA: 0x0045F160
    bool GetRubbleShape(SHPStruct** image, int* frame) const;
    /// VA: 0x0045F1D0
    bool GetRubbleShadowShape(SHPStruct** image, int* frame) const;
    /// VA: 0x465570
#if defined(RA2_YRPP_GAME)
    RectangleStruct* GetDrawRect(RectangleStruct* output) JMP_THIS(0x465570);
#else
    RectangleStruct* GetDrawRect(RectangleStruct* output);
#endif

    /// VA: 0x00465AF0.
    void ClearBuildUp()
        { JMP_THIS(0x465AF0); }

    /// VA: 0x00465D40.
#if defined(RA2_YRPP_GAME)
    bool IsVehicle() const
        { JMP_THIS(0x465D40); }
#else
    bool IsVehicle() const;
#endif

    /// VA: 0x0045EC90.
#if defined(RA2_YRPP_GAME)
    short GetFoundationWidth() const
        { JMP_THIS(0x45EC90); }
#else
    short GetFoundationWidth() const;
#endif
    /// VA: 0x0045ECA0.
#if defined(RA2_YRPP_GAME)
    short GetFoundationHeight(bool bIncludeBib) const
        { JMP_THIS(0x45ECA0); }
#else
    short GetFoundationHeight(bool bIncludeBib) const;
#endif

    // Existing TechnoType virtual slot 0x00A8, not a new virtual slot.
    /// VA: 0x00464AC0
#if defined(RA2_YRPP_GAME)
    bool CanCreateHere(const CellStruct& cell, HouseClass* owner) const override
        { JMP_THIS(0x464AC0); }
#else
    bool CanCreateHere(const CellStruct& cell, HouseClass* owner) const override;
#endif

    /// VA: 0x00464AC0
#if defined(RA2_YRPP_GAME)
    bool CanPlaceHere(CellStruct* cell, HouseClass* owner) const
        { JMP_THIS(0x464AC0); }
#else
    bool CanPlaceHere(CellStruct* cell, HouseClass* owner) const;
#endif

    // helpers
    bool HasSuperWeapon(int index) const {
        return (this->SuperWeapon == index || this->SuperWeapon2 == index);
    }

    bool HasSuperWeapon() const {
        return (this->SuperWeapon != -1 || this->SuperWeapon2 != -1);
    }

    bool CanTogglePower() const {
        return this->TogglePower && (this->PowerDrain > 0 || this->Powered);
    }

    BuildingAnimStruct& GetBuildingAnim(BuildingAnimSlot slot) {
        return this->BuildingAnim[static_cast<int>(slot)];
    }

    const BuildingAnimStruct& GetBuildingAnim(BuildingAnimSlot slot) const {
        return this->BuildingAnim[static_cast<int>(slot)];
    }

    // Constructor
    struct ConstructionDefaults {
        /// Global VA: 0x0089C8D0; runtime-initialized, not an on-disk default.
        CoordStruct coordinate;
        /// Global VA: 0x0089C8A0; runtime-initialized, not an on-disk default.
        int unknown_1538[4];
    };
    /// VA: 0x0045DD90.
    BuildingTypeClass(const char* pID) noexcept : BuildingTypeClass(noinit_t()) { JMP_THIS(0x0045DD90); }
    BuildingTypeClass(const char* pID, const ConstructionDefaults& defaults);
    static BuildingTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id, const ConstructionDefaults& defaults);

protected:
    explicit __forceinline BuildingTypeClass(noinit_t) noexcept
        : TechnoTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    CellStruct* FoundationData;
    SHPStruct* Buildup;
    bool BuildupLoaded;
    BuildCat BuildCat;
    CoordStruct HalfDamageSmokeLocation1;
    CoordStruct HalfDamageSmokeLocation2;
    DWORD align_E24;
    double GateCloseDelay;
    int LightVisibility;
    int LightIntensity;
    int LightRedTint;
    int LightGreenTint;
    int LightBlueTint;
    Point2D PrimaryFirePixelOffset;
    Point2D SecondaryFirePixelOffset;
    OverlayTypeClass* ToOverlay;
    IsometricTileTypeClass* ToTile;
    char BuildupFile [0x10];
    int BuildupSound;
    int PackupSound;
    int CreateUnitSound;
    int UnitEnterSound;
    int UnitExitSound;
    int WorkingSound;
    int NotWorkingSound;
    char PowersUpBuilding [0x18];
    UnitTypeClass* FreeUnit;
    InfantryTypeClass* SecretInfantry;
    UnitTypeClass* SecretUnit;
    BuildingTypeClass* SecretBuilding;
    int field_EB0;
    int Adjacent;
    AbstractType Factory;
    CoordStruct TargetCoordOffset;
    CoordStruct ExitCoord;
    CellStruct* FoundationOutside;
    int StartFacing;
    int DeployFacing;
    int PowerBonus;
    int PowerDrain;
    int ExtraPowerBonus;
    int ExtraPowerDrain;
    Foundation Foundation;
    int Height;
    int OccupyHeight;
    int MidPoint;
    int DoorStages;

    BuildingAnimFrameStruct BuildingAnimFrame[6];

    BuildingAnimStruct BuildingAnim[0x15];

    int Upgrades;
    SHPStruct* DeployingAnim;
    bool DeployingAnimLoaded;
    SHPStruct* UnderDoorAnim;
    bool UnderDoorAnimLoaded;
    SHPStruct* Rubble;
    bool RubbleLoaded;
    SHPStruct* RoofDeployingAnim;
    bool RoofDeployingAnimLoaded;
    SHPStruct* UnderRoofDoorAnim;
    bool UnderRoofDoorAnimLoaded;
    SHPStruct* DoorAnim;
    SHPStruct* SpecialZOverlay;
    int SpecialZOverlayZAdjust;
    SHPStruct* BibShape;
    bool BibShapeLoaded;
    int NormalZAdjust;
    int AntiAirValue;
    int AntiArmorValue;
    int AntiInfantryValue;
    Point2D ZShapePointMove;
    int unknown_1538;
    int unknown_153C;
    int unknown_1540;
    int unknown_1544;
    WORD ExtraLight;
    bool TogglePower;
    bool HasSpotlight;
    bool IsTemple;
    bool IsPlug;
    bool HoverPad;
    bool BaseNormal;
    bool EligibileForAllyBuilding;
    bool EligibleForDelayKill;
    bool NeedsEngineer;
    int CaptureEvaEvent;
    int ProduceCashStartup;
    int ProduceCashAmount;
    int ProduceCashDelay;
    int InfantryGainSelfHeal;
    int UnitsGainSelfHeal;
    int RefinerySmokeFrames;
    bool Bib;
    bool Wall;
    bool Capturable;
    bool Powered;
    bool PoweredSpecial;
    bool Overpowerable;
    bool Spyable;
    bool CanC4;
    bool WantsExtraSpace;
    bool Unsellable;
    bool ClickRepairable;
    bool CanBeOccupied;
    bool CanOccupyFire;
    int MaxNumberOccupants;
    bool ShowOccupantPips;

    Point2D MuzzleFlash[0xA];

    Point2D DamageFireOffset[8];

    Point2D QueueingCell;
    int NumberImpassableRows;

    Point2D RemoveOccupy[8];

    Point2D AddOccupy[8];

    bool Radar;
    bool SpySat;
    bool ChargeAnim;
    bool IsAnimDelayedFire;
    bool SiloDamage;
    bool UnitRepair;
    bool UnitReload;
    bool Bunker;
    bool Cloning;
    bool Grinding;
    bool UnitAbsorb;
    bool InfantryAbsorb;
    bool SecretLab;
    bool DoubleThick;
    bool Flat;
    bool DockUnload;
    bool Recoilless;
    bool HasStupidGuardMode;
    bool BridgeRepairHut;
    bool Gate;
    bool SAM;
    bool ConstructionYard;
    bool NukeSilo;
    bool Refinery;
    bool Weeder;
    bool WeaponsFactory;
    bool LaserFencePost;
    bool LaserFence;
    bool FirestormWall;
    bool Hospital;
    bool Armory;
    bool EMPulseCannon;
    bool TickTank;
    bool TurretAnimIsVoxel;
    bool BarrelAnimIsVoxel;
    bool CloakGenerator;
    bool SensorArray;
    bool ICBMLauncher;
    bool Artillary;
    bool Helipad;
    bool OrePurifier;
    bool FactoryPlant;
    float InfantryCostBonus;
    float UnitsCostBonus;
    float AircraftCostBonus;
    float BuildingsCostBonus;
    float DefensesCostBonus;
    bool GDIBarracks;
    bool NODBarracks;
    bool YuriBarracks;
    float ChargedAnimTime;
    int DelayedFireDelay;
    int SuperWeapon;
    int SuperWeapon2;
    int GateStages;
    int PowersUpToLevel;
    bool DamagedDoor;
    bool InvisibleInGame;
    bool TerrainPalette;
    bool PlaceAnywhere;
    bool ExtraDamageStage;
    bool AIBuildThis;
    bool IsBaseDefense;
    BYTE CloakRadiusInCells;
    bool ConcentricRadialIndicator;
    int PsychicDetectionRadius;
    int BarrelStartPitch;
    char VoxelBarrelFile [0x14];

    double VoxelBarrelScale;

    CoordStruct VoxelBarrelOffsetToPitchPivotPoint;
    CoordStruct VoxelBarrelOffsetToRotatePivotPoint;
    CoordStruct VoxelBarrelOffsetToBuildingPivotPoint;
    CoordStruct VoxelBarrelOffsetToBarrelEnd;
    bool DemandLoad;
    bool DemandLoadBuildup;
    bool FreeBuildup;
    bool IsThreatRatingNode;
    bool PrimaryFireDualOffset;
    bool ProtectWithWall;
    bool CanHideThings;
    bool CrateBeneath;
    bool LeaveRubble;
    bool CrateBeneathIsMoney;
    char TheaterSpecificID [0x13];
    int NumberOfDocks;
    VectorClass<CoordStruct> DockingOffsets;
private: DWORD align_1794;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(offsetof(BuildingTypeClass, VoxelBarrelScale) == 0x1728, "BuildingTypeClass::VoxelBarrelScale moved");
static_assert(offsetof(BuildingTypeClass, VoxelBarrelFile) == 0x1714, "BuildingTypeClass::VoxelBarrelFile moved");
#endif
