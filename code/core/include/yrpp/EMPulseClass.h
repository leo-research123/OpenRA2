/*
    EMP - no, you're NOT seeing things :P
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

class NOVTABLE EMPulseClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::EMPulse;

    // Static
    /// Global VA: 0x008A3870.
    DEFINE_REFERENCE(DynamicVectorClass<EMPulseClass*>, Array, 0x8A3870u)

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~EMPulseClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    // Constructor
    /// VA: 0x004C52B0.
    EMPulseClass(CellStruct dwCrd, int nSpread, int nDuration,
        TechnoClass* pGenerator) noexcept : EMPulseClass(noinit_t())
    { JMP_THIS(0x4C52B0); }

protected:
    explicit __forceinline EMPulseClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    CellStruct BaseCoords;
    int Spread;
    int CreationTime;	//frame in which this EMP got created
    int Duration;
};
