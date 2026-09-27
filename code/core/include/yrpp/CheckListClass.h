#pragma once

#include "yrpp/ListClass.h"

class NOVTABLE CheckListClass : public ListClass
{
public:
    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x00488280.
    CheckListClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight, TextPrintType eFlag, SHPStruct* UpSHP, SHPStruct* DownSHP) noexcept
        : CheckListClass(noinit_t()) { JMP_THIS(0x488280); }

protected:
    explicit __forceinline CheckListClass(noinit_t)  noexcept
        : ListClass(noinit_t())
    {
    }

    // Properties
public:

    bool IsReadOnly;
};
