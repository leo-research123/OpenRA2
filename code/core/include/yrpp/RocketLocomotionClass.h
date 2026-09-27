// Locomotor = {B7B49766-E576-11d3-9BD9-00104B972FE8}

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("B7B49766-E576-11D3-9BD9-00104B972FE8")) RocketLocomotionClass : public LocomotionClass
{
public:

    // IUnknown
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) R0;
    /// VA: unknown (legacy placeholder).
    virtual ULONG YRPP_STDCALL AddRef() R0;
    /// VA: unknown (legacy placeholder).
    virtual ULONG YRPP_STDCALL Release() R0;

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
    virtual void YRPP_STDCALL Do_Turn(DirStruct coord) RX;
    /// VA: unknown (legacy placeholder).
    virtual Layer YRPP_STDCALL In_Which_Layer() RT(Layer);
    /// VA: unknown (legacy placeholder).
    virtual void YRPP_STDCALL Mark_All_Occupation_Bits(MarkType mark) RX;
    /// VA: unknown (legacy placeholder).
    virtual void YRPP_STDCALL Limbo() RX;

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
    virtual ~RocketLocomotionClass() RX;

    // LocomotionClass
    /// VA: unknown (legacy placeholder).
    virtual	int Size() R0;

    // RocketLocomotionClass

    // Constructor
    /// VA: 0x00661EC0.
    RocketLocomotionClass()
        : RocketLocomotionClass(noinit_t())
    { JMP_THIS(0x661EC0); }

protected:
    explicit __forceinline RocketLocomotionClass(noinit_t)
        : LocomotionClass(noinit_t())
    { }

    // Properties

public:

    CoordStruct MovingDestination;
    RateTimer MissionTimer;
    CDTimerClass TrailerTimer;
    int MissionState;
    DWORD unknown_44;
    double CurrentSpeed;
    bool unknown_bool_4C;
    bool SpawnerIsElite;
    float CurrentPitch;
    DWORD unknown_58;
    DWORD unknown_5C;
};
