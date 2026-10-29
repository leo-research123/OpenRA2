/*
    Aircraft
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/FootClass.h"
#include "yrpp/AircraftTypeClass.h"

// AircraftClass
class NOVTABLE AircraftClass : public FootClass, public IFlyControl {
public:
    /// VA: 0x0041C1D0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x0041C1D0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    static const AbstractType AbsID = AbstractType::Aircraft;
    /// VA: 0x41B900
#if defined(RA2_YRPP_GAME)
    void DecreaseAmmo() override { JMP_THIS(0x41B900); }
#else
    // Aircraft attack missions consume one round after the firing sequence.
    void DecreaseAmmo() override {}
#endif
    /// VA: 0x00417F80
#if defined(RA2_YRPP_GAME)
    Action MouseOverCell(const CellStruct* cell, bool checkFog = false, bool ignoreForce = false) const override {
        JMP_THIS(0x417F80);
    }
#else
    Action MouseOverCell(const CellStruct* cell, bool checkFog = false, bool ignoreForce = false) const override;
#endif
    /// VA: 0x00417CC0
#if defined(RA2_YRPP_GAME)
    Action MouseOverObject(const ObjectClass* object, bool ignoreForce = false) const override { JMP_THIS(0x417CC0); }
#else
    Action MouseOverObject(const ObjectClass* object, bool ignoreForce = false) const override;
#endif
    /// VA: 0x004144B0
    void DrawIt(Point2D* point, RectangleStruct* bounds) const override;

    /// VA: 0x0041ADF0
#if defined(RA2_YRPP_GAME)
    void See(DWORD incremental, DWORD dontMap) override { JMP_THIS(0x41ADF0); }
#else
    void See(DWORD incremental, DWORD dontMap) override;
#endif

    /// VA: 0x41B920
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    bool IsInAir() const override {
        using Entry = bool(YRPP_THISCALL*)(const AircraftClass*);
        return reinterpret_cast<Entry>(0x41B920)(this);
    }
#else
    bool IsInAir() const override { JMP_THIS(0x41B920); }
#endif
#else
    bool IsInAir() const override;
#endif
    /// VA: 0x41B980
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    bool IsOnFloor() const override {
        using Entry = bool(YRPP_THISCALL*)(const AircraftClass*);
        return reinterpret_cast<Entry>(0x41B980)(this);
    }
#else
    bool IsOnFloor() const override { JMP_THIS(0x41B980); }
#endif
#else
    bool IsOnFloor() const override;
#endif
    /// VA: 0x41B910
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    bool IsStrange() const override {
        using Entry = bool(YRPP_THISCALL*)(const AircraftClass*);
        return reinterpret_cast<Entry>(0x41B910)(this);
    }
#else
    bool IsStrange() const override { JMP_THIS(0x41B910); }
#endif
#else
    bool IsStrange() const override;
#endif
    /// VA: 0x41BA90
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    bool QueueMission(Mission mission, bool start) override {
        using Entry = bool(YRPP_THISCALL*)(AircraftClass*, Mission, bool);
        return reinterpret_cast<Entry>(0x41BA90)(this, mission, start);
    }
#else
    bool QueueMission(Mission mission, bool start) override { JMP_THIS(0x41BA90); }
#endif
#else
    bool QueueMission(Mission mission, bool start) override;
#endif
    /// VA: 0x4190B0
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    RadioCommand ReceiveCommand(TechnoClass* sender, RadioCommand command, AbstractClass*& data) override {
        using Entry = RadioCommand(YRPP_THISCALL*)(AircraftClass*, TechnoClass*, RadioCommand, AbstractClass*&);
        return reinterpret_cast<Entry>(0x4190B0)(this, sender, command, data);
    }
#else
    RadioCommand ReceiveCommand(TechnoClass* sender, RadioCommand command, AbstractClass*& data) override { JMP_THIS(0x4190B0); }
#endif
#else
    RadioCommand ReceiveCommand(TechnoClass* sender, RadioCommand command, AbstractClass*& data) override;
#endif
    /// VA: 0x419C80
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    int Mission_Enter() override {
        using Entry = int(YRPP_THISCALL*)(AircraftClass*);
        return reinterpret_cast<Entry>(0x419C80)(this);
    }
#else
    int Mission_Enter() override { JMP_THIS(0x419C80); }
#endif
#else
    int Mission_Enter() override;
#endif
    /// VA: 0x41A5C0
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    int Mission_Guard() override {
        using Entry = int(YRPP_THISCALL*)(AircraftClass*);
        return reinterpret_cast<Entry>(0x41A5C0)(this);
    }
#else
    int Mission_Guard() override { JMP_THIS(0x41A5C0); }
#endif
#else
    int Mission_Guard() override;
#endif
    /// VA: 0x41A940
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    int Mission_AreaGuard() override {
        using Entry = int(YRPP_THISCALL*)(AircraftClass*);
        return reinterpret_cast<Entry>(0x41A940)(this);
    }
#else
    int Mission_AreaGuard() override { JMP_THIS(0x41A940); }
#endif
#else
    int Mission_AreaGuard() override;
#endif
    /// VA: 0x41BBD0
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    BuildingClass* TryNearestDockBuilding(TypeList<BuildingTypeClass*>* types, DWORD arg, DWORD busy) const override {
        using Entry = BuildingClass*(YRPP_THISCALL*)(const AircraftClass*, TypeList<BuildingTypeClass*>*, DWORD, DWORD);
        return reinterpret_cast<Entry>(0x41BBD0)(this, types, arg, busy);
    }
#else
    BuildingClass* TryNearestDockBuilding(TypeList<BuildingTypeClass*>* types, DWORD arg, DWORD busy) const override { JMP_THIS(0x41BBD0); }
#endif
#else
    BuildingClass* TryNearestDockBuilding(TypeList<BuildingTypeClass*>* types, DWORD arg, DWORD busy) const override;
#endif
    /// VA: 0x41A160
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    AbstractClass* FindLandingLocation() {
        using Entry = AbstractClass*(YRPP_THISCALL*)(AircraftClass*);
        return reinterpret_cast<Entry>(0x41A160)(this);
    }
#else
    AbstractClass* FindLandingLocation() { JMP_THIS(0x41A160); }
#endif
#else
    AbstractClass* FindLandingLocation();
#endif

    // Static
    /// Global VA: 0x00A8E390.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<AircraftClass*>, Array, 0xA8E390u)
#else
    static DynamicVectorClass<AircraftClass*>& Array;
#endif

    // IFlyControl
    /// VA: 0x0041B6A0
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Landing_Altitude() override {
        using Entry = int(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B6A0)(static_cast<IFlyControl*>(this));
    }
#else
    int YRPP_STDCALL Landing_Altitude() override;
#endif
    /// VA: 0x0041B760
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Landing_Direction() override {
        using Entry = int(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B760)(static_cast<IFlyControl*>(this));
    }
#else
    int YRPP_STDCALL Landing_Direction() override;
#endif
    /// VA: 0x0041B7D0
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Loaded() override {
        using Entry = long(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B7D0)(static_cast<IFlyControl*>(this));
    }
#else
    long YRPP_STDCALL Is_Loaded() override;
#endif
    /// VA: 0x0041B7F0
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Strafe() override {
        using Entry = long(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B7F0)(static_cast<IFlyControl*>(this));
    }
#else
    long YRPP_STDCALL Is_Strafe() override;
#endif
    /// VA: 0x0041B840
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Fighter() override {
        using Entry = long(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B840)(static_cast<IFlyControl*>(this));
    }
#else
    long YRPP_STDCALL Is_Fighter() override;
#endif
    /// VA: 0x0041B860
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Locked() override {
        using Entry = long(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B860)(static_cast<IFlyControl*>(this));
    }
#else
    long YRPP_STDCALL Is_Locked() override;
#endif

    // IUnknown
    /// VA: 0x00414290
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) override { JMP_STD(0x00414290); }
#else
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) override;
#endif
    /// VA: 0x004142F0
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL AddRef() override { JMP_STD(0x004142F0); }
#else
    ULONG YRPP_STDCALL AddRef() override;
#endif
    /// VA: 0x00414300
#if defined(RA2_YRPP_GAME)
    ULONG YRPP_STDCALL Release() override { JMP_STD(0x00414300); }
#else
    ULONG YRPP_STDCALL Release() override;
#endif

    // IPersist
    /// VA: 0x0041C190
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override { JMP_STD(0x0041C190); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;
#endif

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // AbstractClass
    /// VA: 0x0041C180
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x0041C170
    int Size() const override { return sizeof(*this); }

    // Destructor
    /// VA: 0x00414080
    ~AircraftClass() override;

    // Constructor
    /// VA: 0x00413D20.
#if defined(RA2_YRPP_GAME)
    AircraftClass(AircraftTypeClass* pType, HouseClass* pOwner) noexcept : AircraftClass(noinit_t()) {
        JMP_THIS(0x413D20);
    }
#else
    AircraftClass(AircraftTypeClass* pType, HouseClass* pOwner) noexcept;
#endif

    // Creates the [Aircraft] section's objects of a scenario INI.
    /// VA: 0x0041B110
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ReadINI(CCINIClass* ini) { JMP_STD(0x41B110); }
#else
    static void YRPP_FASTCALL ReadINI(CCINIClass* ini);
#endif

    /// VA: 0x0041B890
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    bool IsLeavingMap() const override {
        using Entry = bool(YRPP_THISCALL*)(const AircraftClass*);
        return reinterpret_cast<Entry>(0x41B890)(this);
    }
#else
    bool IsLeavingMap() const override { JMP_THIS(0x41B890); }
#endif
#else
    bool IsLeavingMap() const override;
#endif
    /// VA: 0x0041B660
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x41B660); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    /// VA: 0x004165C0
#if defined(RA2_YRPP_GAME)
    DamageState ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead, ObjectClass* source,
                              bool ignoreDefenses, bool preventEscape, HouseClass* sourceHouse) override {
        JMP_THIS(0x4165C0);
    }
#else
    DamageState ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead, ObjectClass* source,
                              bool ignoreDefenses, bool preventEscape, HouseClass* sourceHouse) override;
#endif
    bool InitializeLocomotor() noexcept;
    /// VA: 0x0041C1D0
#if defined(RA2_YRPP_GAME)
    ObjectTypeClass* GetType() const override { JMP_THIS(0x41C1D0); }
#else
    ObjectTypeClass* GetType() const override;
#endif
    /// VA: 0x00414BB0
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x414BB0); }
#else
    void Update() override;
#endif
    /// VA: 0x00414310
#if defined(RA2_YRPP_GAME)
    bool Unlimbo(const CoordStruct& at, DirType facing) override { JMP_THIS(0x414310); }
#else
    bool Unlimbo(const CoordStruct& at, DirType facing) override;
#endif
    /// VA: 0x0041B5E0
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    bool ReadyToNextMission() const override {
        using Entry = bool(YRPP_THISCALL*)(const AircraftClass*);
        return reinterpret_cast<Entry>(0x41B5E0)(this);
    }
#else
    bool ReadyToNextMission() const override { JMP_THIS(0x41B5E0); }
#endif
#else
    bool ReadyToNextMission() const override;
#endif
    /// VA: 0x414A80
#if defined(RA2_YRPP_GAME)
    int Mission_Hunt() override { JMP_THIS(0x414A80); }
#else
    int Mission_Hunt() override;
#endif
    /// VA: 0x417FE0
#if defined(RA2_YRPP_GAME)
    int Mission_Attack() override { JMP_THIS(0x417FE0); }
#else
    int Mission_Attack() override;
#endif
    /// VA: 0x004166C0
#if defined(RA2_YRPP_GAME)
    int Mission_Move() override { JMP_THIS(0x4166C0); }
#else
    int Mission_Move() override;
#endif
    /// VA: 0x0041AA80
#if defined(RA2_YRPP_GAME)
    void SetDestination(AbstractClass* target, bool immediate) override { JMP_THIS(0x41AA80); }
#else
    void SetDestination(AbstractClass* target, bool immediate) override;
#endif
    /// VA: 0x004176F0
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    // clang-cl may spill AL between the legacy macro's separate asm blocks.
    // A typed call preserves the original thiscall contract on that compiler.
    bool EnterIdleMode(bool initial, bool resume) override {
        using Entry = bool(YRPP_THISCALL*)(AircraftClass*, bool, bool);
        return reinterpret_cast<Entry>(0x4176F0)(this, initial, resume);
    }
#else
    bool EnterIdleMode(bool initial, bool resume) override { JMP_THIS(0x4176F0); }
#endif
#else
    bool EnterIdleMode(bool initial, bool resume) override;
#endif

    /// VA: 0x004158E0
#if defined(RA2_YRPP_GAME)
    int Mission_ParaDropApproach() override { JMP_THIS(0x004158E0); }
#else
    int Mission_ParaDropApproach() override;
#endif
    /// VA: 0x00415960
#if defined(RA2_YRPP_GAME)
    int Mission_ParaDropOverfly() override { JMP_THIS(0x00415960); }
#else
    int Mission_ParaDropOverfly() override;
#endif
    /// VA: 0x4155F0
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    int Mission_SpyPlaneApproach() override {
        using Entry = int(YRPP_THISCALL*)(AircraftClass*);
        return reinterpret_cast<Entry>(0x4155F0)(this);
    }
#else
    int Mission_SpyPlaneApproach() override { JMP_THIS(0x4155F0); }
#endif
#else
    int Mission_SpyPlaneApproach() override;
#endif
    /// VA: 0x4157C0
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    int Mission_SpyPlaneOverfly() override {
        using Entry = int(YRPP_THISCALL*)(AircraftClass*);
        return reinterpret_cast<Entry>(0x4157C0)(this);
    }
#else
    int Mission_SpyPlaneOverfly() override { JMP_THIS(0x4157C0); }
#endif
#else
    int Mission_SpyPlaneOverfly() override;
#endif
    /// VA: 0x00415A50
#if defined(RA2_YRPP_GAME)
    int Mission_Retreat() override { JMP_THIS(0x00415A50); }
#else
    int Mission_Retreat() override;
#endif
    /// VA: 0x00415C60
#if defined(RA2_YRPP_GAME)
    int ParadropCargo() { JMP_THIS(0x00415C60); }
#else
    int ParadropCargo();
#endif

    /// VA: 0x004197C0.
#if defined(RA2_YRPP_GAME)
    AbstractClass* FindFireLocation(AbstractClass* pTarget) { JMP_THIS(0x4197C0); }
#else
    AbstractClass* FindFireLocation(AbstractClass* pTarget);
#endif
    /// VA: 0x419B00
#if defined(RA2_YRPP_GAME)
    bool IsCellClearToLand(const CellStruct* cell, bool includeSelf) { JMP_THIS(0x419B00); }
#else
    bool IsCellClearToLand(const CellStruct* cell, bool includeSelf);
#endif

protected:
    explicit __forceinline AircraftClass(noinit_t) noexcept : FootClass(noinit_t()) {}

    // Properties

public:
    AircraftTypeClass* Type;
    bool ShouldLoseAmmo;  // Whether or not to deduct ammo after firing run (strafing) is over
    bool HasPassengers;   //parachutes
    bool IsKamikaze;      // when crashing down, duh
    BuildingClass* DockNowHeadingTo;
    bool unknown_bool_6D0;
    bool unknown_bool_6D1;
    bool IsLocked;  // Whether or not aircraft is locked to a firing run (strafing)
    char NumParadropsLeft;
    bool IsCarryallNotLanding;
    bool IsReturningFromAttackRun;  // Aircraft finished attack run and/or went idle and is now returning from it
};
