#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"

// forward declarations
class CCINIClass;
class HouseTypeClass;
class TActionClass;
class TEventClass;

class TriggerTypeClass : public AbstractTypeClass
{
public:
    static const AbstractType AbsID = AbstractType::TriggerType;

    // Array
    static DynamicVectorClass<TriggerTypeClass*>& Array;
    static TriggerTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x00727AA0
    static TriggerTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id) noexcept;

    // IPersist
    /// VA: 0x00727BB0.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    /// VA: 0x00727BF0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x00727BF0); }
    /// VA: 0x00727C80; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x00727C80); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TriggerTypeClass();

    // AbstractClass
    /// VA: 0x00727090; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x00727CA0.
    virtual AbstractType WhatAmI() const override;
    /// VA: 0x00727CB0.
    virtual int Size() const override;
    /// VA: 0x00727B30; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00727B30); }

    // AbstractTypeClass
    /// VA: 0x00727CC0; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x00727240
    virtual bool LoadFromINI(CCINIClass* pINI) override;
    /// VA: 0x007276A0; reference retained, not a local implementation.
    virtual bool SaveToINI(CCINIClass* pINI) override { JMP_THIS(0x007276A0); }

    // static
    /// VA: 0x007275D0.
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI) noexcept;

    /// VA: 0x00727880.
    static void YRPP_FASTCALL SaveToINIList(CCINIClass* pINI)
        { JMP_STD(0x727880); }

    /// VA: 0x00727120.
    TriggerTypeClass* YRPP_FASTCALL FindByNameOrID(char const* pName)
        { JMP_STD(0x727120); }

    // non-virtual
    using Flags = BYTE; // same as trigger and event flags?
    /// VA: 0x007271E0.
    Flags GetFlags() const
        { JMP_THIS(0x7271E0); }

    // contains at least one Allow Win action
    /// VA: 0x00726FE0.
    bool HasAllowWinAction() const
        { JMP_THIS(0x726FE0); }

    // contains at least one Global Set or Global Cleared event
    /// VA: 0x00727010.
    bool HasGlobalSetOrClearedEvent(int idxGlobal) const
        { JMP_THIS(0x727010); }

    // contains at least one Local Set or Local Cleared event
    /// VA: 0x00727050.
    bool HasLocalSetOrClearedEvent(int idxLocal) const
        { JMP_THIS(0x727050); }

    // contains at least one Crosses Horizontal Line event
    /// VA: 0x00726F80.
    bool HasCrossesHorizontalLineEvent() const
        { JMP_THIS(0x726F80); }

    // contains at least one Crosses Vertical Line event
    /// VA: 0x00726F50.
    bool HasCrossesVerticalLineEvent() const
        { JMP_THIS(0x726F50); }

    // contains at least one Zone Entry By event
    /// VA: 0x00726FB0.
    bool HasZoneEntryByEvent() const
        { JMP_THIS(0x726FB0); }

    // deletes an action from the list
    /// VA: 0x007279E0.
    bool RemoveAction(TActionClass* pAction)
        { JMP_THIS(0x7279E0); }

    // deletes an event from the list
    /// VA: 0x00727A40.
    bool RemoveEvent(TEventClass* pEvent)
        { JMP_THIS(0x727A40); }

    // Constructor
    /// VA: 0x00726C80.
    TriggerTypeClass(char const* pName);

protected:
    explicit __forceinline TriggerTypeClass(noinit_t)
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:
    int ArrayIndex;
    bool Difficulty[3]; // easy = 0, normal = 1, hard = 2
    bool Enabled;
    bool MustTransfer; // vehicle thieves must take Tag with it when hijacking
    PROTECTED_PROPERTY(BYTE, align_A1[3]);
    HouseTypeClass* House;
    TriggerTypeClass* NextTrigger;
    TEventClass* FirstEvent;
    TActionClass* FirstAction;
};
