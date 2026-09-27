/*
    Aircraft
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/FootClass.h"
#include "yrpp/AircraftTypeClass.h"

// AircraftClass
class NOVTABLE AircraftClass : public FootClass, public IFlyControl
{
public:
    /// VA: 0x0041C1D0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x0041C1D0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    static const AbstractType AbsID = AbstractType::Aircraft;
    /// VA: 0x004144B0
    void DrawIt(Point2D* point, RectangleStruct* bounds) const override;

    /// VA: 0x0041ADF0
#if defined(RA2_YRPP_GAME)
    void See(DWORD incremental, DWORD dontMap) override { JMP_THIS(0x41ADF0); }
#else
    void See(DWORD incremental, DWORD dontMap) override;
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
        using Entry=int(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B6A0)(static_cast<IFlyControl*>(this));
    }
#else
    int YRPP_STDCALL Landing_Altitude() override;
#endif
    /// VA: 0x0041B760
#if defined(RA2_YRPP_GAME)
    int YRPP_STDCALL Landing_Direction() override {
        using Entry=int(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B760)(static_cast<IFlyControl*>(this));
    }
#else
    int YRPP_STDCALL Landing_Direction() override;
#endif
    /// VA: 0x0041B7D0
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Loaded() override {
        using Entry=long(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B7D0)(static_cast<IFlyControl*>(this));
    }
#else
    long YRPP_STDCALL Is_Loaded() override;
#endif
    /// VA: 0x0041B7F0
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Strafe() override {
        using Entry=long(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B7F0)(static_cast<IFlyControl*>(this));
    }
#else
    long YRPP_STDCALL Is_Strafe() override;
#endif
    /// VA: 0x0041B840
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Fighter() override {
        using Entry=long(YRPP_STDCALL*)(IFlyControl*);
        return reinterpret_cast<Entry>(0x0041B840)(static_cast<IFlyControl*>(this));
    }
#else
    long YRPP_STDCALL Is_Fighter() override;
#endif
    /// VA: 0x0041B860
#if defined(RA2_YRPP_GAME)
    long YRPP_STDCALL Is_Locked() override {
        using Entry=long(YRPP_STDCALL*)(IFlyControl*);
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
    AircraftClass(AircraftTypeClass* pType, HouseClass* pOwner) noexcept
        : AircraftClass(noinit_t())
    { JMP_THIS(0x413D20); }
#else
    AircraftClass(AircraftTypeClass* pType, HouseClass* pOwner) noexcept;
#endif

    /// VA: 0x0041B890
#if defined(RA2_YRPP_GAME)
    bool IsLeavingMap() const override { JMP_THIS(0x41B890); }
#else
    bool IsLeavingMap() const override;
#endif
    /// VA: 0x0041B660
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object,bool removed) override { JMP_THIS(0x41B660); }
#else
    void PointerExpired(AbstractClass* object,bool removed) override;
#endif
    /// VA: 0x004165C0
#if defined(RA2_YRPP_GAME)
    DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) override { JMP_THIS(0x4165C0); }
#else
    DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) override;
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
    bool Unlimbo(const CoordStruct& at,DirType facing) override { JMP_THIS(0x414310); }
#else
    bool Unlimbo(const CoordStruct& at,DirType facing) override;
#endif
    /// VA: 0x0041B5E0
#if defined(RA2_YRPP_GAME)
    bool ReadyToNextMission() const override { JMP_THIS(0x41B5E0); }
#else
    bool ReadyToNextMission() const override;
#endif
    /// VA: 0x004166C0
#if defined(RA2_YRPP_GAME)
    int Mission_Move() override { JMP_THIS(0x4166C0); }
#else
    int Mission_Move() override;
#endif
    /// VA: 0x0041AA80
#if defined(RA2_YRPP_GAME)
    void SetDestination(AbstractClass* target,bool immediate) override { JMP_THIS(0x41AA80); }
#else
    void SetDestination(AbstractClass* target,bool immediate) override;
#endif
    /// VA: 0x004176F0
#if defined(RA2_YRPP_GAME)
    bool EnterIdleMode(bool initial,bool resume) override { JMP_THIS(0x4176F0); }
#else
    bool EnterIdleMode(bool initial,bool resume) override;
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
    AbstractClass* FindFireLocation(AbstractClass* pTarget)
        { JMP_THIS(0x4197C0); }

protected:
    explicit __forceinline AircraftClass(noinit_t) noexcept
        : FootClass(noinit_t())
    { }

    // Properties

public:

    AircraftTypeClass* Type;
    bool ShouldLoseAmmo; // Whether or not to deduct ammo after firing run (strafing) is over
    bool HasPassengers;	//parachutes
    bool IsKamikaze; // when crashing down, duh
    BuildingClass* DockNowHeadingTo;
    bool unknown_bool_6D0;
    bool unknown_bool_6D1;
    bool IsLocked; // Whether or not aircraft is locked to a firing run (strafing)
    char NumParadropsLeft;
    bool IsCarryallNotLanding;
    bool IsReturningFromAttackRun; // Aircraft finished attack run and/or went idle and is now returning from it
};
