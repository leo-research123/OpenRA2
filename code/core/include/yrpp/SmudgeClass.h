/*
    Smudges
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/SmudgeTypeClass.h"

class NOVTABLE SmudgeClass : public ObjectClass
{
public:
    // Native scenario-load overload of ReadINI. Parses [Smudge] and applies
    // the loading-time Mark/Place result without retaining temporary objects.
    // This status/diagnostic signature is not the original binary entry ABI.
    // Resets rejectedRecords; missing sections succeed. Exceptions return false
    // and do not escape; earlier placements remain if a later record fails.
    /// VA: 0x006B4C80
    static bool ReadINI(CCINIClass& ini, unsigned int& rejectedRecords) noexcept;

    static const AbstractType AbsID = AbstractType::Smudge;

    // Static
    /// Global VA: 0x00A8B1E0.
    DEFINE_REFERENCE(DynamicVectorClass<SmudgeClass*>, Array, 0xA8B1E0u)

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~SmudgeClass() RX;

    // AbstractClass
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    virtual ObjectTypeClass* GetType() const override;

    // Constructor
    /// VA: 0x006B4A50.
    SmudgeClass(SmudgeTypeClass* pType) noexcept
        : SmudgeClass(noinit_t())
    { JMP_THIS(0x6B4A50); }

protected:
    explicit __forceinline SmudgeClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    SmudgeTypeClass* Type;

};
