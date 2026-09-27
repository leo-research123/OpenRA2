#pragma once

#include "yrpp/platform/ABI.h"
#ifndef _MSC_VER
#define __forceinline inline
#define __declspec(attribute)
// Preserve source compatibility for existing consumers of the old spelling.
#define __stdcall YRPP_STDCALL
#define __fastcall YRPP_FASTCALL
#define __cdecl YRPP_CDECL
#endif

// the most basic globals
#include "yrpp/Fundamentals.h"

// Syringe interaction header - also includes <windows.h>
#ifdef _WIN32
#include "yrpp/Syringe.h"
#endif

// Assembly macros
#include "yrpp/ASMMacros.h"

#include "yrpp/Memory.h"

#include <wchar.h>
#include <cstdio>

#include "yrpp/Helpers/EnumFlags.h"

// Avoid default CTOR trick
#define DECLARE_PROPERTY(type,name)\
union{\
    type name; \
    char __##name[sizeof(type)]; \
}

#define DECLARE_PROPERTY_ARRAY(type,name,cnt)\
union{\
    type name[cnt]; \
    char __##name[sizeof(type) * cnt]; \
}

// Not gettable/settable members
#define PROTECTED_PROPERTY(type,name)\
    protected:\
        type name; \
    public:

/*
Legacy declaration bodies

These two replace a function's implementation.

R0 is used for functions which return a numeric value or a pointer.
RX is for void functions and destructors only.
Functions that return struct instances will have to be written manually.

Do not use RX for a value-returning function.

Example usage:
virtual int foo(int bar) R0;
virtual void alla(double malla) RX;
*/

#define R0 {return 0;}
#define RX {}
#define RT(type) {return type();}

#define NOVTABLE __declspec(novtable)

// noinit_t is declared by Memory.h -> platform/ABI.h.
