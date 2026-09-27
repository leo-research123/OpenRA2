#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ASMMacros.h"
#include "yrpp/YRPPCore.h"
#include "yrpp/YRMathVector.h"
#include "yrpp/BasicStructures.h"

#include <cstring>
#ifndef _WIN32
#include <strings.h>
#define _strcmpi strcasecmp
#endif
struct DirStruct;

// used for cell coordinates/vectors
using CellStruct = Vector2D<short>;
using Point2D = Vector2D<int>;
using CoordStruct = Vector3D<int>;

struct BasePlanningCell {
    int Weight;
    CellStruct Position;
};

// this crap is used in several Base planning routines
struct BasePlanningCellContainer {
    BasePlanningCell * Items;
    int Count;
    int Capacity;
    bool Sorted;
    DWORD Unknown_10;

    /// VA: 0x00510860.
    bool AddCapacity(int AdditionalCapacity)
        { JMP_THIS(0x510860); }

    // for qsort
    /// VA: 0x005108F0.
    static int YRPP_CDECL Comparator(const void *, const void *)
        { JMP_STD(0x5108F0); }
};
// element type of the game's name/value tables, e.g. the pip table at 0x81B958
struct NamedValue {
    const char* Name;
    int Value;

    bool operator == (int value) const {
        return this->Value == value;
    }

    bool operator == (const char* name) const {
        return !_strcmpi(this->Name, name);
    }

    bool operator == (const NamedValue& other) const {
        return this->Value == other.Value && *this == other.Name;
    }
};
