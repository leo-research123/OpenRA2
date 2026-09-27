/*
    TaskForces as in the AI inis
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"

// forward declarations
class TechnoTypeClass;

struct TaskForceEntryStruct
{
    int Amount;
    TechnoTypeClass* Type;
};

class TaskForceClass : public AbstractTypeClass
{
public:
    /// VA: 0x006E8750; local body in core/src/yrpp.
    virtual void ComputeCRC(CRCEngine& crc) const override;
    /// VA: 0x006E8420; reference retained, not a local implementation.
    virtual bool LoadFromINI(CCINIClass* pINI) override;
    /// VA: 0x006E8510; reference retained, not a local implementation.
    virtual bool SaveToINI(CCINIClass* pINI) override;

    static const AbstractType AbsID = AbstractType::TaskForce;

    // Array
    static DynamicVectorClass<TaskForceClass*>& Array;
    static TaskForceClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x006E85F0
    static TaskForceClass* YRPP_FASTCALL FindOrAllocate(const char* id) noexcept;

    // IPersist
    /// VA: 0x006E8710.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x006E86A0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override;
    /// VA: 0x006E8680; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override;

    // Destructor
    /// Implementation/provenance: matching core/src/yrpp source.
    virtual ~TaskForceClass();

    // AbstractClass
    /// VA: 0x006E87D0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x006E87E0.
    virtual int Size() const;

    /// VA: 0x006E8220
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI, int scope) noexcept;
    /// VA: 0x006E8780
    int GetRequiredTechLevel() const;

    // Constructor
    /// VA: 0x006E7E80.
    TaskForceClass(const char* pID) noexcept;

protected:
    explicit __forceinline TaskForceClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    int     Group;
    int     CountEntries;
    int     IsGlobal;
    TaskForceEntryStruct Entries [0x6];
};
