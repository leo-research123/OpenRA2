#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ToggleClass.h"

class ConvertClass;
struct SHPStruct;

enum class ShapeButtonFlag : int
{
    UpShape = 0x0,
    DownShape = 0x1,
    DisabledShape = 0x2
};

class NOVTABLE ShapeButtonClass : public ToggleClass
{
public:
    // ShapeButtonClass
    /// VA: unknown (legacy placeholder).
    virtual void SetShape(SHPStruct* pSHP, int Width, int Height);
    bool Draw(bool forced) override;

    // Non virtual
    /// VA: 0x004B5630.
    ShapeButtonClass& operator=(ShapeButtonClass const& another)
        JMP_THIS(0x4B5630);

    /// VA: 0x0069DE70.
    void ClearShape()
        JMP_THIS(0x69DE70);

    /// VA: 0x0069DEA0.
    void SetLoadedStatus()
        JMP_THIS(0x69DEA0);

    /// VA: 0x0069DFC0.
    bool StartFlashing(int nDelay, int nInitDelay, bool bStart)
        JMP_THIS(0x69DFC0);

    /// VA: 0x0069DFF0.
    bool StopFlashing()
        JMP_THIS(0x69DFF0);

    /// VA: 0x0069E010.
    bool AI()
        JMP_THIS(0x69E010); // Flash_AI

    /// VA: 0x0069E050.
    bool IsFlashing()
        JMP_THIS(0x69E050);

    // bool ToFlash()
        // JMP_THIS(0x69E060); // no need for this function

    // Statics
    /// VA: 0x006CFCC0.
    static int YRPP_FASTCALL FindIndex(const char* name)
        JMP_STD(0x6CFCC0);

    /// VA: 0x006CFD40.
    static ShapeButtonClass* YRPP_FASTCALL GetButton(int index)
        JMP_STD(0x6CFD40);

    /// VA: 0x006D09C0.
    static void YRPP_FASTCALL SetToolTip(ShapeButtonClass* pButton, const char* tip)
        JMP_STD(0x6D09C0);

    // Constructors
    /// VA: 0x0069DCF0.
    ShapeButtonClass() noexcept;

    /// VA: 0x0069DD30.
    ShapeButtonClass(unsigned int nID,SHPStruct* shape,int nX,int nY,int nWidth,int nHeight,bool bIsAlpha) noexcept;

protected:
    explicit __forceinline ShapeButtonClass(noinit_t) noexcept
        : ToggleClass(noinit_t())
    { }

    // Properties
public:
    bool IsToFlash;
    int FlashDelay;
    int FlashCounter;
    bool UseFlash; // ReflectButtonState
    Point2D DrawPosition;
    bool UseSidebarSurface;
    ConvertClass* Drawer;
    bool IsDrawn;
    bool IsAlpha;
    SHPStruct* ShapeData;
    bool IsShapeLoaded;
};
