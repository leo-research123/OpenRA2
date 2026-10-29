// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tube.cpp constructor/destructor; YR 0x727FD0/0x728710.
// Copyright 2026 OpenTS contributors. EA terms: third_party/opents/LICENSE.md.
#include "yrpp/TubeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CCINIClass.h"
#include <cstring>
#include <cstdlib>

namespace {
DynamicVectorClass<TubeClass*> tubes;
}
DynamicVectorClass<TubeClass*>& TubeClass::Array = tubes;

TubeClass::TubeClass(CellStruct* cell, int facing) noexcept
    : EnterCell(*cell), ExitCell(*cell), ExitFace(facing), Faces{}, FaceCount(0) {
    Create_ID();
    for (auto& direction : Faces)
        direction = -1;
    if (!Array.AddItem(this)) std::abort();
    if (cell->X || cell->Y)
        MapClass::Instance.GetCellAt(*cell)->TubeIndex = static_cast<short>(Array.FindItemIndex(this));
}

TubeClass::~TubeClass() {
    // Tube has no Object/Type flags or dedicated list in 0x7258D0. Full global
    // UI-pointer invalidation remains part of the AnnounceExpiredPointer work.
    auto* cell = MapClass::Instance.GetCellAt(EnterCell);
    if (cell && cell->TubeIndex == Array.FindItemIndex(this)) cell->TubeIndex = -1;
    Array.Remove(this);  // Original renumbering is separate (Assign_Tubes).
}

HRESULT YRPP_STDCALL TubeClass::GetClassID(CLSID* id) {
    if (!id) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[]{0x0B4CA41C, 0x11D1B3A7, 0x600057B4, 0x79A9C697};
    std::memcpy(id, words, sizeof(words));
    return 0;
}

#if !defined(RA2_YRPP_GAME)
// YR 0x7283C0. The original passes every field to atoi unchecked and faults on
// a short record; a missing coordinate reads as zero and a missing face ends
// the list, as an explicit -1 would.
void YRPP_FASTCALL TubeClass::ReadINI(CCINIClass* ini) {
    ini->Reset();
    const int count = ini->GetKeyCount("Tubes");
    for (int i = 0; i < count; ++i) {
        char text[512];
        ini->ReadString("Tubes", ini->GetKeyName("Tubes", i), nullptr, text, sizeof(text));
        CellStruct origin{0, 0};
        auto* tube = GameCreate<TubeClass>(&origin, 0);
        const auto number = [](const char* field) { return field ? std::atoi(field) : 0; };
        tube->EnterCell.X = short(number(std::strtok(text, ",")));
        tube->EnterCell.Y = short(number(std::strtok(nullptr, ",")));
        tube->ExitFace = number(std::strtok(nullptr, ","));
        tube->ExitCell.X = short(number(std::strtok(nullptr, ",")));
        tube->ExitCell.Y = short(number(std::strtok(nullptr, ",")));
        // The count starts at -1 and also counts the -1 terminator.
        tube->FaceCount = -1;
        for (int face = 0; face < 100; ++face) {
            const char* field = std::strtok(nullptr, ",");
            tube->Faces[face] = field ? std::atoi(field) : -1;
            ++tube->FaceCount;
            if (tube->Faces[face] == -1) break;
        }
        // The cell keeps the INI row number, not the tube's array index.
        MapClass::Instance.GetCellAt(tube->EnterCell)->TubeIndex = static_cast<short>(i);
    }
}
#endif
