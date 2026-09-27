#pragma once

#include "yrpp/GaugeClass.h"

class NOVTABLE SliderClass : public GaugeClass
{
public:
    // SliderClass
    /// VA: unknown (legacy placeholder).
    virtual int Bump(bool bMinus) R0; // CurrentValue +=/-= Thumb
    /// VA: unknown (legacy placeholder).
    virtual int Step(bool bMinus) R0; // CurrentValue +=/-= 1

    // Non virtual
    /// VA: 0x006B1EE0.
    void RecalculateThumb()
        JMP_THIS(0x6B1EE0);

    // Statics

    // Constructors
    /// VA: 0x006B1B20.
    SliderClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight, bool bBelongToList) noexcept
        : GaugeClass(noinit_t())
    {
        JMP_THIS(0x6B1B20);
    }

    /// VA: 0x005581A0.
    SliderClass(SliderClass& another) noexcept
        : GaugeClass(noinit_t())
    {
        JMP_THIS(0x5581A0);
    }

    explicit __forceinline SliderClass(noinit_t) noexcept // not protected for ListClass Constructor
        : GaugeClass(noinit_t())
    { }

    // Properties
public:
    GadgetClass* PlusGadget;
    GadgetClass* MinusGadget;
    bool BelongToList;
    int Thumb;
    int ThumbSize;
    int ThumbStart;
};
