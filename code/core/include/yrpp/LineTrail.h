#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/ArrayClasses.h"

#include "yrpp/Helpers/CompileTime.h"
class ObjectClass;

struct LineTrailNode
{
    CoordStruct Position;
    int Value;
};

class LineTrail
{
public:
    /// Global VA: 0x00ABCB78.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<LineTrail*>, Array, 0xABCB78u)
#else
    static DynamicVectorClass<LineTrail*>& Array;
#endif

    // Constructor, Destructor
    /// VA: 0x00556A20.
    LineTrail();

    /// VA: 0x00556B30.
    ~LineTrail();

    // Original destructor detaches without freeing: the array retains the
    // fading tail. Native callers use this without ending the C++ lifetime.
    /// VA: 0x00556B30
    void Detach();

    /// VA: 0x00556B50.
    void SetDecrement(int val);

    /// VA: 0x00556B70
    void Update();
    /// VA: 0x00556C00
    void Draw() const;
    // Render the current simulation snapshot without advancing fade twice.
    static void DrawAll();
    // Native update/reclaim half of the original update-and-draw pass.
    /// VA: 0x00556D40
    static void UpdateAll();

    /// VA: 0x00556DF0.
    static void DeleteAll();

    // Properties

public:

    ColorStruct Color;
    ObjectClass* Owner;
    int Decrement;
    int ActiveSlot;
    LineTrailNode Trails[32];
};
