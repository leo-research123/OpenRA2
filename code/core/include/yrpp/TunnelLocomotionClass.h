// Locomotor = {4A582743-9839-11d1-B709-00A024DDAFD1}

#pragma once

#include "yrpp/LocomotionClass.h"

class __declspec(align(4)) NOVTABLE __declspec(uuid("4A582743-9839-11D1-B709-00A024DDAFD1")) TunnelLocomotionClass : public LocomotionClass
{
public:

    /// Global VA: 0x007F5B20.
    DEFINE_REFERENCE(double const, TunnelMovementSpeed, 0x7F5B20u)

    enum State
    {
        Idle = 0,
        PreDigIn = 1,
        DiggingIn = 2,
        DugIn = 3,
        Digging = 4,
        PreDigOut = 5,
        DiggingOut = 6,
        DugOut = 7
    };

    // TODO stub virtuals implementations

    /// VA: 0x007291F0.
    bool ProcessPreDigIn()
        { JMP_THIS(0x7291F0); }

    /// VA: 0x00729370.
    bool ProcessDiggingIn()
        { JMP_THIS(0x729370); }

    /// VA: 0x007294E0.
    bool ProcessDugIn()
        { JMP_THIS(0x7294E0); }

    /// VA: 0x00729580.
    bool ProcessDigging()
        { JMP_THIS(0x729580); }

    /// VA: 0x007298F0.
    bool ProcessPreDigOut()
        { JMP_THIS(0x7298F0); }

    /// VA: 0x00729AA0.
    bool ProcessDiggingOut()
        { JMP_THIS(0x729AA0); }

    /// VA: 0x00729480.
    bool ProcessDugOut()
        { JMP_THIS(0x729480); }

    /// VA: 0x00728A00.
    TunnelLocomotionClass()
        : TunnelLocomotionClass(noinit_t())
    { JMP_THIS(0x728A00); }

protected:
    explicit __forceinline TunnelLocomotionClass(noinit_t)
        : LocomotionClass(noinit_t())
    { }

    // Properties

public:

    TunnelLocomotionClass::State State;
    CoordStruct Coords;
    RateTimer DigTimer;
    bool bool38;
};

static_assert(sizeof(TunnelLocomotionClass) == 0x3C);
