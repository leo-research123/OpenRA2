#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"

class TechnoClass;
class InfantryTypeClass;
class CellClass;

class HouseClass;
class InfantryClass;

enum class SlaveManagerStatus : unsigned int {
    Ready = 0,
    Scanning = 1,
    Travelling = 2,
    Deploying = 3,
    Working = 4,
    ScanningAgain = 5,
    PackingUp = 6
};

enum class SlaveControlStatus : unsigned int {
    Unknown = 0,
    ScanningForTiberium = 1,
    MovingToTiberium = 2,
    Harvesting = 3,
    BringingItBack = 4,
    Respawning = 5,
    Dead = 6
};

class NOVTABLE SlaveManagerClass : public AbstractClass {
public:
    /// VA: 0x006B0CC0
#if defined(RA2_YRPP_GAME)
    void BeginScanning() { JMP_THIS(0x6B0CC0); }
#else
    void BeginScanning();
#endif
    /// VA: 0x006AF5F0
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x6AF5F0); }
#else
    void Update() override;
#endif

    struct SlaveControl {
        InfantryClass* Slave;
        SlaveControlStatus State;
        CDTimerClass RespawnTimer;
    };

    static const AbstractType AbsID = AbstractType::SlaveManager;

    // Static
    /// Global VA: 0x00B0B5F0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<SlaveManagerClass*>, Array, 0xB0B5F0u)
#else
    static DynamicVectorClass<SlaveManagerClass*>& Array;
#endif

    // IPersist
    /// VA: 0x6B1130
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override { JMP_THIS(0x6B1130); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;
#endif

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x6AF4A0
#if defined(RA2_YRPP_GAME)
    virtual ~SlaveManagerClass() { JMP_THIS(0x6AF4A0); }
#else
    ~SlaveManagerClass() override;
#endif

    // AbstractClass
    /// VA: 0x6B1380
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x6B1370
    int Size() const override { return 0x64; }

    // non-virtual
    /// VA: 0x006B0880
#if defined(RA2_YRPP_GAME)
    bool IsSlaveAtCell(InfantryClass* slave, CellClass* cell) { JMP_THIS(0x6B0880); }
#else
    bool IsSlaveAtCell(InfantryClass* slave, CellClass* cell);
#endif
    /// VA: 0x006AF580.
#if defined(RA2_YRPP_GAME)
    void SetOwner(TechnoClass* NewOwner) { JMP_THIS(0x6AF580); }
#else
    void SetOwner(TechnoClass* NewOwner);
#endif

    /// VA: 0x006AF650.
#if defined(RA2_YRPP_GAME)
    void CreateSlave(SlaveControl* Node) { JMP_THIS(0x6AF650); }
#else
    void CreateSlave(SlaveControl* Node);
#endif

    /// VA: 0x006B0A20.
#if defined(RA2_YRPP_GAME)
    void LostSlave(InfantryClass* Slave) { JMP_THIS(0x6B0A20); }
#else
    void LostSlave(InfantryClass* Slave);
#endif

    /// VA: 0x006B0D60.
#if defined(RA2_YRPP_GAME)
    void Deploy2() { JMP_THIS(0x6B0D60); }
#else
    void Deploy2();
#endif

    /// VA: 0x6B0D10
#if defined(RA2_YRPP_GAME)
    void Deploy() { JMP_THIS(0x6B0D10); }
#else
    void Deploy();
#endif
    /// VA: 0x6B0DB0
#if defined(RA2_YRPP_GAME)
    void Harvest() { JMP_THIS(0x6B0DB0); }
#else
    void Harvest();
#endif
    /// VA: 0x6AF6C0
#if defined(RA2_YRPP_GAME)
    void UpdateSlaves() { JMP_THIS(0x6AF6C0); }
#else
    void UpdateSlaves();
#endif
    /// VA: 0x6AFD60
#if defined(RA2_YRPP_GAME)
    void UpdateOwner() { JMP_THIS(0x6AFD60); }
#else
    void UpdateOwner();
#endif
    /// VA: 0x6B0300
#if defined(RA2_YRPP_GAME)
    CellStruct* FindDeployCell(CellStruct* output, CellStruct target) { JMP_THIS(0x6B0300); }
#else
    CellStruct* FindDeployCell(CellStruct* output, CellStruct target);
#endif
    /// VA: 0x6B0490
#if defined(RA2_YRPP_GAME)
    void GuardSlaves() { JMP_THIS(0x6B0490); }
#else
    void GuardSlaves();
#endif
    /// VA: 0x6B04C0
#if defined(RA2_YRPP_GAME)
    void DeploySlaves() { JMP_THIS(0x6B04C0); }
#else
    void DeploySlaves();
#endif
    /// VA: 0x6B0690
#if defined(RA2_YRPP_GAME)
    CellStruct* GetExitCell(CellStruct* output) { JMP_THIS(0x6B0690); }
#else
    CellStruct* GetExitCell(CellStruct* output);
#endif
    /// VA: 0x6B0750
#if defined(RA2_YRPP_GAME)
    CellStruct* GetAlternateExitCell(CellStruct* output) { JMP_THIS(0x6B0750); }
#else
    CellStruct* GetAlternateExitCell(CellStruct* output);
#endif

    // switches the slaves to the killer house with cheers and hoorahs
    // note that this->Owner will be NULL once this function is done
    /// VA: 0x006B0AE0.
#if defined(RA2_YRPP_GAME)
    void Killed(TechnoClass* Killer, HouseClass* ForcedOwnerHouse = nullptr) { JMP_THIS(0x6B0AE0); }
#else
    void Killed(TechnoClass* killer, HouseClass* forcedOwnerHouse = nullptr);
#endif

    /// VA: 0x006B0C80.
#if defined(RA2_YRPP_GAME)
    void AllGuard() { JMP_THIS(0x6B0C80); }
#else
    void AllGuard();
#endif

    /// VA: 0x006B1020.
#if defined(RA2_YRPP_GAME)
    bool ShouldWakeUpNow() { JMP_THIS(0x6B1020); }
#else
    bool ShouldWakeUpNow();
#endif

    // the slaves will become free citizens without any announcements or cheers, if you don't call Killed() beforehand
    void ZeroOutSlaves();

    // stops scanning, spawning slaves and driving around.
    void SuspendWork() {
        this->RespawnTimer.StartTime = -1;
        if (!this->RespawnTimer.TimeLeft) {
            this->RespawnTimer.TimeLeft = 1;
        }
    }

    // resumes to harvest automatically.
    void ResumeWork() { this->RespawnTimer.Resume(); }

    // Constructor
    /// VA: 0x006AF1A0.
#if defined(RA2_YRPP_GAME)
    SlaveManagerClass(TechnoClass* pOwner, InfantryTypeClass* pSlave, int num, int RegenRate, int ReloadRate) noexcept
        : SlaveManagerClass(noinit_t()) {
        JMP_THIS(0x6AF1A0);
    }
#else
    SlaveManagerClass(TechnoClass* owner, InfantryTypeClass* slave, int count, int regenRate, int reloadRate) noexcept;
#endif

protected:
    explicit __forceinline SlaveManagerClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

public:
    // Properties

    TechnoClass* Owner;
    InfantryTypeClass* SlaveType;
    int SlaveCount;
    int RegenRate;
    int ReloadRate;
    DynamicVectorClass<SlaveControl*> SlaveNodes;
    CDTimerClass RespawnTimer;
    SlaveManagerStatus State;
    int LastScanFrame;
};
