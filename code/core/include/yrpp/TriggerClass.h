#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"

// forward declarations
class ObjectClass;
class TechnoClass;
class TriggerTypeClass;

class TriggerClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Trigger;

    // Static
    /// Global VA: 0x00A8EAE8.
    static DynamicVectorClass<TriggerClass*>& Array;

    // finds an instance using the type, or creates one
    /// VA: 0x00726630.
    static TriggerClass* YRPP_FASTCALL GetInstance(TriggerTypeClass* pType);

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x00726860; original retained.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x00726860); }
    /// VA: 0x007268D0; original retained.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) override { JMP_STD(0x007268D0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TriggerClass();

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const override;
    /// VA: unknown (legacy placeholder).
    virtual int Size() const override;
    /// VA: 0x00726790; original retained.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00726790); }

    // contains at least one Crosses Horizontal Line event
    /// VA: 0x00726250.
    bool HasCrossesHorizontalLineEvent() const
        { JMP_THIS(0x726250); }

    // contains at least one Crosses Vertical Line event
    /// VA: 0x00726290.
    bool HasCrossesVerticalLineEvent() const
        { JMP_THIS(0x726290); }

    // contains at least one Zone Entry By event
    /// VA: 0x007262D0.
    bool HasZoneEntryByEvent() const
        { JMP_THIS(0x7262D0); }

    // contains at least one Allow Win action
    /// VA: 0x00726310.
    bool HasAllowWinAction() const
        { JMP_THIS(0x726310); }

    // contains at least one Global Set or Global Cleared event
    /// VA: 0x00726350.
    bool HasGlobalSetOrClearedEvent(int idxGlobal) const
        { JMP_THIS(0x726350); }

    // called when a global is updated. resets timers
    /// VA: 0x007263A0.
    void NotifyGlobalChanged(int idxGlobal)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x7263A0); }
#else
        ;
#endif

    // called when a local is updated. resets timers
    /// VA: 0x007263D0.
    void NotifyLocalChanged(int idxLocal)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x7263D0); }
#else
        ;
#endif

    // resets the timers for all Elapsed Time and Random Delay events
    /// VA: 0x00726400.
    void ResetTimers();

    void MarkEventAsOccured(int idx)
        { this->OccuredEvents |= (1u << idx); }
    void MarkEventAsNotOccured(int idx)
        { this->OccuredEvents &= ~(1u << idx); }
    bool HasEventOccured(int idx) const
        { return (this->OccuredEvents & (1u << idx)) != 0u; }

    /// VA: 0x00726720.
    void Destroy()
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x726720); }
#else
        ;
#endif
    bool HasBeenDestroyed() const
        { return this->Destroyed; }

    void SetHouse(HouseClass* pHouse)
        { this->House = pHouse; }
    /// VA: 0x00726910.
    HouseClass* GetHouse() const
        { return this->House; }

    // enables the trigger and resets the timers
    void Enable()
        { this->Enabled = true; this->ResetTimers(); }
    void Disable()
        { this->Enabled = false; }

    // called whenever an event bubbles up, returns true if all of this
    // trigger's events occured. persistent events are remembered
    /// VA: 0x007264C0.
    bool RegisterEvent(
        TriggerEvent event, ObjectClass* pObject, bool forceFire,
        bool persistent, TechnoClass* pSource)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x7264C0); }
#else
        ;
#endif

    // returns whether any action was executed
    /// VA: 0x007265C0.
    bool FireActions(ObjectClass* pObj, CellStruct location)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x7265C0); }
#else
        ;
#endif

    // Constructor
    /// VA: 0x00725FA0.
    TriggerClass(TriggerTypeClass* pType);

protected:
    explicit __forceinline TriggerClass(noinit_t)
        : AbstractClass(noinit_t())
    { }

    // Properties

public:
    TriggerTypeClass*	Type;
    TriggerClass*		NextTrigger;
    HouseClass*			House;
    bool				Destroyed; // ActionClass::DestroyTrigger called on
    PROTECTED_PROPERTY(BYTE, align_31[3]);
    CDTimerClass			Timer;
    DWORD				OccuredEvents; // bitfield for 32 events max
    bool				Enabled;
    PROTECTED_PROPERTY(BYTE, padding_45[3]);
};
