/*
    Base class for units that can move (that have "feet")
*/

#pragma once

#include "yrpp/TechnoClass.h"
#include "yrpp/ParasiteClass.h"

// forward declarations
class LocomotionClass;
class PathFinderData;
class TeamClass;
class WaypointClass;

class NOVTABLE FootClass : public TechnoClass
{
public:
    /// VA: 0x004DAFC0
#if defined(RA2_YRPP_GAME)
    int GetZAdjustment() const override { JMP_THIS(0x4DAFC0); }
#else
    int GetZAdjustment() const override;
#endif
    /// VA: 0x004DB0A0
    ZGradient GetZGradient() const override;
    static const auto AbsDerivateID = AbstractFlags::Foot;

    /// Global VA: 0x008B3DC0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<FootClass*>, Array, 0x8B3DC0u)
#else
    static DynamicVectorClass<FootClass*>& Array;
#endif

    /// VA: 0x004DC060
#if defined(RA2_YRPP_GAME)
    void DrawActionLines(bool force, DWORD dashed) override { JMP_THIS(0x4DC060); }
#else
    void DrawActionLines(bool force, DWORD dashed) override;
#endif

    // IPersistStream
    // Destructor
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~FootClass() RX;
#else
    virtual ~FootClass();
#endif

    // AbstractClass
    /// VA: 0x4D8F40
    void Override_Mission(Mission mission,AbstractClass* target,AbstractClass* destination) override;
    /// VA: 0x4D8F80
    bool Mission_Revert() override;
    using TechnoClass::GetDestination;
    /// VA: 0x4D8FB0
    RadioCommand ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data) override;
    /// VA: 0x004DBDF0
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetDestination(CoordStruct* output, TechnoClass* docker = nullptr) const override { JMP_THIS(0x4DBDF0); }
#else
    CoordStruct* GetDestination(CoordStruct* output, TechnoClass* docker = nullptr) const override;
#endif
    /// VA: 0x004DA530
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x4DA530); }
#else
    void Update() override;
#endif
    /// VA: 0x004D5660
    void Stun() override;
    /// VA: 0x004D9960
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x4D9960); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    // ObjectClass
    /// VA: 0x004DDED0
    Action MouseOverObject(ObjectClass const* object,bool ignoreForce=false) const override;
    /// VA: 0x004DB260
    bool Limbo() override;
    /// VA: 0x004D7330
    DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) override;
    /// VA: 0x004DDDE0
#if defined(RA2_YRPP_GAME)
    Action MouseOverCell(const CellStruct* cell,bool checkFog=false,bool ignoreForce=false) const override { JMP_THIS(0x4DDDE0); }
#else
    Action MouseOverCell(const CellStruct* cell,bool checkFog=false,bool ignoreForce=false) const override;
#endif
    /// VA: 0x004D7D50
#if defined(RA2_YRPP_GAME)
    bool CellClickedAction(Action action,CellStruct* cell,CellStruct* follow,bool ignoreForce) override { JMP_THIS(0x4D7D50); }
#else
    bool CellClickedAction(Action action,CellStruct* cell,CellStruct* follow,bool ignoreForce) override;
#endif
    /// VA: 0x004D74E0
#if defined(RA2_YRPP_GAME)
    bool ObjectClickedAction(Action action,ObjectClass* target,bool ignoreForce) override { JMP_THIS(0x4D74E0); }
#else
    bool ObjectClickedAction(Action action,ObjectClass* target,bool ignoreForce) override;
#endif
    /// VA: 0x004DE1D0
#if defined(RA2_YRPP_GAME)
    CellStruct* MoveOrder(CellStruct* output,const CellStruct* where,bool checkFog) { JMP_THIS(0x4DE1D0); }
#else
    CellStruct* MoveOrder(CellStruct* output,const CellStruct* where,bool checkFog);
#endif
    /// VA: 0x004D9720
#if defined(RA2_YRPP_GAME)
    void Disappear(bool permanently) override { JMP_THIS(0x4D9720); }
#else
    void Disappear(bool permanently) override;
#endif
    /// VA: 0x004DBED0
    bool SetOwningHouse(HouseClass* house,bool announce=true) override;
    /// VA: 0x004DE630
    void AddPassenger(FootClass* passenger) override;
    /// VA: 0x004D4B20
    int Mission_Capture() override;
    /// VA: 0x4D9290
    int Mission_Enter() override;
    /// VA: 0x004D85D0
#if defined(RA2_YRPP_GAME)
    void UpdatePosition(PCPType reason) override { JMP_THIS(0x4D85D0); }
#else
    void UpdatePosition(PCPType reason) override;
#endif
    /// VA: 0x004DE5D0
#if defined(RA2_YRPP_GAME)
    void UnInit() override { JMP_THIS(0x4DE5D0); }
#else
    void UnInit() override;
#endif
    /// VA: 0x004DB7E0
    Layer InWhichLayer() const override;
    /// VA: 0x004D7170
    bool Unlimbo(const CoordStruct& where,DirType facing) override;
    /// VA: 0x004D3780
    bool Mark(MarkType value) override;
    /// VA: 0x004DB810
    void SetLocation(const CoordStruct& coord) override;
    /// VA: 0x004DDC40
    bool IsOnBridge(TechnoClass* docker = nullptr) const override;
    /// VA: 0x004DE620
    bool IsInAir() const override { return IsOnMap && GetHeight() >= 2 * 104; }
    /// VA: 0x0041C150
    CellStruct GetLastFlightMapCoords() const override { return LastFlightMapCoords; }
    /// VA: 0x0041C160
    void SetLastFlightMapCoords(CellStruct cell) override { LastFlightMapCoords=cell; }
    /// VA: 0x004D9C60
#if defined(RA2_YRPP_GAME)
    Move CanReachCell(const CellClass* destination, FacingType facing,
        int& level, bool& bridge, const CellClass* source) const override { JMP_THIS(0x4D9C60); }
#else
    Move CanReachCell(const CellClass* destination, FacingType facing,
        int& level, bool& bridge, const CellClass* source) const override;
#endif
    // MissionClass

    /// VA: 0x004D4DC0
#if defined(RA2_YRPP_GAME)
    int Mission_Attack() override { JMP_THIS(0x4D4DC0); }
#else
    int Mission_Attack() override;
#endif

    /// VA: 0x004D5070
#if defined(RA2_YRPP_GAME)
    int Mission_Guard() override { JMP_THIS(0x4D5070); }
#else
    int Mission_Guard() override;
#endif

    /// VA: 0x004D4200
#if defined(RA2_YRPP_GAME)
    int Mission_Move() override { JMP_THIS(0x4D4200); }
#else
    int Mission_Move() override;
#endif

    /// VA: 0x004D6AA0.
    virtual int Mission_AreaGuard() override;

    // TechnoClass
    /// VA: 0x004DC810
    void AssignPlanningPath(signed int idxPath,signed char idxWP) override;

    /// VA: 0x0041C050
    bool BelongsToATeam() const override { return Team!=nullptr; }
    /// VA: 0x004DE770
#if defined(RA2_YRPP_GAME)
    bool IsParalyzed() const override { JMP_THIS(0x4DE770); }
#else
    bool IsParalyzed() const override;
#endif
    /// VA: 0x004DBDA0
#if defined(RA2_YRPP_GAME)
    bool IsCloakable() const override { JMP_THIS(0x4DBDA0); }
#else
    bool IsCloakable() const override;
#endif
    /// VA: 0x004DA4E0
#if defined(RA2_YRPP_GAME)
    VisualType VisualCharacter(VARIANT_BOOL raw,HouseClass* asking) const override { JMP_THIS(0x4DA4E0); }
#else
    VisualType VisualCharacter(VARIANT_BOOL raw,HouseClass* asking) const override;
#endif
    /// VA: 0x004D82B0
#if defined(RA2_YRPP_GAME)
    bool EnterIdleMode(bool initial, bool resume) override { JMP_THIS(0x4D82B0); }
#else
    bool EnterIdleMode(bool initial, bool resume) override;
#endif
    /// VA: 0x004DF1A0
    void ClearMegaMissionData() override {
        MegaMission=Mission::None; MegaDestination=nullptr; MegaTarget=nullptr; HaveAttackMoveTarget=false;
    }
    /// VA: 0x004DF0E0
#if defined(RA2_YRPP_GAME)
    Mission RespondMegaEventMission(EventClass* event) override { JMP_THIS(0x4DF0E0); }
#else
    Mission RespondMegaEventMission(EventClass* event) override;
#endif
    /// VA: 0x004DF1C0
    bool HaveMegaMission() const override { return MegaMission!=Mission::None; }
    /// VA: 0x004DF310
    bool MegaMissionIsAttackMove() const override { return MegaMission==Mission::AttackMove; }
    /// VA: 0x004DF320
#if defined(RA2_YRPP_GAME)
    bool ContinueMegaMission() override { JMP_THIS(0x4DF320); }
#else
    bool ContinueMegaMission() override;
#endif
    /// VA: 0x004DF3A0
#if defined(RA2_YRPP_GAME)
    void UpdateAttackMove() override { JMP_THIS(0x4DF3A0); }
#else
    void UpdateAttackMove() override;
#endif
    /// VA: 0x004DF4B0
#if defined(RA2_YRPP_GAME)
    bool RefreshMegaMission() override { JMP_THIS(0x4DF4B0); }
#else
    bool RefreshMegaMission() override;
#endif
    /// VA: 0x004DA030
#if defined(RA2_YRPP_GAME)
    void HandleNavigationList() { JMP_THIS(0x4DA030); }
#else
    void HandleNavigationList();
#endif
    /// VA: 0x004D94B0
    void SetDestination(AbstractClass* destination, bool immediate) override;
    /// VA: 0x004D94A0
    void vt_entry_47C(AbstractClass* follow) override { unknown_5A0=follow; }
    /// VA: 0x004DBA50
#if defined(RA2_YRPP_GAME)
    bool IsInSameZoneAs(AbstractClass* target) override { JMP_THIS(0x4DBA50); }
#else
    bool IsInSameZoneAs(AbstractClass* target) override;
#endif
    /// VA: 0x004D3810
    bool IsInSameZoneAsCoords(const CoordStruct& coord) override;
    /// VA: 0x004DA1D0
#if defined(RA2_YRPP_GAME)
    bool vt_entry_320() const override { JMP_THIS(0x4DA1D0); }
#else
    bool vt_entry_320() const override; // Original Is_Allowed_To_Leave_Map.
#endif
    /// VA: 0x4D98C0
    virtual void Destroyed(ObjectClass *Killer) RX;
    /// VA: unknown (legacy placeholder).
    virtual bool ForceCreate(CoordStruct& coord, DWORD dwUnk = 0) R0;
    /// VA: 0x004D9920.
    virtual AbstractClass* GreatestThreat(ThreatType threat, CoordStruct* pCoord, bool onlyTargetHouseEnemy) override;

    /// VA: 0x004DEBB0
#if defined(RA2_YRPP_GAME)
    bool Crash(ObjectClass* killer) override { JMP_THIS(0x4DEBB0); }
#else
    bool Crash(ObjectClass* killer) override;
#endif

    // FootClass
    /// VA: 0x004DE750.
    virtual void ReceiveGunner(FootClass* Gunner) RX;
    /// VA: 0x004DE760.
    virtual void RemoveGunner(FootClass* Gunner) RX;
    /// VA: 0x004DC790
#if defined(RA2_YRPP_GAME)
    virtual bool IsLeavingMap() const { JMP_THIS(0x4DC790); }
#else
    virtual bool IsLeavingMap() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool vt_entry_4E0() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool CanDeployNow() const R0;
    /// VA: 0x004DE7B0
#if defined(RA2_YRPP_GAME)
    virtual void AddSensorsAt(CellStruct cell) { JMP_THIS(0x4DE7B0); }
#else
    virtual void AddSensorsAt(CellStruct cell);
#endif
    /// VA: 0x004DE940
#if defined(RA2_YRPP_GAME)
    virtual void RemoveSensorsAt(CellStruct cell) { JMP_THIS(0x4DE940); }
#else
    virtual void RemoveSensorsAt(CellStruct cell);
#endif
    /// VA: 0x004D9FF0
#if defined(RA2_YRPP_GAME)
    virtual CoordStruct* vt_entry_4F0(CoordStruct* pCrd) { JMP_THIS(0x4D9FF0); }
#else
    virtual CoordStruct* vt_entry_4F0(CoordStruct* pCrd);
#endif
    /// VA: 0x004DC8C0
    bool FollowWaypoint(WaypointClass* waypoint) { JMP_THIS(0x4DC8C0); }
    /// Global VA: 0x00A83DBC.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(int, MapTriggerID, 0xA83DBCu)
#else
    static int& MapTriggerID;
#endif
    /// VA: 0x004DC030
    virtual void vt_entry_4F4();
    /// VA: unknown (legacy placeholder).
    virtual bool vt_entry_4F8() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool MoveTo(CoordStruct* pCrd) R0;
    /// VA: 0x004D55C0
    virtual bool StopMoving();
    /// VA: unknown (legacy placeholder).
    virtual bool TryEnterIdle() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool ChronoWarpTo(CoordStruct pDest) R0; // fsds... only implemented for one new YR map trigger, other chrono events repeat the code...
    /// VA: 0x0041C090
    virtual void Draw_A_SHP(
        SHPStruct *SHP, int idxFacing, Point2D * Coords, RectangleStruct *Rectangle,
        DWORD dwUnk5, DWORD dwUnk6, DWORD dwUnk7, ZGradient ZGradient,
        DWORD dwUnk9, int extraLight, DWORD dwUnk11, SHPStruct* depthImage,
        DWORD dwUnk13, DWORD dwUnk14, DWORD dwUnk15, DWORD dwUnk16);

    /// VA: 0x004DAF10
    virtual void Draw_A_VXL(
        VoxelStruct *VXL, int HVAFrameIndex, int Flags, IndexClass<VoxelIndexKey, VoxelCacheStruct*> *Cache, RectangleStruct *Rectangle,
        Point2D *CenterPoint, Matrix3D *Matrix, int Brightness, BlitterFlags DrawFlags, DWORD dwUnk10);

    /// VA: 0x004DB0D0
    void DrawVoxelShadow(VoxelStruct* voxel, int layer, VoxelIndexKey key,
        IndexClass<ShadowVoxelIndexKey, VoxelCacheStruct*>* cache, RectangleStruct* clip,
        Point2D* point, Matrix3D* matrix, bool force, Surface* surface, Point2D shadowPoint);

    /// VA: unknown (legacy placeholder).
    virtual void GoBerzerk() RX;
    /// VA: unknown (legacy placeholder).
    virtual void Panic() RX;
    /// VA: unknown (legacy placeholder).
    virtual void UnPanic() RX; //never
    /// VA: 0x0041C120
    virtual void PlayIdleAnim(int nIdleAnimNumber) RX;
    /// VA: unknown (legacy placeholder).
    virtual DWORD vt_entry_524() R0;
    /// VA: 0x4DF040
    virtual BuildingClass* TryNearestDockBuilding(TypeList<BuildingTypeClass*>* bList, DWORD dwUnk2, DWORD dwUnk3) const;
    /// VA: 0x4DEE80
    virtual BuildingClass* FindCloserDockBuilding(BuildingTypeClass* bType, DWORD dwUnk2, DWORD dwUnk3, int* pDistance) const;
    /// VA: 0x4DEE50
    virtual BuildingClass* FindNearestDockBuilding(BuildingTypeClass* bType, DWORD dwUnk2, DWORD dwUnk3) const;
    /// VA: unknown (legacy placeholder).
    virtual void TryCrushCell(const CellStruct& cell, bool warn) RX;
    /// VA: 0x004DB1A0
    virtual int GetCurrentSpeed() const;
    /// VA: 0x004D5690
#if defined(RA2_YRPP_GAME)
    virtual AbstractClass* ApproachTarget(DWORD queryOnly) { JMP_THIS(0x4D5690); }
#else
    virtual AbstractClass* ApproachTarget(DWORD queryOnly);
#endif
    /// VA: 0x0041C140
    virtual void vt_entry_540(PathFinderData*) {} // Original empty Fixup_Path.
    /// VA: 0x004D3710
    virtual void SetSpeedPercentage(double percentage);
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_548() RX;
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_54C() RX;
    /// VA: unknown (legacy placeholder).
    virtual bool IsLandZoneClear(AbstractClass* pDestination) R0;

    /// VA: 0x004DA230.
    bool CanBeRecruited(HouseClass *ByWhom) const
        { JMP_THIS(0x4DA230); }

    // non-virtual

    // only used by squid damage routines, normal wakes are created differently it seems
    // creates 3 wake animations behind the unit
    /// VA: 0x00629E90.
    void CreateWakes(CoordStruct coords)
        { JMP_THIS(0x629E90); }

    // can this jumpjet stay in this cell or not? (two jumpjets in one cell are not okay, locomotor kicks one of them out in the next frame)
    /// VA: 0x004135A0.
    bool Jumpjet_LocationClear() const
        { JMP_THIS(0x4135A0); }

    /// VA: 0x004E00B0.
    void Jumpjet_OccupyCell(CellStruct Cell)
        { JMP_THIS(0x4E00B0); }

    // changes locomotor to the given one, Magnetron style
    // mind that this locks up the source too, Magnetron style
    /// VA: 0x00710000.
    void FootClass_ImbueLocomotor(FootClass *target, CLSID clsid)
        { JMP_THIS(0x710000); }

    // var $this = this; $.each($this.Passengers, function(ix, p) { p.Location = $this.Location; });
    /// VA: 0x007104F0.
    void UpdatePassengerCoords()
        { JMP_THIS(0x7104F0); }

    /// VA: 0x004DF0D0.
    void AbortMotion()
        { unknown_5A0 = nullptr; Destination = nullptr; }

    /// VA: 0x004D3920.
    // OpenTS FootClass::Basic_Path; owns the 24-direction cache and failure transition.
    bool UpdatePathfinding(CellStruct destinationCell, int pathOffset, int mode);

    /// VA: 0x004DC760
    double ThreatAvoidanceValue() const;

    /// VA: 0x004CBBA0.
    PathFinderData* FindPath(CellStruct* pReachableDestCell, int* pBuffer, int unusedInt, int unusedInt_1, int nPathDirectionsIdx, int nMode);

    // Removes the first passenger and updates the Gunner.
    /// VA: 0x004DE710.
#if defined(RA2_YRPP_GAME)
    FootClass* RemoveFirstPassenger()
        { JMP_THIS(0x4DE710); }
#else
    FootClass* RemoveFirstPassenger();
#endif

    // Removes a specific passenger and updates the Gunner.
    /// VA: 0x004DE670.
    FootClass* RemovePassenger(FootClass* pPassenger)
        { JMP_THIS(0x4DE670); }

    // Adds a specific passenger and updates the Gunner.
    /// VA: 0x004DE630.
    void EnterAsPassenger(FootClass* pPassenger)
        { JMP_THIS(0x4DE630); }

    // Adds to the NavQueue
    /// VA: 0x004DA0E0.
    void QueueNavigationList(AbstractClass * target)
    { JMP_THIS(0x4DA0E0); }

    // Clears NavQueue
    /// VA: 0x004DA1C0.
    void ClearNavigationList()
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x4DA1C0); }
#else
        { NavQueue.Clear(); }
#endif

    // searches cell, sets destination, and returns whether unit is on that cell
    /// VA: 0x004DCFE0.
    bool MoveToTiberium(int radius, bool scanClose = false);
    /// VA: 0x4DD0A0
    CellStruct* ScanForTiberium(CellStruct* output, int range, DWORD scanClose) const override;

    // searches cell, sets destination, and returns whether unit is on that cell
    /// VA: 0x004DDB90.
    bool MoveToWeed(int radius)
        { JMP_THIS(0x4DDB90); }

    // Constructor
    /// VA: 0x004D31E0
#if defined(RA2_YRPP_GAME)
    FootClass(HouseClass* pOwner) noexcept : FootClass(noinit_t())
        { JMP_THIS(0x4D31E0); }
#else
    FootClass(HouseClass* pOwner) noexcept;
#endif

protected:
    explicit __forceinline FootClass(noinit_t) noexcept
        : TechnoClass(noinit_t())
    { }

    // Properties

public:

    int             PlanningPathIdx; // which planning path am I following?
    CellStruct      WaypointNearbyAccessibleCellDelta; // add to WaypointCell to get Nearby_Cell for this foot
    CellStruct      WaypointCell; // current waypoint cell
    DWORD           unknown_52C;	//unused?
    double          ThreatAvoidanceCoefficient;
    int				WalkedFramesSoFar;
    bool            IsMoveSoundPlaying;
    int             MoveSoundDelay;

    DECLARE_PROPERTY(AudioController, MoveSoundAudioController);

    CellStruct      CurrentMapCoords;
    CellStruct      LastMapCoords; // ::UpdatePosition uses this to remove threat from last occupied cell, etc
    CellStruct      LastFlightMapCoords; // which cell was I occupying previously? only for AircraftTracker-tracked stuff
    CellStruct      CurrentJumpjetMapCoords; // unconfirmed, which cell am I occupying? only for jumpjets
    CoordStruct     CurrentTunnelCoords;
    PROTECTED_PROPERTY(DWORD,   unused_574);
    double          SpeedPercentage;
    double          SpeedMultiplier;
    DECLARE_PROPERTY(DynamicVectorClass<AbstractClass*>, unknown_abstract_array_588);
    AbstractClass*  unknown_5A0;
    AbstractClass*  Destination; // possibly other objects as well
    AbstractClass*  LastDestination;
    DECLARE_PROPERTY(DynamicVectorClass<AbstractClass*>, NavQueue); // Stores sequence of movement destinations
    Mission         MegaMission; // only Mission::AttackMove or Mission::None
    AbstractClass*  MegaDestination; // when AttackMove target is a cell
    AbstractClass*  MegaTarget; // when AttackMove target is an object
    BYTE            unknown_5D0;	//unused?
    bool            HaveAttackMoveTarget; // fighting an enemy on the way
    TeamClass*      Team;
    FootClass*      NextTeamMember;        //next unit in team
    DWORD           unknown_5DC;
    int             PathDirections[24]; // list of directions to move in next, like tube directions
    DECLARE_PROPERTY(CDTimerClass, PathDelayTimer);
    int             PathWaitTimes;
    DECLARE_PROPERTY(CDTimerClass, unknown_timer_650);
    DECLARE_PROPERTY(CDTimerClass, SightTimer);
    DECLARE_PROPERTY(CDTimerClass, BlockagePathTimer);
#if defined(_MSC_VER)
    DECLARE_PROPERTY(ILocomotionPtr, Locomotor);
#else
    // Native layout can be inspected without MSVC's _com_ptr_t. Movement and
    // COM ownership are not implemented by the map display build.
    DECLARE_PROPERTY(ILocomotion*, Locomotor);
#endif
    CoordStruct       unknown_point3d_678;
    signed char       TubeIndex;	//I'm in this tunnel
    signed char       TubeFaceIndex;
    signed char       WaypointIndex; // which waypoint in my planning path am I following?
    bool              ShouldScatterInNextIdle;
    bool              IsScanLimited;
    bool              IsInitiated; // Is a fully joined member of a team, used for regroup etc. checks
    bool              ShouldScanForTarget;
    bool              unknown_bool_68B; //unused?
    bool              IsDeploying;
    bool              IsFiring;
    bool              unknown_bool_68E;
    bool              ShouldEnterAbsorber; // orders the unit to enter the closest bio reactor
    bool              ShouldEnterOccupiable; // orders the unit to enter the closest battle bunker
    bool              ShouldGarrisonStructure; // orders the unit to enter the closest neutral building
    FootClass*        ParasiteEatingMe; // the tdrone/squid that's eating me
    int               LastBeParasitedStartFrame;
    ParasiteClass*    ParasiteImUsing;	// my parasitic half, nonzero for, eg, terror drone or squiddy
    DECLARE_PROPERTY(CDTimerClass, ParalysisTimer); // for squid victims
    bool              unknown_bool_6AC;
    bool              IsAttackedByLocomotor; // the unit's locomotor is jammed by a magnetron
    bool              IsLetGoByLocomotor; // a magnetron attacked this unit and let it go. falling, landing, or sitting on the ground
    bool              unknown_bool_6AF;
    bool              unknown_bool_6B0;
    bool              unknown_bool_6B1;
    bool              unknown_bool_6B2;
    bool              unknown_bool_6B3;
    signed char       DrawingYOffset; // YR drawing reads the signed byte at 0x6B4.
    bool              IsCrushingSomething;
    bool              FrozenStill; // frozen in first frame of the proper facing - when magnetron'd or warping
    bool              IsWaitingBlockagePath;
    bool              unknown_bool_6B8;
    PROTECTED_PROPERTY(DWORD,   unused_6BC);	//???
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(FootClass) == 0x6C0);
#endif
