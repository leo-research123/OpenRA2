#pragma once

#include "yrpp/EditClass.h"
#include "yrpp/ListClass.h"

class NOVTABLE DropListClass : public EditClass
{
public:
    // DropListClass
    /// VA: unknown (legacy placeholder).
    virtual int AddItem(const wchar_t* lpStr) R0;
    /// VA: unknown (legacy placeholder).
    virtual wchar_t* CurrentItem() R0;
    /// VA: unknown (legacy placeholder).
    virtual int CurrentIndex() R0;
    /// VA: unknown (legacy placeholder).
    virtual void SetSelectedItem(const wchar_t* lpStr) RX;
    /// VA: unknown (legacy placeholder).
    virtual void SetSelectedIndex(int index) RX;
    /// VA: unknown (legacy placeholder).
    virtual int GetCount() R0;
    /// VA: unknown (legacy placeholder).
    virtual wchar_t* GetItem(int index) R0;

    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x004B4E10.
    DropListClass(unsigned int nID, wchar_t* pText, int nMaxLength, TextPrintType eTextFlag, int nX, int nY,
        int nWidth, int nHeight, EditFlag eEditFlag,int nSomeHeight, SHPStruct* UpSHP, SHPStruct* DownSHP) noexcept
        : DropListClass(noinit_t()) { JMP_THIS(0x4B4E10); }

protected:
    explicit __forceinline DropListClass(noinit_t)  noexcept
        : EditClass(noinit_t())
        , List(noinit_t())
    {
    }

    // Properties
public:

    bool bool_48;
    int SomeHeight;
    ShapeButtonClass SomeButton;
    ListClass List;
};
