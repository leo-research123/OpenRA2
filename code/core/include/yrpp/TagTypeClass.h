#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"

// forward declarations
class CCINIClass;
class TriggerTypeClass;

class TagTypeClass : public AbstractTypeClass
{
public:
    static const AbstractType AbsID = AbstractType::TagType;

    // Array
    static DynamicVectorClass<TagTypeClass*>& Array;
    static TagTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x006E6310
    static TagTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id) noexcept;

    // IPersist
    /// VA: 0x006E63A0.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    /// VA: 0x006E6410; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x006E6410); }
    /// VA: 0x006E6470; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x006E6470); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TagTypeClass();

    // AbstractClass
    /// VA: 0x006E5E50; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x006E6490.
    virtual AbstractType WhatAmI() const override;
    /// VA: 0x006E64A0.
    virtual int Size() const override;
    /// VA: 0x006E63E0; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x006E63E0); }

    // AbstractTypeClass
    /// VA: 0x006E64B0; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x006E6080
    virtual bool LoadFromINI(CCINIClass* pINI) override;
    /// VA: 0x006E6160; reference retained, not a local implementation.
    virtual bool SaveToINI(CCINIClass* pINI) override { JMP_THIS(0x006E6160); }

    // static
    /// VA: 0x006E5ED0.
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI) noexcept;

    /// VA: 0x006E5FE0.
    static void YRPP_FASTCALL SaveToINIList(CCINIClass* pINI)
        { JMP_STD(0x6E5FE0); }

    /// VA: 0x006E5E70.
    static TagTypeClass* YRPP_FASTCALL FindByNameOrID(char const* pName)
#if defined(RA2_YRPP_GAME)
        { JMP_STD(0x6E5E70); }
#else
        ;
#endif

    // non-virtual
    using Flags = BYTE; // same as trigger and event flags?
    /// VA: 0x006E61F0.
    Flags GetFlags() const
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x6E61F0); }
#else
        ;
#endif

    // contains at least one Allow Win action
    /// VA: 0x006E6220.
    bool HasAllowWinAction() const
        { JMP_THIS(0x6E6220); }

    // contains at least one Crosses Horizontal Line event
    /// VA: 0x006E6250.
    bool HasCrossesHorizontalLineEvent() const
        { JMP_THIS(0x6E6250); }

    // contains at least one Crosses Vertical Line event
    /// VA: 0x006E6280.
    bool HasCrossesVerticalLineEvent() const
        { JMP_THIS(0x6E6280); }

    // contains at least one Zone Entry By event
    /// VA: 0x006E62B0.
    bool HasZoneEntryByEvent() const
        { JMP_THIS(0x6E62B0); }

    // adds a trigger to the list
    /// VA: 0x006E5DD0.
    bool AddTrigger(TriggerTypeClass* pTrigger)
        { JMP_THIS(0x6E5DD0); }

    // removes a trigger from the list
    /// VA: 0x006E5E00.
    bool RemoveTrigger(TriggerTypeClass* pTrigger)
        { JMP_THIS(0x6E5E00); }

    // check whether the trigger is contained in the list
    /// VA: 0x006E62E0.
    bool ContainsTrigger(TriggerTypeClass* pTrigger) const
        { JMP_THIS(0x6E62E0); }

    void DestroyTagInstances();

    // Constructor
    /// VA: 0x006E5B60.
    TagTypeClass(char const* pName);

protected:
    explicit __forceinline TagTypeClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:
    int ArrayIndex;
    TriggerPersistence Persistence;
    TriggerTypeClass* FirstTrigger;
};
