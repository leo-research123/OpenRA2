#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/YRPPCore.h"

class LinkClass
{
public:
    // Destructor
    /// VA: 0x005565A0.
    virtual ~LinkClass() ;

    // LinkClass
    /// VA: 0x00556620.
    virtual LinkClass* GetNext() ;
    /// VA: 0x00556630.
    virtual LinkClass* GetPrev() ;
    /// VA: 0x005566A0.
    virtual LinkClass* Add(LinkClass& another) ;
    /// VA: 0x00556700.
    virtual LinkClass* AddTail(LinkClass& another) ;
    /// VA: 0x005566D0.
    virtual LinkClass* AddHead(LinkClass& another) ;
    /// VA: 0x00556640.
    virtual LinkClass* HeadOfList() ;
    /// VA: 0x00556670.
    virtual LinkClass* TailOfList() ;
    /// VA: 0x005565F0.
    virtual void Zap() ;
    /// VA: 0x00556730.
    virtual LinkClass* Remove() ;

    // Non virtual
    /// VA: 0x00556600.
    LinkClass& operator=(LinkClass& another) ;

    // Constructors
    /// VA: 0x00556550.
    LinkClass() noexcept;
    LinkClass(LinkClass& another);

protected:
    explicit __forceinline LinkClass(noinit_t)  noexcept
    {
    }

    // Properties
public:

    LinkClass* Next;
    LinkClass* Previous;
};
