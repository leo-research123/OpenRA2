#pragma once

#include "yrpp/AbstractClass.h"
#include "yrpp/Timer.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/GeneralDefinitions.h"

#include "yrpp/Helpers/CompileTime.h"

// forward declarations
class AircraftClass;
class AbstractClass;
class CellClass;

class Kamikaze {
public:
    struct KamikazeControl {
        AircraftClass* Item;
        CellClass* Cell;
    };

    /// Global VA: 0x00ABC5F8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(Kamikaze, Instance, 0xABC5F8u)
#else
    static Kamikaze& Instance;
#endif

    Kamikaze() noexcept : UpdateTimer(100), Nodes() {}

    /// VA: 0x0054E690.
#if defined(RA2_YRPP_GAME)
    ~Kamikaze() { JMP_THIS(0x54E690); }
#else
    ~Kamikaze();
#endif

    /// VA: 0x0054E3B0.
#if defined(RA2_YRPP_GAME)
    void Add(AircraftClass* pAircraft, AbstractClass* target) { JMP_THIS(0x54E3B0); }
#else
    void Add(AircraftClass* pAircraft, AbstractClass* target);
#endif

    /// VA: 0x0054E590.
#if defined(RA2_YRPP_GAME)
    void Remove(AbstractClass* object) { JMP_THIS(0x54E590); }
#else
    void Remove(AbstractClass* object);
#endif

    /// VA: 0x0054E4D0.
#if defined(RA2_YRPP_GAME)
    void Update() { JMP_THIS(0x54E4D0); }
#else
    void Update();
#endif

    /// VA: 0x0054E6F0.
#if defined(RA2_YRPP_GAME)
    void Clear() { JMP_THIS(0x54E6F0); }
#else
    void Clear();
#endif

    /// VA: 0x0054E750.
    HRESULT Save(IStream* pStm) { JMP_THIS(0x54E750); }

    /// VA: 0x0054E7B0.
    HRESULT Load(IStream* pStm) { JMP_THIS(0x54E7B0); }

    CDTimerClass UpdateTimer;
    DynamicVectorClass<KamikazeControl*> Nodes;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(Kamikaze) == 0x24);
static_assert(sizeof(Kamikaze::KamikazeControl) == 0x8);
#endif
