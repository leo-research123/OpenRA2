// Locomotor = {2BEA74E1-7CCA-11d3-BE14-00104B62A16C}

#pragma once

#include "yrpp/LocomotionClass.h"

class NOVTABLE __declspec(uuid("2BEA74E1-7CCA-11D3-BE14-00104B62A16C")) ShipLocomotionClass : public LocomotionClass, public IPiggyback
{
public:

    // TODO stub virtuals implementations

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~ShipLocomotionClass() RX;

    // Constructor
    /// VA: 0x0069EC50.
    ShipLocomotionClass()
        : ShipLocomotionClass(noinit_t())
    { JMP_THIS(0x69EC50); }

protected:
    explicit __forceinline ShipLocomotionClass(noinit_t)
        : LocomotionClass(noinit_t())
    { }

    // Properties

public:

    int CurrentRamp;
    int PreviousRamp;
    RateTimer SlopeTimer;
    CoordStruct Destination;
    CoordStruct HeadToCoord;
    int SpeedAccum;
    double movementspeed_50;
    int TrackNumber;
    int TrackIndex;
    bool IsOnShortTrack;
    BYTE IsTurretLockedDown;
    bool IsRotating;
    bool IsDriving;
    bool IsRocking;
    bool UnLocked;
    ILocomotion* Piggybackee;
};

static_assert(sizeof(ShipLocomotionClass) == 0x70);
