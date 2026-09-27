// Locomotor = {4A582742-9839-11d1-B709-00A024DDAFD1}

#pragma once

#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("4A582742-9839-11D1-B709-00A024DDAFD1")) HoverLocomotionClass : public LocomotionClass
{
public:

    // TODO stub virtuals implementations

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~HoverLocomotionClass() RX;

    /// VA: 0x00514F70.
    void sub_514F70(bool arg)
        { JMP_THIS(0x514F70); }

    /// VA: 0x005164D0.
    void sub_5164D0(bool arg)
        { JMP_THIS(0x5164D0); }

    // Constructor
    /// VA: 0x00513C20.
    HoverLocomotionClass()
        : LocomotionClass(noinit_t())
    { JMP_THIS(0x513C20); }

protected:
    explicit __forceinline HoverLocomotionClass(noinit_t)
        : LocomotionClass(noinit_t())
    { }

    // Properties

public:

    CoordStruct Destination;
    CoordStruct HeadToCoord;
    FacingClass LocomotionFacing;
    double MaxSpeed;
    double CurrentSpeed;
    double BoostSpeed;
    double CurrentWobbles;
    bool unknown_bool_68;
    int unknown_int_6C;
    bool unknown_bool_70;
};
