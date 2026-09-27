#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AircraftClass.h"

enum class SpawnManagerStatus : unsigned int
{
    Idle = 0, // no target or out of range
    Launching = 1, // one launch in progress
    CoolDown = 2 // waiting for launch to complete
};

enum class SpawnNodeStatus : unsigned int
{
    Idle = 0, // docked, waiting for target
    TakeOff = 1, // missile tilting and launch
    Preparing = 2, // gathering, waiting
    Attacking = 3, // attacking until no ammo
    Returning = 4, // return to carrier
    // Unused_5, // not used
    Reloading = 6, // docked, reloading ammo and health
    Dead = 7 // respawning
};

struct SpawnControl
{
    AircraftClass* Unit;
    SpawnNodeStatus Status;
    CDTimerClass SpawnTimer;
    BOOL IsSpawnMissile;
};

class NOVTABLE SpawnManagerClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::SpawnManager;
    /// VA: 0x006B7230
    void Update() override { JMP_THIS(0x6B7230); }

    // Static
    /// Global VA: 0x00B0B880.
    DEFINE_REFERENCE(DynamicVectorClass<SpawnManagerClass*>, Array, 0xB0B880u)

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~SpawnManagerClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    // non-virtual
    /// VA: 0x006B7100.
    void KillNodes()
        { JMP_THIS(0x6B7100); }

    /// VA: 0x006B7B90.
    void SetTarget(AbstractClass* pTarget)
        { JMP_THIS(0x6B7B90); }

    /// VA: 0x006B7C40.
    bool UpdateTarget()
        { JMP_THIS(0x6B7C40); }

    /// VA: 0x006B7BB0.
    void ResetTarget()
        { JMP_THIS(0x6B7BB0); }

    /// VA: 0x006B7D30.
    int CountAliveSpawns() const
        { JMP_THIS(0x6B7D30); }

    /// VA: 0x006B7D50.
    int CountDockedSpawns() const
        { JMP_THIS(0x6B7D50); }

    /// VA: 0x006B7D80.
    int CountLaunchingSpawns() const
        { JMP_THIS(0x6B7D80); }

    /// VA: 0x006B7C60.
    void UnlinkPointer(AbstractClass* pRemove)
        { JMP_THIS(0x6B7C60); }

    // Constructor
    /// VA: 0x006B6C90.
    SpawnManagerClass(
        TechnoClass* pOwner, AircraftTypeClass* pSpawnType, int nMaxNodes,
        int RegenRate, int ReloadRate) noexcept : SpawnManagerClass(noinit_t())
    { JMP_THIS(0x6B6C90); }

protected:
    explicit __forceinline SpawnManagerClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    TechnoClass* Owner;
    AircraftTypeClass* SpawnType;
    int SpawnCount;
    int RegenRate;
    int ReloadRate;
    DynamicVectorClass<SpawnControl*> SpawnedNodes;
    CDTimerClass UpdateTimer;
    CDTimerClass SpawnTimer;
    AbstractClass* Target;
    AbstractClass* NewTarget;
    SpawnManagerStatus Status;
};
