// Theater-art loop adapted from EA REDALERT/SDATA.CPP at
// f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae, SmudgeTypeClass::Init.
//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection
// License copy: third_party/ea/LICENSE.TXT.
// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/SmudgeTypeClass.h"
#include <cstring>

HRESULT YRPP_STDCALL SmudgeTypeClass::GetClassID(CLSID* dest) {
    // Original 0x6b58d0: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x5af2ce78u, 0x11d20634u, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType SmudgeTypeClass::WhatAmI() const { return AbsID; } // 0x6b6130
int SmudgeTypeClass::Size() const { return sizeof(*this); } // 0x6b6140
static_assert(static_cast<int>(SmudgeTypeClass::AbsID) == 30);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(SmudgeTypeClass) == 0x2a4);
#endif

int SmudgeTypeClass::GetArrayIndex() const { return ArrayIndex; }

#include "yrpp/CCFileClass.h"
#include <cstdio>

namespace { DynamicVectorClass<SmudgeTypeClass*> smudge_types; }
DynamicVectorClass<SmudgeTypeClass*>& SmudgeTypeClass::Array = smudge_types;

SmudgeTypeClass* YRPP_FASTCALL SmudgeTypeClass::Find(const char* id) {
    const int index = FindIndex(id);
    return index < 0 ? nullptr : Array[index];
}
int YRPP_FASTCALL SmudgeTypeClass::FindIndex(const char* id) {
    if (!id) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (_strcmpi(Array[i]->ID, id) == 0) return i;
    return -1;
}
void YRPP_FASTCALL SmudgeTypeClass::LoadFromIniList(int theater) {
    // 6B5490 uses ID (not ImageFile), only processes Theater=true, and leaves
    // ImageAllocated unchanged. Callers retain ownership of overwritten images.
    for (int i = 0; i < Array.Count; ++i) {
        auto* type = Array[i];
        if (!type->Theater) continue;
        char filename[512];
        std::snprintf(filename, sizeof(filename), "%s.%s", type->ID,
            ::Theater::Array[theater].Extension);
        CCFileClass file(filename);
        type->Image = static_cast<SHPStruct*>(file.ReadWholeFile());
    }
}

// Supplied 6B5260; values mapped against existing x86 member offsets.
SmudgeTypeClass::SmudgeTypeClass(const char* id) noexcept : ObjectTypeClass(id), ArrayIndex(-1), Width(1), Height(1), Crater(false), Burn(false) {
    Create_ID();
    RadarInvisible = true; Selectable = false; LegalTarget = false;
    Insignificant = true; Immune = true; AllowCellContent = false;
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}
// 6B6160 contains the destructor body plus scalar-delete epilogue. The C++
// destructor implements only the body; the caller retains deletion ownership.
SmudgeTypeClass::~SmudgeTypeClass() {
    YRMemory::Deallocate(Image); Image = nullptr;
    NotifyTypeExpired();
    Array.Remove(this);
}

#include "yrpp/CRC.h"
void SmudgeTypeClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc);
    crc(ArrayIndex);
    crc(Crater);
    crc(Burn);
    crc(Width);
    crc(Height);
}

#include <new>
// Supplied 006B5910; independent native allocation size, same lookup/sentinel order.
SmudgeTypeClass* YRPP_FASTCALL SmudgeTypeClass::FindOrAllocate(const char* id) {
    if (!id || _strcmpi(id, "none") == 0 || _strcmpi(id, "<none>") == 0) return nullptr;
    if (auto* existing = Find(id)) return existing;
    void* storage = YRMemory::Allocate(sizeof(SmudgeTypeClass));
    return storage ? new (storage) SmudgeTypeClass(id) : nullptr;
}
