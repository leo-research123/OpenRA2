/*
    Electric Bolts
*/

#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/TechnoClass.h"

class UnitClass;

class EBolt {
public:
    /// Global VA: 0x008A0E88.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<EBolt*>, Array, 0x8A0E88u)
#else
    static DynamicVectorClass<EBolt*>& Array;
#endif

    // Constructor, Destructor
    /// VA: 0x004C1E10
#if defined(RA2_YRPP_GAME)
    EBolt() { JMP_THIS(0x4C1E10); }
#else
    EBolt();
#endif

    /// VA: 0x004C2C10
#if defined(RA2_YRPP_GAME)
    ~EBolt() { JMP_THIS(0x4C2C10); }
#else
    ~EBolt();
#endif

    /// VA: 0x004C2BD0
#if defined(RA2_YRPP_GAME)
    void SetOwner(UnitClass* pOwner, int idxWeapon) { JMP_THIS(0x4C2BD0); }
#else
    void SetOwner(UnitClass* pOwner, int idxWeapon);
#endif

    /// VA: 0x004C1E50
#if defined(RA2_YRPP_GAME)
    void ClearOwner() { JMP_THIS(0x4C1E50); }
#else
    void ClearOwner();
#endif

    /// VA: 0x004C2B40
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetSourceCoords(CoordStruct* outBuffer) const { JMP_THIS(0x4C2B40); }
#else
    CoordStruct* GetSourceCoords(CoordStruct* outBuffer) const;
#endif

    CoordStruct GetSourceCoords() const {
        CoordStruct buffer;
        GetSourceCoords(&buffer);
        return buffer;
    }

    /// VA: 0x004C2A60
#if defined(RA2_YRPP_GAME)
    void Fire(CoordStruct P1, CoordStruct P2, DWORD arg18) { JMP_THIS(0x4C2A60); }
#else
    void Fire(CoordStruct P1, CoordStruct P2, DWORD arg18);
#endif

    /// VA: 0x004C1F20
#if defined(RA2_YRPP_GAME)
    void Draw(CoordStruct first, CoordStruct last, int colorIndex) { JMP_THIS(0x4C1F20); }
#else
    void Draw(CoordStruct first, CoordStruct last, int colorIndex);
#endif
    /// VA: 0x004C2790
#if defined(RA2_YRPP_GAME)
    void DrawSingle() { JMP_THIS(0x4C2790); }
#else
    void DrawSingle();
#endif
    /// VA: 0x004C2830
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL DrawAll() { JMP_STD(0x4C2830); }
#else
    static void YRPP_CDECL DrawAll();
#endif
    /// VA: 0x004C29E0
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL DeleteAll() { JMP_STD(0x4C29E0); }
#else
    static void YRPP_CDECL DeleteAll();
#endif

    // Properties

public:
    CoordStruct Point1;
    CoordStruct Point2;
    DWORD unknown_18;    //Duration?
    int Random;          //Random number between 0 and 256
    TechnoClass* Owner;  //ingame this is a UnitClass but needed to circumvent some issues
    int WeaponSlot;      // which weapon # to use from owner
    int Lifetime;        // this is >>= 1 each time DrawAll() is called, 0 => dtor (inline). Hi, welcome to dumb ideas.
    bool AlternateColor;
};
