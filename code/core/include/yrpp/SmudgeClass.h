/*
    Smudges
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/SmudgeTypeClass.h"

class NOVTABLE SmudgeClass : public ObjectClass {
public:
    // Parses [Smudge] and applies the loading-time Mark/Place result without
    // retaining temporary objects. A missing section is not an error; malformed
    // records are skipped and an allocation failure throws, earlier placements remain.
    /// VA: 0x006B4C80
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI) { JMP_STD(0x6B4C80); }
#else
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI);
#endif

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
    SmudgeClass(SmudgeTypeClass* pType) noexcept : SmudgeClass(noinit_t()) { JMP_THIS(0x6B4A50); }

protected:
    explicit __forceinline SmudgeClass(noinit_t) noexcept : ObjectClass(noinit_t()) {}

    // Properties

public:
    SmudgeTypeClass* Type;
};
