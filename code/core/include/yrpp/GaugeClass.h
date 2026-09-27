#pragma once

#include "yrpp/ControlClass.h"

class NOVTABLE GaugeClass : public ControlClass
{
public:
    // GaugeClass
    /// VA: unknown (legacy placeholder).
    virtual bool SetMaximum(int value) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool SetValue(int value) R0;
    /// VA: unknown (legacy placeholder).
    virtual int GetValue() R0;
    /// VA: unknown (legacy placeholder).
    virtual void SetThumb(bool value) RX; // Set HasThumb
    /// VA: unknown (legacy placeholder).
    virtual int GetThumbPixel() R0; // return 4 if not overloaded
    /// VA: unknown (legacy placeholder).
    virtual void DrawThumb() RX;
    /// VA: unknown (legacy placeholder).
    virtual int PixelToValue(int pixel) R0;
    /// VA: unknown (legacy placeholder).
    virtual int ValueToPixel(int value) R0;

    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x004E2500.
    GaugeClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight) noexcept
        : ControlClass(noinit_t()) { JMP_THIS(0x4E2500); }

protected:
    explicit __forceinline GaugeClass(noinit_t)  noexcept
        : ControlClass(noinit_t())
    {
    }

    // Properties
public:

    bool IsColorized;
    bool HasThumb;
    bool IsHorizontal;
    int MaxValue;
    int CurrentValue;
    int ClickDiff;
};
