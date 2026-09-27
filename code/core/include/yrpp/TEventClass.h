#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"
class ObjectClass;

// forward declarations
class SuperClass;
class TechnoClass;
class TeamTypeClass;

class TEventClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Event;

    // Static
    /// Global VA: 0x00B0F1A0.
    static DynamicVectorClass<TEventClass*>& Array;

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x0071F8C0; original retained.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x0071F8C0); }
    /// VA: 0x0071F930; original retained.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x0071F930); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TEventClass();

    virtual void PointerExpired(AbstractClass* object, bool removed) override;
    virtual int GetArrayIndex() const override;
    /// VA: 0x0071F820; reference retained.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x0071F820); }

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const;
    /// VA: unknown (legacy placeholder).
    virtual int Size() const;

    // you are responsible for doing INI::ReadString and strtok'ing it before calling
    // this func only calls strtok again, doesn't know anything about buffers
    /// VA: 0x0071F4E0.
    void LoadFromINI()
        JMP_THIS(0x71F4E0);

    // you allocate the buffer for this, and save it to ini yourself after this returns
    // this func only sprintf's the stuff it needs into buffer
    /// VA: 0x0071F390.
    void PrepareSaveToINI(char* buffer) const
        JMP_THIS(0x71F390);

    /// VA: 0x0071F680.
    static TriggerAttachType YRPP_FASTCALL GetAttachType(int eventKind)
#if defined(RA2_YRPP_GAME)
        { JMP_STD(0x71F680); }
#else
        ;
#endif

    // used in TriggerClass::HaveEventsOccured , when trigger is repeating
    // both need to be true to check this event as done
    /// VA: 0x0071F950.
    bool GetStateA() const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x71F950); }
#else
        ;
#endif

    /// VA: 0x0071F9C0.
    bool GetStateB() const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x71F9C0); }
#else
        ;
#endif

    // Matching entry events record House for actions selecting the entering house.
    /// VA: 0x0071E940.
    bool HasOccured(
        int eventKind,
        HouseClass* pHouse,
        ObjectClass* Object,
        CDTimerClass* ActivationFrame,
        bool* isRepeating, TechnoClass* pSource = nullptr
    )
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x71E940); }
#else
        ;
#endif

    // Constructor
    /// VA: 0x0071E6A0.
    TEventClass();

protected:
    explicit __forceinline TEventClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:
    int ArrayIndex;
    TEventClass* NextEvent;
    TriggerEvent EventKind;
    TeamTypeClass* TeamType; // If this event needs to reference a team type, then this is the pointer to the team type object.
    int Value;
    char String[0x1C];
    HouseClass* House;
};
