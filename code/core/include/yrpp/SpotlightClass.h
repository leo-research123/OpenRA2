#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/GeneralDefinitions.h"

#include "yrpp/Helpers/CompileTime.h"

class SpotlightClass
{
public:
    // Static
    /// Global VA: 0x00AC1678.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<SpotlightClass*>, Array, 0xAC1678u)

    // Destructor
    ~SpotlightClass()
        { THISCALL(0x5FF2D0); }

    /// VA: 0x005FF850.
    void Draw()
        { JMP_THIS(0x5FF850); }

    /// VA: 0x005FF320.
    void Update()
        { JMP_THIS(0x5FF320); }

    /// VA: 0x005FFFA0.
    static void YRPP_FASTCALL DrawAll()
        { JMP_STD(0x5FFFA0); }

    // Constructor
    /// VA: 0x005FF250.
    SpotlightClass(CoordStruct coords, int size)
        { JMP_THIS(0x5FF250); }


#else
    static DynamicVectorClass<SpotlightClass*>& Array;
    /// VA: 0x005FF2D0
    ~SpotlightClass();
    /// VA: 0x005FF850
    void Draw();
    /// VA: 0x005FF320
    void Update();
    /// VA: 0x005FFFA0
    static void YRPP_FASTCALL DrawAll();
    /// VA: 0x005FF250
    SpotlightClass(CoordStruct coords, int size);
#endif

    // Properties

public:

    CoordStruct Coords;
    int MovementRadius;
    int Size;
    SpotlightFlags DisableFlags;
};
