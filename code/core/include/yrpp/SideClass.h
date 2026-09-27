/*
    Sides
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"

class SideClass : public AbstractTypeClass
{
public:
    /// VA: 0x006A4710; local body in core/src/yrpp.
    virtual void ComputeCRC(CRCEngine& crc) const override;
    static const AbstractType AbsID = AbstractType::Side;

    // Array
    static DynamicVectorClass<SideClass*>& Array;
    static SideClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x006A4740.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x006A4780; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override;
    /// VA: 0x006A48A0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override;

    // Destructor
    /// Implementation/provenance: matching core/src/yrpp source.
    virtual ~SideClass();

    // AbstractClass
    /// VA: 0x006A4920.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x006A4910.
    virtual int Size() const;

    // Constructor
    /// VA: 0x006A4550.
    SideClass(const char* pID) noexcept;

protected:
    explicit __forceinline SideClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    TypeList<int> HouseTypes;	//indices!

};
