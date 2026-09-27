// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tube.cpp constructor/destructor; YR 0x727FD0/0x728710.
// Copyright 2026 OpenTS contributors. EA terms: third_party/opents/LICENSE.md.
#include "yrpp/TubeClass.h"
#include "yrpp/MapClass.h"
#include <cstring>
#include <cstdlib>

namespace { DynamicVectorClass<TubeClass*> tubes; }
DynamicVectorClass<TubeClass*>& TubeClass::Array = tubes;

TubeClass::TubeClass(CellStruct* cell, int facing) noexcept
    : EnterCell(*cell), ExitCell(*cell), ExitFace(facing), Faces{}, FaceCount(0) {
    Create_ID();
    for (auto& direction : Faces) direction = -1;
    if (!Array.AddItem(this)) std::abort();
    if (cell->X || cell->Y) MapClass::Instance.GetCellAt(*cell)->TubeIndex = static_cast<short>(Array.FindItemIndex(this));
}

TubeClass::~TubeClass() {
    // Tube has no Object/Type flags or dedicated list in 0x7258D0. Full global
    // UI-pointer invalidation remains part of the AnnounceExpiredPointer work.
    auto* cell = MapClass::Instance.GetCellAt(EnterCell);
    if (cell && cell->TubeIndex == Array.FindItemIndex(this)) cell->TubeIndex = -1;
    Array.Remove(this); // Original renumbering is separate (Assign_Tubes).
}

HRESULT YRPP_STDCALL TubeClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[]{0x0B4CA41C,0x11D1B3A7,0x600057B4,0x79A9C697};
    std::memcpy(id,words,sizeof(words));return 0;
}
