// Locomotor = {92612C46-F71F-11d1-AC9F-006008055BB5}

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("92612C46-F71F-11D1-AC9F-006008055BB5")) JumpjetLocomotionClass : public LocomotionClass, public IPiggyback
{
public:

    enum State
    {
        Grounded = 0,
        Ascending = 1,
        Hovering = 2,
        Cruising = 3,
        Descending = 4,
        Crashing = 5,
        Unknown = 6,
    };

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
    virtual ~JumpjetLocomotionClass() RX;

    // LocomotionClass
    /// VA: unknown (legacy placeholder).
    virtual	int Size() R0;

    // JumpjetLocomotionClass

    // Constructor
    /// VA: 0x0054AC40.
    JumpjetLocomotionClass()
        : LocomotionClass(noinit_t())
    { JMP_THIS(0x54AC40); }

protected:
    explicit __forceinline JumpjetLocomotionClass(noinit_t)
        : LocomotionClass(noinit_t())
    { }

    // Properties

public:

    int TurnRate;
    int Speed;
    float Climb;
    float Crash;
    int Height;
    float Accel;
    float Wobbles;
    int Deviation;
    bool NoWobbles;
    BYTE unknown_3D;
    BYTE unknown_3E;
    BYTE unknown_3F;
    CoordStruct DestinationCoords;
    bool IsMoving;
    BYTE unknown_4D;
    BYTE unknown_4E;
    BYTE unknown_4F;
    JumpjetLocomotionClass::State State;
    FacingClass LocomotionFacing;
    BYTE unknown_6C;
    BYTE unknown_6D;
    BYTE unknown_6E;
    BYTE unknown_6F;
    double CurrentSpeed;
    double MaxSpeed;
    int CurrentHeight;
    BYTE unknown_84;
    BYTE unknown_85;
    BYTE unknown_86;
    BYTE unknown_87;
    double CurrentWobbles;
    bool DestinationReached;
    BYTE unknown_91;
    BYTE unknown_92;
    BYTE unknown_93;
    ILocomotion* Piggybackee;
};

static_assert(sizeof(JumpjetLocomotionClass) == 0x98);
