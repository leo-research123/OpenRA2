/*
    I have not the slightest idea what this is good for...
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

class NOVTABLE NeuronClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Neuron;

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
    virtual ~NeuronClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);

    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    // Constructor
    /// VA: 0x0043A350.
    NeuronClass() noexcept
        : NeuronClass(noinit_t())
    { JMP_THIS(0x43A350); }

protected:
    explicit __forceinline NeuronClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    void* unknown_ptr_24;
    void* unknown_ptr_28;
    void* unknown_ptr_2C;
    CDTimerClass unknown_timer_30;
};

// Even more questions marks on the use of this... >.<
class BrainClass
{
public:
    /// VA: unknown (legacy placeholder).
    virtual ~BrainClass() RX;

    BrainClass()
        { THISCALL(0x43A600); }

    // Properties
    VectorClass<NeuronClass*> Neurons;	//???
};
