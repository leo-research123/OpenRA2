#pragma once

#include "yrpp/ControlClass.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/ShapeButtonClass.h"
#include "yrpp/SliderClass.h"

class NOVTABLE ListClass : public ControlClass
{
public:
    // ListClass
    /// VA: unknown (legacy placeholder).
    virtual int AddItem(const char* lpStr) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool EnableScrollBar() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool Bump(bool bMinus) R0;
    /// VA: unknown (legacy placeholder).
    virtual const int GetCount() R0;
    /// VA: unknown (legacy placeholder).
    virtual const int GetCurrentIndex() R0;
    /// VA: unknown (legacy placeholder).
    virtual const char* GetCurrentItem() R0;
    /// VA: unknown (legacy placeholder).
    virtual const char* GetItem(int index) R0;
    /// VA: unknown (legacy placeholder).
    virtual int StepSelectedIndex(int step) R0; // huh?
    /// VA: unknown (legacy placeholder).
    virtual void RemoveItem(const char* lpStr) RX;
    /// VA: unknown (legacy placeholder).
    virtual void RemoveItemAt(int index) RX;
    /// VA: unknown (legacy placeholder).
    virtual bool DisableScrollBar() R0;
    /// VA: unknown (legacy placeholder).
    virtual void SetSelectedIndex(int index) RX;
    /// VA: unknown (legacy placeholder).
    virtual void SetSelectedItem(const char* lpStr) RX;
    /// VA: unknown (legacy placeholder).
    virtual void SetTabs(void* pTabs) RX;
    /// VA: unknown (legacy placeholder).
    virtual bool SetViewIndex(int index) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool Step(bool bMinus) R0;
    /// VA: unknown (legacy placeholder).
    virtual void DrawEntry(int index, int nX, int nY, int nWidth, bool bUnk) RX; // bUnk : Maybe fill background?

    // Non virtual

    // Statics

    // Constructors
    /// VA: 0x00557230.
    ListClass(unsigned int nID, int nX, int nY, int nWidth, int nHeight, TextPrintType eFlag, SHPStruct* UpSHP, SHPStruct* DownSHP) noexcept
        : ListClass(noinit_t()) { JMP_THIS(0x557230); }

    explicit __forceinline ListClass(noinit_t)  noexcept
        : ControlClass(noinit_t())
        , Scroller(noinit_t())
    {
    }

    // Properties
public:

    TextPrintType TextFlags;
    void* Tabs; // Not sure what it is
    DynamicVectorClass<const char*> List;
    int LineHeight;
    int LineCount;
    bool IsScrollActive;
    ShapeButtonClass UpButton;
    ShapeButtonClass DownButton;
    SliderClass Scroller;
    int SelectedIndex;
    int CurrentTopIndex;
};
