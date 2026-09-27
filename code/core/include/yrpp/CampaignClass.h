#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/CCINIClass.h"

class CampaignClass : public AbstractTypeClass {

public:
    /// VA: 0x0046CF80.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;
    /// VA: 0x0046D000; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x0046D000); }
    /// VA: 0x0046D050; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x0046D050); }
    /// VA: 0x004103E0; delegate to the existing local base body.
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize) override { return AbstractClass::GetSizeMax(pcbSize); }
    /// VA: 0x00410480; local body in core/src/yrpp.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x0046D080.
    virtual AbstractType WhatAmI() const override;
    /// VA: 0x0046D070.
    virtual int Size() const override;
    /// VA: 0x0046CFC0; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x0046CFC0); }
    /// VA: 0x004104B0; local body in core/src/yrpp.
    virtual int GetArrayIndex() const override;
    /// VA: 0x0046CCD0; reference retained, not a local implementation.
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x0046CCD0); }
    /// VA: 0x00410B90; delegate to the existing local base body.
    virtual bool SaveToINI(CCINIClass* pINI) override { return AbstractTypeClass::SaveToINI(pINI); }

    static const AbstractType AbsID = AbstractType::Campaign;
    // Original A83CF8 registry, omitted from the upstream interface.
    static DynamicVectorClass<CampaignClass*>& Array;
    static CampaignClass* YRPP_FASTCALL Find(const char* id);
    virtual ~CampaignClass();

    /// VA: 0x0046CB60.
    CampaignClass(const char* name) noexcept : CampaignClass(noinit_t()) { JMP_THIS(0x0046CB60); }
    CampaignClass(const char* name, const wchar_t* initialDescription);

protected:
    explicit __forceinline CampaignClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

public:
    /// VA: 0x0046CE10.
    static void YRPP_FASTCALL CreateFromINIList(CCINIClass *pINI)
        { JMP_STD(0x46CE10); }

    /// VA: 0x0046CC90.
    static signed int YRPP_FASTCALL FindIndex(const char* name);

public:
    int idxCD;
    char Scenario[512];
    int FinalMovie;
    wchar_t Description[128];
};
