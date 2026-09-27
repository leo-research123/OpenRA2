#pragma once

#include "yrpp/GadgetClass.h"

class NOVTABLE ControlClass : public GadgetClass
{
public:
    const unsigned int GetID() override;
    bool Action(GadgetFlag,DWORD*,KeyModifier) override;
    bool Draw(bool forced) override;

    // Virtual
    /// VA: unknown (legacy placeholder).
    virtual void MakePeer(GadgetClass* pGadget);

    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x0048E520.
    ControlClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight, GadgetFlag eFlag, bool bSticky) noexcept
        ;

    /// VA: 0x0048E570.
    ControlClass(ControlClass& another) noexcept
        : ControlClass(noinit_t()) { JMP_THIS(0x48E570); }

protected:
    explicit __forceinline ControlClass(noinit_t) noexcept
        : GadgetClass(noinit_t())
    { }

    // Properties
public:

    int ID;
    GadgetClass* SendTo; // Peer
};
