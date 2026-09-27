#pragma once

#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralDefinitions.h"

#include "yrpp/Helpers/CompileTime.h"

// forward declarations
class AircraftClass;
class CellClass;

class Kamikaze {
public:
    struct KamikazeControl {
        AircraftClass* Item;
        CellClass* Cell;
    };

    /// Global VA: 0x00ABC5F8.
    DEFINE_REFERENCE(Kamikaze, Instance, 0xABC5F8u)

    Kamikaze() noexcept : UpdateTimer(100), Nodes()
    { }

    /// VA: 0x0054E690.
    ~Kamikaze()
        { JMP_THIS(0x54E690); }

    /// VA: 0x0054E3B0.
    void Add(AircraftClass* pAircraft)
        { JMP_THIS(0x54E3B0); }

    /// VA: 0x0054E590.
    void Remove(AircraftClass* pAircraft)
        { JMP_THIS(0x54E590); }

    /// VA: 0x0054E4D0.
    void Update()
        { JMP_THIS(0x54E4D0); }

    /// VA: 0x0054E6F0.
    void Clear()
        { JMP_THIS(0x54E6F0); }

    /// VA: 0x0054E750.
    HRESULT Save(IStream* pStm)
        { JMP_THIS(0x54E750); }

    /// VA: 0x0054E7B0.
    HRESULT Load(IStream* pStm)
        { JMP_THIS(0x54E7B0); }

    CDTimerClass UpdateTimer;
    DynamicVectorClass<KamikazeControl*> Nodes;
};
