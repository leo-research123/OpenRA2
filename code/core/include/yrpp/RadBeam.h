#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/ArrayClasses.h"

#include "yrpp/Helpers/CompileTime.h"

class TechnoClass;

class RadBeam {
public:
    /// Global VA: 0x00B04A60.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<RadBeam*>, Array, 0xB04A60u)
#else
    static DynamicVectorClass<RadBeam*>& Array;
#endif

    // Constructor removed - do not use it, use Allocate instead

    ~RadBeam() = default;

    /// VA: 0x00659110.
#if defined(RA2_YRPP_GAME)
    static RadBeam* YRPP_FASTCALL Allocate(RadBeamType mode) { JMP_STD(0x659110); }
#else
    static RadBeam* YRPP_FASTCALL Allocate(RadBeamType mode);
#endif
    /// VA: 0x6591B0
#if defined(RA2_YRPP_GAME)
    static void YRPP_CDECL DrawAll() { JMP_STD(0x6591B0); }
#else
    static void YRPP_CDECL DrawAll();
#endif
    static void YRPP_CDECL DeleteAll();
    /// VA: 0x659650
#if defined(RA2_YRPP_GAME)
    bool DrawBeam() { JMP_THIS(0x659650); }
#else
    bool DrawBeam();
#endif
    /// VA: 0x659AC0
#if defined(RA2_YRPP_GAME)
    void PrepareBeamSegment() { JMP_THIS(0x659AC0); }
#else
    void PrepareBeamSegment();
#endif

    /// VA: 0x659CA0
#if defined(RA2_YRPP_GAME)
    bool DrawEruption() { JMP_THIS(0x659CA0); }
#else
    bool DrawEruption();
#endif
    /// VA: 0x659E30
#if defined(RA2_YRPP_GAME)
    void InitializeEruption() { JMP_THIS(0x659E30); }
#else
    void InitializeEruption();
#endif
    /// VA: 0x659EE0
#if defined(RA2_YRPP_GAME)
    void AdvanceEruption() { JMP_THIS(0x659EE0); }
#else
    void AdvanceEruption();
#endif
    /// VA: 0x659FF0
#if defined(RA2_YRPP_GAME)
    void PrepareEruptionSegment() { JMP_THIS(0x659FF0); }
#else
    void PrepareEruptionSegment();
#endif

    void SetColor(const ColorStruct& color) { this->Color = color; }

    void SetCoordsSource(const CoordStruct& loc) { this->SourceLocation = loc; }

    void SetCoordsTarget(const CoordStruct& loc) { this->TargetLocation = loc; }

    // Properties

public:
    TechnoClass* unknown_0;
    TechnoClass* Owner;
    byte unknown_8;
    byte unknown_9;

    /**
     * if there's difference in the Y coord of SourceLocation and TargetLocation,
     * they're both converted to screen coords (2D)
     * and the difference of those Y coords is taken as this field
     */
    DWORD unknown_C;

    RadBeamType Type;
    DWORD unknown_14;
    double unknown_18;
    ColorStruct Color;
    CoordStruct SourceLocation;  //FLH
    CoordStruct TargetLocation;
    DWORD Period;
    double Amplitude;
    double unknown_48;
    DWORD unknown_50;
    DWORD unknown_54;
    byte unknown_58;
    DWORD unknown_5C;
    DWORD unknown_60;
    DWORD unknown_64;
    double unknown_68;
    CoordStruct AnotherLocation;
    DWORD unknown_7C;
    double unknown_80;
    DWORD unknown_88;
    DWORD unknown_8C;
    CoordStruct AndAnotherLocation;
    DWORD unknown_9C;
    DWORD unknown_A0;
    DWORD unknown_A4;
    DWORD unknown_A8;
    DWORD unknown_AC;
    DWORD unknown_B0;
    DWORD unknown_B4;
    double unknown_B8;
    byte unknown_C0;
    byte unknown_C1;
    byte unknown_C2;
    DWORD unknown_C4;
};
