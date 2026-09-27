/*
    Ion Cannons
    If DisableIonBeam is set, no Ion Beam will be shown but just the impact effect.
    Used by the Psychic Dominator
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GeneralDefinitions.h"

class IonBlastClass
{
public:
    /// Global VA: 0x00AA0118.
    DEFINE_REFERENCE(DynamicVectorClass<IonBlastClass*>, Array, 0xAA0118u)

    /// VA: 0x0053D310.
    static void UpdateAll()
        { JMP_STD(0x53D310); }

    /// VA: 0x0053CBE0.
    void Update()
        { JMP_THIS(0x53CBE0); }

    /// VA: 0x0053D850.
    static void YRPP_FASTCALL DrawAll()
        { JMP_STD(0x53D850); }

    // Constructor, Destructor
    /// VA: 0x0053CB10.
    IonBlastClass(CoordStruct Crd)
        { JMP_THIS(0x53CB10); }

    /// VA: 0x0053CB90.
    ~IonBlastClass()
        { JMP_THIS(0x53CB90); }

    // Properties

public:

    CoordStruct Location;
    int Lifetime;
    BOOL DisableIonBeam;	//0 = no, 1 = yes

};
