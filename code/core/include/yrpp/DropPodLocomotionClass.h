#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("4A582745-9839-11D1-B709-00A024DDAFD1")) DropPodLocomotionClass : public LocomotionClass, public IPiggyback
{
public:

    // IUnknown
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) R0;
    /// VA: unknown (legacy placeholder).
    virtual ULONG YRPP_STDCALL AddRef() R0;
    /// VA: unknown (legacy placeholder).
    virtual ULONG YRPP_STDCALL Release() R0;

    // IPiggyback
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Begin_Piggyback(ILocomotion* pointer) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL End_Piggyback(ILocomotion** pointer) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool YRPP_STDCALL Is_Ok_To_End() R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Piggyback_CLSID(GUID* classid) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool YRPP_STDCALL Is_Piggybacking() R0;

    // ILocomotion
    /// VA: unknown (legacy placeholder).
    virtual bool YRPP_STDCALL Is_Moving() R0;
    /// VA: unknown (legacy placeholder).
    virtual CoordStruct* YRPP_STDCALL Destination(CoordStruct* pcoord) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool YRPP_STDCALL Process() R0;
    /// VA: unknown (legacy placeholder).
    virtual void YRPP_STDCALL Move_To(CoordStruct to) RX;
    /// VA: unknown (legacy placeholder).
    virtual void YRPP_STDCALL Stop_Moving() RX;
    /// VA: unknown (legacy placeholder).
    virtual int YRPP_STDCALL Drawing_Code() R0;

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~DropPodLocomotionClass() RX;

    // LocomotionClass
    /// VA: unknown (legacy placeholder).
    virtual	int Size() R0;

    // Constructor
    /// VA: 0x004B5AB0.
    DropPodLocomotionClass()
        : DropPodLocomotionClass(noinit_t())
    {
        JMP_THIS(0x4B5AB0);
    }

protected:
    explicit __forceinline DropPodLocomotionClass(noinit_t)
        : LocomotionClass(noinit_t())
    {}

public:
    bool OutOfMap;
    CoordStruct DestinationCoords;
    ILocomotionPtr Piggybackee;
};

static_assert(sizeof(DropPodLocomotionClass) == 0x30);
