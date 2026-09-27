/*
    Factories are responsible for producing units and buildings.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/StageClass.h"

class HouseClass;
class TechnoClass;
class TechnoTypeClass;

class NOVTABLE FactoryClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Factory;

    /// Global VA: 0x00A83E30.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<FactoryClass*>, Array, 0xA83E30u)
#else
    static DynamicVectorClass<FactoryClass*>& Array;
#endif
    /// VA: 0x004CA6E0
    static void UpdateBuildSpeed(HouseClass* owner);

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    #if defined(RA2_YRPP_GAME)
    virtual ~FactoryClass() RX;
#else
    ~FactoryClass() override;
#endif

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: unknown (legacy placeholder).
    int Size() const override { return sizeof(*this); }
    /// VA: 0x4C9B20
    void Update() override;

    // non-virtual

    // returns whether progress "IsDifferent" and resets the flag
    /// VA: 0x004C9C60.
#if defined(RA2_YRPP_GAME)
    bool HasProgressChanged() { JMP_THIS(0x4C9C60); }
#else
    bool HasProgressChanged();
#endif

    /// VA: 0x004C9C70.
#if defined(RA2_YRPP_GAME)
    bool DemandProduction(TechnoTypeClass const* pType, HouseClass* pOwner, bool shouldQueue) { JMP_THIS(0x4C9C70); }
#else
    bool DemandProduction(TechnoTypeClass const* pType, HouseClass* pOwner, bool shouldQueue);
#endif

    // aborts current product and puts the object in, completed and suspended
    /// VA: 0x004C9E10.
#if defined(RA2_YRPP_GAME)
    void SetObject(TechnoClass* pObject) { JMP_THIS(0x4C9E10); }
#else
    void SetObject(TechnoClass* pObject);
#endif

    /// VA: 0x004C9E60.
#if defined(RA2_YRPP_GAME)
    bool Suspend(bool manual) { JMP_THIS(0x4C9E60); }
#else
    bool Suspend(bool manual);
#endif

    /// VA: 0x004C9EA0.
#if defined(RA2_YRPP_GAME)
    bool Unsuspend(bool manual) { JMP_THIS(0x4C9EA0); }
#else
    bool Unsuspend(bool manual);
#endif

    /// VA: 0x004C9FB0.
#if defined(RA2_YRPP_GAME)
    int GetBuildTimeFrames() const { JMP_THIS(0x4C9FB0); }
#else
    int GetBuildTimeFrames() const;
#endif

    /// VA: 0x004C9FF0.
#if defined(RA2_YRPP_GAME)
    bool AbandonProduction() { JMP_THIS(0x4C9FF0); }
#else
    bool AbandonProduction();
#endif

    // returns Production.Value
    /// VA: 0x004CA120.
#if defined(RA2_YRPP_GAME)
    int GetProgress() const { JMP_THIS(0x4CA120); }
#else
    int GetProgress() const;
#endif

    /// VA: 0x004CA130.
#if defined(RA2_YRPP_GAME)
    bool IsDone() const { JMP_THIS(0x4CA130); }
#else
    bool IsDone() const;
#endif

    /// VA: 0x004CA180.
#if defined(RA2_YRPP_GAME)
    int GetCostPerStep() const { JMP_THIS(0x4CA180); }
#else
    int GetCostPerStep() const;
#endif

    // checks the progress and updates the state if done
    /// VA: 0x004CA1A0.
#if defined(RA2_YRPP_GAME)
    bool CompletedProduction() { JMP_THIS(0x4CA1A0); }
#else
    bool CompletedProduction();
#endif

    // builds an item from the queue
    /// VA: 0x004CA5A0.
#if defined(RA2_YRPP_GAME)
    void StartProduction() { JMP_THIS(0x4CA5A0); }
#else
    void StartProduction();
#endif

    /// VA: 0x004CA620.
#if defined(RA2_YRPP_GAME)
    bool RemoveOneFromQueue(TechnoTypeClass const* pItem) { JMP_THIS(0x4CA620); }
#else
    bool RemoveOneFromQueue(TechnoTypeClass const* pItem);
#endif

    // in queue and in production
    /// VA: 0x004CA670.
#if defined(RA2_YRPP_GAME)
    int CountTotal(TechnoTypeClass const* pType) const { JMP_THIS(0x4CA670); }
#else
    int CountTotal(TechnoTypeClass const* pType) const;
#endif

    // whether at least one item is queued, not in production
    /// VA: 0x004CA6B0.
#if defined(RA2_YRPP_GAME)
    bool IsQueued(TechnoTypeClass const* pType) const { JMP_THIS(0x4CA6B0); }
#else
    bool IsQueued(TechnoTypeClass const* pType) const;
#endif

    static FactoryClass* FindByOwnerAndProduct(
        HouseClass const* const pHouse, TechnoTypeClass const* const pItem)
    {
        for(auto const& pFact : FactoryClass::Array) {
            if(pFact->Owner == pHouse) {
                if(pFact->CountTotal(pItem) > 0) {
                    return pFact;
                }
            }
        }
        return nullptr;
    }

    // Constructor
    /// VA: 0x004C98B0.
#if defined(RA2_YRPP_GAME)
    FactoryClass() noexcept : FactoryClass(noinit_t()) { JMP_THIS(0x4C98B0); }
#else
    FactoryClass() noexcept;
#endif

protected:
    explicit __forceinline FactoryClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:
    StageClass      Production; // hardcoded to be 54 steps (so cameo clock should be 54 frames)
    DynamicVectorClass<TechnoTypeClass*> QueuedObjects;
    TechnoClass*       Object;
    bool               OnHold; // paused when out of money, restored when funds available
    bool               IsDifferent;	// changed progress
    PROTECTED_PROPERTY(BYTE, align_5E[2]);
    int                Balance; // credits house still owes us for building this
    int                OriginalBalance;
    int                SpecialItem; // -1 = none, else Iron Curtain? (was EMPulse in TS)
    HouseClass*        Owner;
    bool               IsSuspended; //completed production, before next (or waiting to place)
    bool               IsManual; // whether the current suspension state was caused by the player
    PROTECTED_PROPERTY(BYTE, padding_72[2]);
};
