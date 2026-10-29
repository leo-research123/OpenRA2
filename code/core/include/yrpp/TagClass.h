#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TagTypeClass.h"

// forward declarations
class TriggerClass;
class ObjectClass;

class TagClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::Tag;

    // Static
    /// Global VA: 0x00B0E720.
    static DynamicVectorClass<TagClass*>& Array;

    // finds an instance using the type, or creates one
    /// VA: 0x006E52A0.
    static TagClass* YRPP_FASTCALL GetInstance(TagTypeClass* pType)
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x6E52A0);
    }
#else
        ;
#endif

    // deletes every tag in array
    /// VA: 0x006E5570.
#if defined(RA2_YRPP_GAME)
    static void YRPP_STDCALL DeleteAll() { JMP_STD(0x6E5570); }
#else
    static void YRPP_STDCALL DeleteAll();
#endif

    // notifies all tags in array that a global was updated
    /// VA: 0x006E57F0.
    static void YRPP_FASTCALL NotifyGlobalChanged(int idxGlobal)
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x6E57F0);
    }
#else
        ;
#endif

    // notifies all tags in array that a global was updated
    /// VA: 0x006E5820.
    static void YRPP_FASTCALL NotifyLocalChanged(int idxLocal)
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x6E5820);
    }
#else
        ;
#endif

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x006E5730; original retained.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x006E5730); }
    /// VA: 0x006E57A0; original retained.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x006E57A0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TagClass();

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const override;
    /// VA: unknown (legacy placeholder).
    virtual int Size() const override;
    /// VA: 0x006E56E0; original retained.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x006E56E0); }

    /// VA: 0x006E52F0
    CellStruct* GetDefaultCoords(CellStruct* output) const {
        *output = DefaultCoords;
        return output;
    }

    // contains at least one Crosses Horizontal Line event
    /// VA: 0x006E5320.
#if defined(RA2_YRPP_GAME)
    bool HasCrossesHorizontalLineEvent() const { JMP_THIS(0x6E5320); }
#else
    bool HasCrossesHorizontalLineEvent() const;
#endif

    // contains at least one Crosses Vertical Line event
    /// VA: 0x006E5300.
#if defined(RA2_YRPP_GAME)
    bool HasCrossesVerticalLineEvent() const { JMP_THIS(0x6E5300); }
#else
    bool HasCrossesVerticalLineEvent() const;
#endif

    // contains at least one Zone Entry By event
    /// VA: 0x006E5340.
#if defined(RA2_YRPP_GAME)
    bool HasZoneEntryByEvent() const { JMP_THIS(0x6E5340); }
#else
    bool HasZoneEntryByEvent() const;
#endif

    // contains at least one Allow Win action
    /// VA: 0x006E5360.
    bool HasAllowWinAction() const { JMP_THIS(0x6E5360); }

    // called when a global is updated
    /// VA: 0x006E55A0.
    void GlobalChanged(int idxGlobal)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E55A0);
    }
#else
        ;
#endif

    // called when a local is updated
    /// VA: 0x006E55B0.
    void LocalChanged(int idxLocal)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E55B0);
    }
#else
        ;
#endif

    // whether there exist no other tag having the same type
    // note: this is not the same as this->InstanceCount
    /// VA: 0x006E5850.
    bool IsOnlyInstanceOfType() const { JMP_THIS(0x6E5850); }

    /// VA: 0x006E53A0.
    bool RaiseEvent(TriggerEvent event, ObjectClass* pTagOwner, CellStruct location, bool forceAllOccured = false,
                    TechnoClass* pSource = nullptr)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E53A0);
    }
#else
        ;
#endif

    // whether the tag transfers when the owner is "changed",
    // like vehicle thief to tank and vice versa
    /// VA: 0x6E57C0
#if defined(RA2_YRPP_GAME)
    bool ShouldReplace() const { JMP_THIS(0x6E57C0); }
#else
    bool ShouldReplace() const;
#endif

    /// VA: 0x006E5230.
    void Destroy()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E5230);
    }
#else
        ;
#endif
    bool HasBeenDestroyed() const { return this->Destroyed; }

    // adds a trigger to the list
    /// VA: 0x006E55C0.
    void AddTrigger(TriggerClass* pTrigger) { JMP_THIS(0x6E55C0); }

    // removes a trigger from the list
    /// VA: 0x006E55D0.
    bool RemoveTrigger(TriggerClass* pTrigger) { JMP_THIS(0x6E55D0); }

    // check whether the trigger is contained in the list
    /// VA: 0x006E5380.
    bool ContainsTrigger(TriggerClass* pTrigger) const
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x6E5380);
    }
#else
        ;
#endif

    // Native scenario construction can supply explicit default coordinates.
    TagClass(TagTypeClass* type, const CellStruct& defaultCoords);

    // Constructor
    /// VA: 0x006E4DE0.
#if defined(RA2_YRPP_GAME)
    TagClass(TagTypeClass* pType) noexcept : TagClass(noinit_t()) { JMP_THIS(0x6E4DE0); }
#else
    TagClass(TagTypeClass* type) noexcept;
#endif

protected:
    explicit __forceinline TagClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties

public:
    TagTypeClass* Type;
    TriggerClass* FirstTrigger;
    int InstanceCount;
    CellStruct DefaultCoords;
    bool Destroyed;
    bool IsExecuting;
    PROTECTED_PROPERTY(BYTE, padding_36[2]);
};
