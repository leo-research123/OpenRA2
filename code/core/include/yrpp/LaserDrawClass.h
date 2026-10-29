/*
    Lasers
*/

#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/StageClass.h"
#include "yrpp/ArrayClasses.h"

class LaserDrawClass {
public:
    // Constructor, Destructor
    LaserDrawClass(const CoordStruct& source, const CoordStruct& target, const ColorStruct& innerColor,
                   const ColorStruct& outerColor, const ColorStruct& outerSpread, int duration)
        : LaserDrawClass(source, target, 0, 1, innerColor, outerColor, outerSpread, duration) {}

    /// VA: 0x0054FE60
    LaserDrawClass(CoordStruct source, CoordStruct target, int zAdjust, BYTE unknown, ColorStruct innerColor,
                   ColorStruct outerColor, ColorStruct outerSpread, int duration, bool blinks = false,
                   bool fades = true, float startIntensity = 1.0f, float endIntensity = 0.0f)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x54FE60);
    }
#else
        ;
#endif

    /// VA: 0x0054FFB0
    ~LaserDrawClass()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x54FFB0);
    }
#else
        ;
#endif

    /// VA: 0x00550080
    void Update()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x550080);
    }
#else
        ;
#endif
    /// VA: 0x00550150
    static void YRPP_CDECL UpdateAll()
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x550150);
    }
#else
        ;
#endif
    /// VA: 0x00550240
    static void YRPP_CDECL DrawAll()
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x550240);
    }
#else
        ;
#endif
    /// VA: 0x00550260
    void Draw()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x550260);
    }
#else
        ;
#endif
    /// VA: 0x005509F0
    void DrawHouseColor()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x5509F0);
    }
#else
        ;
#endif
    /// VA: 0x00550000
    static void YRPP_CDECL DeleteAll()
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x550000);
    }
#else
        ;
#endif
    // Original global collection; static storage does not change object layout.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<LaserDrawClass*>, Array, 0xABC878u)
#else
    static DynamicVectorClass<LaserDrawClass*>& Array;
#endif

    // Properties
public:
    DECLARE_PROPERTY(StageClass, Progress);
    int Thickness;  // only respected if IsHouseColor
    bool IsHouseColor;
    bool IsSupported;  // this changes the values for InnerColor (false: halve, true: double), HouseColor only
    PROTECTED_PROPERTY(BYTE, align_22[2]);
    CoordStruct Source;
    CoordStruct Target;
    int ZAdjust;
    char field_40;
    ColorStruct InnerColor;
    ColorStruct OuterColor;
    ColorStruct OuterSpread;
    PROTECTED_PROPERTY(BYTE, align_4A[2]);
    int Duration;
    bool Blinks;
    bool BlinkState;
    bool Fades;
    PROTECTED_PROPERTY(BYTE, align_53);
    float StartIntensity;
    float EndIntensity;
};
