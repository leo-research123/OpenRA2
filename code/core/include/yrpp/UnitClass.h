/*
    Ground & Naval Vehicles
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/FootClass.h"
#include "yrpp/UnitTypeClass.h"

// forward declarations
class EBolt;

class NOVTABLE UnitClass : public FootClass {
public:
    /// VA: 0x746100
#if defined(RA2_YRPP_GAME)
    bool INoticeSink_Unknown(DWORD code, DWORD parameter = 0) override {
        return reinterpret_cast<bool(__thiscall*)(INoticeSink*, DWORD, DWORD)>(0x746100)(
            static_cast<INoticeSink*>(this), code, parameter);
    }
#else
    bool INoticeSink_Unknown(DWORD code, DWORD parameter = 0) override;
#endif

    /// VA: 0x00746B20
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00746B20); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x0073B0B0
    bool DrawIfVisible(RectangleStruct* bounds, bool forced, DWORD extrasOnly) const override;
    // Reads the map's records and creates the objects. Malformed records are skipped;
    // allocation failures and unreadable data throw, and earlier placements remain.
    /// VA: 0x00743270
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI) { JMP_STD(0x743270); }
#else
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI);
#endif
    static const AbstractType AbsID = AbstractType::Unit;

    // Static
    /// Global VA: 0x008B4108.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<UnitClass*>, Array, 0x8B4108u)
#else
    static DynamicVectorClass<UnitClass*>& Array;
#endif

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x00735780
#if defined(RA2_YRPP_GAME)
    virtual ~UnitClass() RX;
#else
    ~UnitClass() override;
#endif

    // AbstractClass
    /// VA: 0x007446E0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x7446E0); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    /// VA: unknown (legacy placeholder).
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: unknown (legacy placeholder).
    int Size() const override { return sizeof(*this); }
    /// VA: 0x741490
    ObjectTypeClass* GetType() const override { return Type; }
    /// VA: 0x737430
    RadioCommand ReceiveCommand(TechnoClass* sender, RadioCommand command, AbstractClass*& data) override;

    // ObjectClass

    /// VA: 0x00743190
    AbstractClass* GreatestThreat(ThreatType threat, CoordStruct* origin, bool onlyTargetHouseEnemy) override;
    /// VA: 0x00746C90
    bool IsUnderEMP() const override { return DeathFrameCounter != -1 || TechnoClass::IsUnderEMP(); }
    /// VA: 0x00746750
    bool IsDisguisedAs(HouseClass* house) const override;
    /// VA: 0x007465B0
    ObjectTypeClass* GetDisguise(bool againstAllies) const override;
    /// VA: 0x00746670
#if defined(RA2_YRPP_GAME)
    DWORD DisguiseAs(AbstractClass* target) override { JMP_THIS(0x746670); }
#else
    DWORD DisguiseAs(AbstractClass* target) override;
#endif
    /// VA: 0x007465F0
    HouseClass* GetDisguiseHouse(bool againstAllies) const override;
    /// VA: 0x00746720
    void ClearDisguise() override;
    /// VA: 0x00741340
    BulletClass* Fire(AbstractClass* target, int weapon) override;

    /// VA: 0x744720
#if defined(RA2_YRPP_GAME)
    void RegisterDestruction(TechnoClass* source) override { JMP_THIS(0x744720); }
#else
    void RegisterDestruction(TechnoClass* source) override;
#endif

    /// VA: 0x737C90
#if defined(RA2_YRPP_GAME)
    DamageState ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead, ObjectClass* source,
                              bool ignoreDefenses, bool preventEscape, HouseClass* sourceHouse) override {
        JMP_THIS(0x737C90);
    }
#else
    DamageState ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead, ObjectClass* source,
                              bool ignoreDefenses, bool preventEscape, HouseClass* sourceHouse) override;
#endif

    /// VA: 0x00746400.
    bool IsStrange() const override { return !Type->NonVehicle; }
    /// VA: 0x00738890.
#if defined(RA2_YRPP_GAME)
    bool ObjectClickedAction(Action action, ObjectClass* target, bool ignoreForce) override { JMP_THIS(0x738890); }
#else
    bool ObjectClickedAction(Action action, ObjectClass* target, bool ignoreForce) override;
#endif

    /// VA: 0x738910
#if defined(RA2_YRPP_GAME)
    bool CellClickedAction(Action action, CellStruct* cell, CellStruct* follow, bool ignoreForce) override {
        JMP_THIS(0x738910);
    }
#else
    bool CellClickedAction(Action action, CellStruct* cell, CellStruct* follow, bool ignoreForce) override;
#endif

    /// VA: 0x007463A0
#if defined(RA2_YRPP_GAME)
    bool SetOwningHouse(HouseClass* house, bool announce = true) override { JMP_THIS(0x7463A0); }
#else
    bool SetOwningHouse(HouseClass* house, bool announce = true) override;
#endif

    /// VA: 0x007404B0.

#if defined(RA2_YRPP_GAME)
    virtual Action MouseOverCell(CellStruct const* pCell, bool checkFog = false,
                                 bool ignoreForce = false) const override {
        JMP_THIS(0x7404B0)
    };
#else
    Action MouseOverCell(CellStruct const* pCell, bool checkFog = false, bool ignoreForce = false) const override;
#endif
    /// VA: 0x0073FD50.

#if defined(RA2_YRPP_GAME)
    virtual Action MouseOverObject(ObjectClass const* pObject, bool ignoreForce = false) const override {
        JMP_THIS(0x73FD50)
    };
#else
    Action MouseOverObject(ObjectClass const* pObject, bool ignoreForce = false) const override;
#endif

    /// VA: 0x007441B0.

#if defined(RA2_YRPP_GAME)
    virtual void MarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x7441B0) };
#else
    void MarkAllOccupationBits(const CoordStruct& coords) override;
#endif
    /// VA: 0x00744210.

#if defined(RA2_YRPP_GAME)
    virtual void UnmarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x744210) };
#else
    void UnmarkAllOccupationBits(const CoordStruct& coords) override;
#endif
    // ...and so on
    // FIXME other virtual function explicit addresses

    using TechnoClass::TurretFacing;
    /// VA: 0x00746E30
#if defined(RA2_YRPP_GAME)
    DirStruct* TurretFacing(DirStruct* output) const override { JMP_THIS(0x746E30); }
#else
    DirStruct* TurretFacing(DirStruct* output) const override;
#endif

    /// VA: 0x00740FD0.

#if defined(RA2_YRPP_GAME)
    virtual FireError GetFireError(AbstractClass* pTarget, int nWeaponIndex, bool checkRange) const override
        JMP_THIS(0x740FD0);
#else
    FireError GetFireError(AbstractClass* pTarget, int nWeaponIndex, bool checkRange) const override;
#endif

    bool InitializeLocomotor() noexcept;
    /// VA: 0x744180
#if defined(RA2_YRPP_GAME)
    bool vt_entry_29C() override { return reinterpret_cast<bool(YRPP_THISCALL*)(UnitClass*)>(0x744180)(this); }
#else
    bool vt_entry_29C() override;
#endif
    /// VA: 0x7416A0
#if defined(RA2_YRPP_GAME)
    void TryCrushCell(const CellStruct& cell, bool warn) override {
        reinterpret_cast<void(YRPP_THISCALL*)(UnitClass*, const CellStruct&, bool)>(0x7416A0)(this, cell, warn);
    }
#else
    void TryCrushCell(const CellStruct& cell, bool warn) override;
#endif
    /// VA: 0x736D50
#if defined(RA2_YRPP_GAME)
    bool CanDeployNow() const override { JMP_THIS(0x736D50); }
#else
    bool CanDeployNow() const override;
#endif
    /// VA: 0x741970
    void SetDestination(AbstractClass* destination, bool immediate) override;
    /// VA: 0x7414E0
#if defined(RA2_YRPP_GAME)
    AbstractClass* ApproachTarget(DWORD queryOnly) override {
        return reinterpret_cast<AbstractClass*(YRPP_THISCALL*)(UnitClass*, DWORD)>(0x7414E0)(this, queryOnly);
    }
#else
    AbstractClass* ApproachTarget(DWORD queryOnly) override;
#endif
    /// VA: 0x743A50
    void Scatter(const CoordStruct& from, bool force, bool urgent) override;
    /// VA: 0x7360C0
    void Update() override;
    /// VA: 0x739EC0
    void UpdatePosition(PCPType reason) override;
    /// VA: 0x744270
    bool ReadyToNextMission() const override;
    /// VA: 0x41C070
    bool IsStandingStill() const override { return FrozenStill; }
    /// VA: 0x738970
    bool EnterIdleMode(bool initial, bool resume) override;
    /// VA: 0x73F0A0
    Move IsCellOccupied(CellClass* cell, FacingType facing, int level, CellClass* source = nullptr,
                        bool checkLocomotor = true) const override;

    /// VA: 0x740A90
#if defined(RA2_YRPP_GAME)
    int Mission_Move() override { JMP_THIS(0x740A90); }
#else
    int Mission_Move() override;
#endif
    /// VA: 0x73D630
#if defined(RA2_YRPP_GAME)
    int Mission_Unload() override { JMP_THIS(0x73D630); }
#else
    int Mission_Unload() override;
#endif
    /// VA: 0x740810
#if defined(RA2_YRPP_GAME)
    int Mission_Guard() override { JMP_THIS(0x740810); }
#else
    int Mission_Guard() override;
#endif
    /// VA: 0x744100
#if defined(RA2_YRPP_GAME)
    int Mission_AreaGuard() override { JMP_THIS(0x744100); }
#else
    int Mission_AreaGuard() override;
#endif
    /// VA: 0x73EFC0
#if defined(RA2_YRPP_GAME)
    int Mission_Hunt() override { JMP_THIS(0x73EFC0); }
#else
    int Mission_Hunt() override;
#endif
    /// VA: 0x73E5E0
    int Mission_Harvest() override;
    int MissionHarvestUnload();
    /// VA: 0x740B60
#if defined(RA2_YRPP_GAME)
    FacingType DesiredLoadDir(FootClass* passenger, CellStruct& cell) const override { JMP_THIS(0x740B60); }
#else
    FacingType DesiredLoadDir(FootClass* passenger, CellStruct& cell) const override;
#endif

    // UnitClass
    /// VA: 0x00746420.
#if defined(RA2_YRPP_GAME)
    void ReceiveGunner(FootClass* gunner) override { JMP_THIS(0x746420); }
#else
    void ReceiveGunner(FootClass* gunner) override;
#endif
    /// VA: 0x007464E0.
#if defined(RA2_YRPP_GAME)
    void RemoveGunner(FootClass* gunner) override { JMP_THIS(0x7464E0); }
#else
    void RemoveGunner(FootClass* gunner) override;
#endif

    // main drawing functions - Draw() calles one of these, they call parent's Draw_A_smth
    /// VA: 0x0073CEC0
    void DrawIt(Point2D* point, RectangleStruct* bounds) const override;
    /// VA: 0x0073B470
    virtual void DrawAsVXL(Point2D Coords, RectangleStruct BoundingRect, int Brightness, int Tint);

    /// VA: 0x0073C5F0.
    virtual void DrawAsSHP(Point2D Coords, RectangleStruct BoundingRect, int Brightness, int Tint);

    /// VA: 0x0073B140
#if defined(RA2_YRPP_GAME)
    virtual void DrawObject(Surface* pSurface, Point2D Coords, RectangleStruct CacheRect, int Brightness, int Tint) {
        JMP_THIS(0x73B140);
    }
#else
    virtual void DrawObject(Surface* pSurface, Point2D Coords, RectangleStruct CacheRect, int Brightness, int Tint);
#endif

    // non-virtual

    /// VA: 0x00746DB0
    bool IsDeployingOrUndeploying() const { return Deploying || Undeploying; }

    /// VA: 0x0070FBD0
    bool IsDeactivated() const
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x70FBD0);
    }
#else
    {
        return Deactivated;
    }
#endif

    /// VA: 0x007359F0.
    void UpdateTube() JMP_THIS(0x7359F0);
    /// VA: 0x736990
#if defined(RA2_YRPP_GAME)
    void UpdateRotation() JMP_THIS(0x736990);
#else
    void UpdateRotation();
#endif
    /// VA: 0x00736C10.
    void UpdateEdgeOfWorld() JMP_THIS(0x736C10);  // inlined in game
    /// VA: 0x00736DF0.
#if defined(RA2_YRPP_GAME)
    void UpdateFiring() JMP_THIS(0x736DF0);
#else
    void UpdateFiring();
#endif
    /// VA: 0x00737180.
    void UpdateVisceroid() JMP_THIS(0x737180);
    /// VA: 0x007468C0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void UpdateDisguise() { reinterpret_cast<void(YRPP_THISCALL*)(UnitClass*)>(0x7468C0)(this); }
#else
    void UpdateDisguise();
#endif

    /// VA: 0x00746D60
#if defined(RA2_YRPP_GAME)
    void Destroy() override { JMP_THIS(0x00746D60); }
#else
    void Destroy() override;
#endif

    /// VA: 0x738680
#if defined(RA2_YRPP_GAME)
    void Explode() JMP_THIS(0x738680);
#else
    void Explode();
#endif

    /// VA: 0x00738D30.
    /// VA: 0x738D30
#if defined(RA2_YRPP_GAME)
    bool GotoClearSpot() { JMP_THIS(0x738D30); }
#else
    bool GotoClearSpot();
#endif
    /// VA: 0x007393C0.
#if defined(RA2_YRPP_GAME)
    bool TryToDeploy() JMP_THIS(0x7393C0);
#else
    bool TryToDeploy();
#endif
    /// VA: 0x739AC0
#if defined(RA2_YRPP_GAME)
    void Deploy() JMP_THIS(0x739AC0);
#else
    void Deploy();
#endif
    /// VA: 0x739CD0
#if defined(RA2_YRPP_GAME)
    void Undeploy() JMP_THIS(0x739CD0);
#else
    void Undeploy();
#endif
    /// VA: 0x746CD0
#if defined(RA2_YRPP_GAME)
    int SelectWeapon(AbstractClass* target) const override { JMP_THIS(0x746CD0); }
#else
    int SelectWeapon(AbstractClass* target) const override;
#endif

    /// VA: 0x0073D450.
    bool Harvesting();

    /// VA: 0x00740DF0.
    /// VA: 0x740DF0
#if defined(RA2_YRPP_GAME)
    bool FlagAttach(int nHouseIdx) { JMP_THIS(0x740DF0); }
#else
    bool FlagAttach(int nHouseIdx);
#endif
    /// VA: 0x00740E20.
    /// VA: 0x740E20
#if defined(RA2_YRPP_GAME)
    bool FlagRemove() { JMP_THIS(0x740E20); }
#else
    bool FlagRemove();
#endif

    /// VA: 0x00740E60.
    void APCCloseDoor() JMP_THIS(0x740E60);  // inlined in game
    /// VA: 0x00740E80.
    void APCOpenDoor() JMP_THIS(0x740E80);  // inlined in game

    /// VA: 0x007436E0.
    static void YRPP_FASTCALL WriteINI(CCINIClass* pINI) JMP_STD(0x7436E0);

    /// VA: 0x007438F0.
    bool ShouldCrashIt(TechnoClass* pTarget) JMP_THIS(0x7438F0);

    /// VA: 0x007447B0.
    AbstractClass* AssignDestination_7447B0(AbstractClass* pTarget) JMP_THIS(0x7447B0);
    /// VA: 0x00746000.
    bool AStarAttempt(const CellStruct& cell1, const CellStruct& cell2) JMP_THIS(0x746000);

    // Constructor
    /// VA: 0x007353C0.
#if defined(RA2_YRPP_GAME)
    UnitClass(UnitTypeClass* pType, HouseClass* pOwner) noexcept : UnitClass(noinit_t()) { JMP_THIS(0x7353C0); }
#else
    UnitClass(UnitTypeClass* pType, HouseClass* pOwner) noexcept;
#endif

protected:
    explicit __forceinline UnitClass(noinit_t) noexcept : FootClass(noinit_t()) {}

    // Properties

public:
    int CurrentFiringFrame;
    UnitTypeClass* Type;
    UnitClass* FollowerCar;  // groovy - link defined in the map's [Units] section, looked up on startup
    int FlagHouseIndex;      // Carrying the flag of this House
    bool IsFollowerCar;      // This vehicle is another vehicle's FollowerCar (such as a train car following train).
    bool Unloading;
    bool IsHarvesting;
    bool TerrainPalette;
    int unknown_int_6D4;
    int DeathFrameCounter;
    EBolt* ElectricBolt;  //Unit is the owner of this
    bool Deployed;
    bool Deploying;
    bool Undeploying;
    int NonPassengerCount;  // Set when unloading passengers. Units with TurretCount>0 will not unload the gunner.

    wchar_t ToolTipText[0x100];
};
