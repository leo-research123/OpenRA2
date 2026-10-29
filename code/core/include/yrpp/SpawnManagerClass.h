#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AircraftClass.h"

enum class SpawnManagerStatus : unsigned int {
    Idle = 0,       // no target or out of range
    Launching = 1,  // one launch in progress
    CoolDown = 2    // waiting for launch to complete
};

enum class SpawnNodeStatus : unsigned int {
    Idle = 0,       // docked, waiting for target
    TakeOff = 1,    // missile tilting and launch
    Preparing = 2,  // gathering, waiting
    Attacking = 3,  // attacking until no ammo
    Returning = 4,  // return to carrier
    // Unused_5, // not used
    Reloading = 6,  // docked, reloading ammo and health
    Dead = 7        // respawning
};

struct SpawnControl {
    AircraftClass* Unit;
    SpawnNodeStatus Status;
    CDTimerClass SpawnTimer;
    BOOL IsSpawnMissile;
};

class NOVTABLE SpawnManagerClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::SpawnManager;
    /// VA: 0x006B7230
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x6B7230); }
#else
    void Update() override;
#endif

    // Static
    /// Global VA: 0x00B0B880.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<SpawnManagerClass*>, Array, 0xB0B880u)
#else
    static DynamicVectorClass<SpawnManagerClass*>& Array;
#endif

    // IPersist
    /// VA: 0x6B7ED0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override { JMP_STD(0x6B7ED0); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* id) override;
#endif

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x6B7010
#if defined(RA2_YRPP_GAME)
    ~SpawnManagerClass() override { JMP_THIS(0x6B7010); }
#else
    ~SpawnManagerClass() override;
#endif

    // AbstractClass
    /// VA: 0x6B8130
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x6B8120
    int Size() const override { return 0x74; }

    // non-virtual
    /// VA: 0x006B7100.
#if defined(RA2_YRPP_GAME)
    void KillNodes() { JMP_THIS(0x6B7100); }
#else
    void KillNodes();
#endif

    /// VA: 0x006B7B90
#if defined(RA2_YRPP_GAME)
    void SetTarget(AbstractClass* pTarget) { JMP_THIS(0x6B7B90); }
#else
    void SetTarget(AbstractClass* pTarget);
#endif

    /// VA: 0x006B7C40
#if defined(RA2_YRPP_GAME)
    bool UpdateTarget() { JMP_THIS(0x6B7C40); }
#else
    bool UpdateTarget();
#endif

    /// VA: 0x006B7BB0.
#if defined(RA2_YRPP_GAME)
    void ResetTarget() { JMP_THIS(0x6B7BB0); }
#else
    void ResetTarget();
#endif

    /// VA: 0x006B7D30.
#if defined(RA2_YRPP_GAME)
    int CountAliveSpawns() const { JMP_THIS(0x6B7D30); }
#else
    int CountAliveSpawns() const;
#endif

    /// VA: 0x006B7D50.
#if defined(RA2_YRPP_GAME)
    int CountDockedSpawns() const { JMP_THIS(0x6B7D50); }
#else
    int CountDockedSpawns() const;
#endif

    /// VA: 0x006B7D80
#if defined(RA2_YRPP_GAME)
    int CountLaunchingSpawns() const { JMP_THIS(0x6B7D80); }
#else
    int CountLaunchingSpawns() const;
#endif

    /// VA: 0x006B7C60.
#if defined(RA2_YRPP_GAME)
    void UnlinkPointer(AbstractClass* pRemove) { JMP_THIS(0x6B7C60); }
#else
    void UnlinkPointer(AbstractClass* pRemove);
#endif

    // Constructor
    /// VA: 0x006B6C90.
#if defined(RA2_YRPP_GAME)
    SpawnManagerClass(TechnoClass* pOwner, AircraftTypeClass* pSpawnType, int nMaxNodes, int RegenRate,
                      int ReloadRate) noexcept
        : SpawnManagerClass(noinit_t()) {
        JMP_THIS(0x6B6C90);
    }
#else
    SpawnManagerClass(TechnoClass* owner, AircraftTypeClass* type, int count, int regen, int reload) noexcept;
#endif

protected:
    explicit __forceinline SpawnManagerClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

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

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SpawnControl) == 0x18);
static_assert(sizeof(SpawnManagerClass) == 0x74);
static_assert(offsetof(SpawnManagerClass, Target) == 0x68);
static_assert(offsetof(SpawnManagerClass, NewTarget) == 0x6C);
#endif
