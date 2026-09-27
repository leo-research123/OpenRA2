/*
    [ScriptTypes]
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"

struct ScriptActionNode
{
    int Action;
    int Argument;
};

// forward declarations
class TechnoTypeClass;

class ScriptTypeClass : public AbstractTypeClass
{
public:
    /// VA: 0x00691E30; local body in core/src/yrpp.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x00691E00; local body in core/src/yrpp.
    virtual void ComputeCRC(CRCEngine& crc) const override;
    /// VA: 0x00691F90; local body in core/src/yrpp.
    virtual int GetArrayIndex() const override;
    /// VA: 0x006918A0; local body in core/src/yrpp.
    virtual bool LoadFromINI(CCINIClass* pINI) override;
    /// VA: 0x006917F0; local body in core/src/yrpp.
    virtual bool SaveToINI(CCINIClass* pINI) override;
    static const AbstractType AbsID = AbstractType::ScriptType;

    // Array
    static DynamicVectorClass<ScriptTypeClass*>& Array;
    static ScriptTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x00691C00.
    static ScriptTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);
    // IPersist
    /// VA: 0x00691D50.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x00691D90; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override;
    /// VA: 0x00691DE0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override;

    // Destructor
    /// Implementation/provenance: matching core/src/yrpp source.
    virtual ~ScriptTypeClass();

    // AbstractClass
    /// VA: 0x00691F70.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00691F80.
    virtual int Size() const;

    // AbstractTypeClass
    /// VA: 0x00691970.
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI, int scope) noexcept;
    // Constructor
    /// VA: 0x006916B0.
    ScriptTypeClass(const char* pID) noexcept;

protected:
    explicit __forceinline ScriptTypeClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    int      ArrayIndex;
    int      IsGlobal;
    int      ActionsCount;
    ScriptActionNode ScriptActions [50];
};
