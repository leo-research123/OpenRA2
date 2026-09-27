#pragma once

#include "yrpp/ControlClass.h"

class NOVTABLE ToggleClass : public ControlClass
{
public:
    bool Action(GadgetFlag,DWORD*,KeyModifier) override;
    // Non virtual
    /// VA: 0x00723EA0.
    void TurnOn();
    /// VA: 0x00723EB0.
    void TurnOff();

    // Statics

    // Constructors
    /// VA: 0x00723E60.
    ToggleClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight) noexcept
        ;

protected:
    explicit __forceinline ToggleClass(noinit_t)  noexcept
        : ControlClass(noinit_t())
    { }

    // Properties
public:

    bool IsPressed;
    bool IsOn;
    DWORD ToggleType;
};
