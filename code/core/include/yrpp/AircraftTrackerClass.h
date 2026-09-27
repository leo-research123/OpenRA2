#pragma once

#include "yrpp/Helpers/CompileTime.h"

#include "yrpp/CellClass.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/ArrayClasses.h"

class TechnoClass;

// Tracks aerial units via 20x20 vectors spread across the maps for efficient search
class AircraftTrackerClass
{
public:
    /// Global VA: 0x00887888.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(AircraftTrackerClass, Instance, 0x887888u)
#else
    static AircraftTrackerClass& Instance;
#endif

    // Fills CurrentVector with items from TrackerVectors matching given range around cell.
    /// VA: 0x00412B40
    void FillCurrentVector(CellClass* pCell, int range);

    // Gets items from CurrentVector.
    /// VA: 0x004137A0
    TechnoClass* Get();

    /// VA: 0x004134A0
    void Add(TechnoClass* entry);
    /// VA: 0x004138C0
    void Update(TechnoClass* entry, CellStruct oldPos, CellStruct newPos);
    /// VA: 0x004135D0
    void Remove(TechnoClass* entry);

    /// VA: 0x00413800
    bool Clear();

    /// VA: 0x004135A0
    bool IsJumpjet(TechnoClass* entry);
    /// VA: 0x00412AC0
    int GetVectorIndex(CellStruct pos);

    // TODO write other entries

private:
    AircraftTrackerClass() {}

public:
    DynamicVectorClass<TechnoClass*> TrackerVectors[20][20];
    DynamicVectorClass<TechnoClass*> CurrentVector;
};

#if defined(RA2_YRPP_GAME)
static_assert(sizeof(AircraftTrackerClass) == 0x2598);
#endif
