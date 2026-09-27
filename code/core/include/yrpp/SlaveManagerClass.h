#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

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

class NOVTABLE SlaveManagerClass : public AbstractClass
{
public:
    /// VA: 0x006B0CC0
    void BeginScanning() { JMP_THIS(0x6B0CC0); }
    /// VA: 0x006AF5F0
    void Update() override { JMP_THIS(0x6AF5F0); }

    struct SlaveControl {
        InfantryClass* Slave;
        SlaveControlStatus State;
        CDTimerClass RespawnTimer;
    };

    static const AbstractType AbsID = AbstractType::SlaveManager;

    // Static
    /// Global VA: 0x00B0B5F0.
    DEFINE_REFERENCE(DynamicVectorClass<SlaveManagerClass*>, Array, 0xB0B5F0u)

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
    virtual ~SlaveManagerClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    // non-virtual
    /// VA: 0x006B0880
    bool IsSlaveAtCell(InfantryClass* slave, CellClass* cell) { JMP_THIS(0x6B0880); }
    /// VA: 0x006AF580.
    void SetOwner(TechnoClass *NewOwner)
        { JMP_THIS(0x6AF580); }

    /// VA: 0x006AF650.
    void CreateSlave(SlaveControl *Node)
        { JMP_THIS(0x6AF650); }

    /// VA: 0x006B0A20.
    void LostSlave(InfantryClass *Slave)
        { JMP_THIS(0x6B0A20); }

    /// VA: 0x006B0D60.
    void Deploy2()
        { JMP_THIS(0x6B0D60); }

    // switches the slaves to the killer house with cheers and hoorahs
    // note that this->Owner will be NULL once this function is done
    /// VA: 0x006B0AE0.
    void Killed(TechnoClass *Killer, HouseClass * ForcedOwnerHouse = nullptr)
        { JMP_THIS(0x6B0AE0); }

    /// VA: 0x006B0C80.
    void AllGuard()
        { JMP_THIS(0x6B0C80); }

    /// VA: 0x006B1020.
    bool ShouldWakeUpNow()
        { JMP_THIS(0x6B1020); }

    // the slaves will become free citizens without any announcements or cheers, if you don't call Killed() beforehand
    void ZeroOutSlaves();

    // stops scanning, spawning slaves and driving around.
    void SuspendWork() {
        this->RespawnTimer.StartTime = -1;
        if(!this->RespawnTimer.TimeLeft) {
            this->RespawnTimer.TimeLeft = 1;
        }
    }

    // resumes to harvest automatically.
    void ResumeWork() {
        this->RespawnTimer.Resume();
    }

    // Constructor
    /// VA: 0x006AF1A0.
    SlaveManagerClass(
        TechnoClass* pOwner, InfantryTypeClass* pSlave, int num, int RegenRate,
        int ReloadRate) noexcept : SlaveManagerClass(noinit_t())
    { JMP_THIS(0x6AF1A0); }

protected:
    explicit __forceinline SlaveManagerClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

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
