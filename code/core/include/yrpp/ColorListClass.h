#pragma once

#include "yrpp/ListClass.h"

class NOVTABLE ColorListClass : public ListClass
{
public:
    // ColorListClass
    /// VA: unknown (legacy placeholder).
    virtual int AddItem(const char* lpStr, int color) R0;
    /// VA: unknown (legacy placeholder).
    virtual void SetSelectedStyle(int style, int color) RX;

    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x00488760.
    ColorListClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight, TextPrintType eFlag, SHPStruct* UpSHP, SHPStruct* DownSHP) noexcept
        : ColorListClass(noinit_t()) { JMP_THIS(0x488760); }

protected:
    explicit __forceinline ColorListClass(noinit_t)  noexcept
        : ListClass(noinit_t())
    {
    }

    // Properties
public:

    DynamicVectorClass<DWORD> Colors;
    int Style; // ?
    int SelectColor; // ??
};
