#pragma once

#include "yrpp/GaugeClass.h"

class NOVTABLE TriColorGaugeClass : public GaugeClass
{
public:
    // TriColorGaugeClass
    /// VA: unknown (legacy placeholder).
    virtual bool SetRedLimit(int value) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool SetYellowLimit(int value) R0;

    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x004E2A50.
    TriColorGaugeClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight) noexcept
        : GaugeClass(noinit_t()) { JMP_THIS(0x4E2A50); }

protected:
    explicit __forceinline TriColorGaugeClass(noinit_t)  noexcept
        : GaugeClass(noinit_t())
    {
    }

    // Properties
public:

    int RedLimit;
    int YellowLimit;
};
