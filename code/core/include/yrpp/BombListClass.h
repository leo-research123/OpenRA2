#pragma once

#include "yrpp/AbstractClass.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/BombClass.h"
#include "yrpp/Helpers/CompileTime.h"

// forward declarations
class ObjectClass;
class TechnoClass;

// this class contains a vector of BombClass, a vector of bomb-revealing TechnoClass, and some other properties
class BombListClass
{
public:
    /// Global VA: 0x0087F5D8.
    DEFINE_REFERENCE(BombListClass, Instance, 0x87F5D8u)

    // draws all the visible bombs, expires the outdated ones
    /// VA: 0x00438BF0.
    void Update()
        { JMP_THIS(0x438BF0); }

    // the main one, ivan planting a bomb (creates a BombClass inside)
    /// VA: 0x00438E70.
    void Plant(TechnoClass *SourceObject, ObjectClass *TargetObject)
        { JMP_THIS(0x438E70); }

    // duh
    /// VA: 0x00439080.
    void AddDetector(TechnoClass *Detector)
        { JMP_THIS(0x439080); }

    // duh
    /// VA: 0x004390D0.
    void RemoveDetector(TechnoClass *Detector)
        { JMP_THIS(0x4390D0); }

    /// VA: 0x00439150.
    void PointerGotInvalid(AbstractClass* pInvalid)
        { JMP_THIS(0x439150); }

protected:
    // Properties
public:
    DynamicVectorClass<BombClass *> Bombs;       // all the BombClass instances on the map
    DynamicVectorClass<TechnoClass *> Detectors; // all the BombSight'ed objects currently on the map
    int UpdateDelay;                             // defaults to 100, some iterators set it to 1
};
