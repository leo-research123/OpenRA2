#pragma once

#include "yrpp/AbstractClass.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/BombClass.h"
#include "yrpp/Helpers/CompileTime.h"

// forward declarations
class ObjectClass;
class TechnoClass;

// this class contains a vector of BombClass, a vector of bomb-revealing TechnoClass, and some other properties
class BombListClass {
public:
    /// Global VA: 0x0087F5D8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(BombListClass, Instance, 0x87F5D8u)
#else
    static BombListClass& Instance;
#endif

    // Retire detached bombs and update ticking audio and visibility.
    /// VA: 0x00438BF0
#if defined(RA2_YRPP_GAME)
    void Update() { JMP_THIS(0x438BF0); }
#else
    void Update();
#endif

    // the main one, ivan planting a bomb (creates a BombClass inside)
    /// VA: 0x00438E70
#if defined(RA2_YRPP_GAME)
    void Plant(TechnoClass* SourceObject, ObjectClass* TargetObject) { JMP_THIS(0x438E70); }
#else
    void Plant(TechnoClass* SourceObject, ObjectClass* TargetObject);
#endif

    // duh
    /// VA: 0x00439080
#if defined(RA2_YRPP_GAME)
    void AddDetector(TechnoClass* Detector) { JMP_THIS(0x439080); }
#else
    void AddDetector(TechnoClass* Detector);
#endif

    // duh
    /// VA: 0x004390D0
#if defined(RA2_YRPP_GAME)
    void RemoveDetector(TechnoClass* Detector) { JMP_THIS(0x4390D0); }
#else
    void RemoveDetector(TechnoClass* Detector);
#endif

    /// VA: 0x00439150
#if defined(RA2_YRPP_GAME)
    void PointerGotInvalid(AbstractClass* pInvalid) { JMP_THIS(0x439150); }
#else
    void PointerGotInvalid(AbstractClass* pInvalid);
#endif
    // Empties both lists, keeping their capacity, and resets the update delay.
    /// VA: 0x00439110
#if defined(RA2_YRPP_GAME)
    void Clear() { JMP_THIS(0x439110); }
#else
    void Clear();
#endif

protected:
    // Properties
public:
    DynamicVectorClass<BombClass*> Bombs;        // all the BombClass instances on the map
    DynamicVectorClass<TechnoClass*> Detectors;  // all the BombSight'ed objects currently on the map
    int UpdateDelay = 100;                       // defaults to 100, some iterators set it to 1
};
