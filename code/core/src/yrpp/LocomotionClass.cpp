// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 loco.cpp, with YR COM identities and reference ownership restored.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/LocomotionClass.h"
#include "yrpp/Unsorted.h"
#include <atomic>
#include <cstring>
#include <bit>
#include "yrpp/Drawing.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"

Point2D YRPP_STDCALL LocomotionClass::Shadow_Point() {
    return {0, TacticalClass::AdjustForZ(LinkedTo->GetHeight())};
}

Matrix3D YRPP_STDCALL LocomotionClass::Shadow_Matrix(VoxelIndexKey* key) {
    const int ramp=LinkedTo->GetCell()->SlopeIndex;
    if(key&&key->Is_Valid_Key())key->Value=std::bit_cast<int>((unsigned(key->Value)<<6)+unsigned(ramp));
    auto matrix=Matrix3D::VoxelRampMatrix[ramp];
    const auto facing=LinkedTo->PrimaryFacing.Current().GetValue<5>();
    matrix.RotateZ(float((int(facing)-8)*-0.1963495408493621));
    if(key&&key->Is_Valid_Key())key->Value=std::bit_cast<int>((unsigned(key->Value)<<5)|unsigned(facing));
    return matrix;
}

Matrix3D YRPP_STDCALL LocomotionClass::Draw_Matrix(VoxelIndexKey* key) {
    auto matrix=Matrix3D::GetIdentity();
    const auto facing=LinkedTo->PrimaryFacing.Current().GetValue<5>();
    matrix.RotateZ(float((int(facing)-8)*-0.1963495408493621));
    if(key){unsigned raw;std::memcpy(&raw,key,sizeof(raw));
        if(raw!=0xFFFFFFFFu){raw=raw*32u|unsigned(LinkedTo->PrimaryFacing.Current().GetValue<5>());std::memcpy(key,&raw,sizeof(raw));}}
    return matrix;
}

HRESULT YRPP_STDCALL LocomotionClass::QueryInterface(REFIID iid, void** output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    *output = nullptr;
    constexpr GUID unknown{0,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    constexpr GUID persist{0x109,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    constexpr GUID stream{0x10C,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    constexpr GUID locomotion{0x070F3290,0x9841,0x11D1,{0xB7,0x09,0,0xA0,0x24,0xDD,0xAF,0xD1}};
    // IUnknown is the ILocomotion subobject, NOT the primary IPersistStream.
    if (!std::memcmp(&iid,&unknown,sizeof(iid)) || !std::memcmp(&iid,&locomotion,sizeof(iid)))
        *output = static_cast<ILocomotion*>(this);
    if (!std::memcmp(&iid,&persist,sizeof(iid)) || !std::memcmp(&iid,&stream,sizeof(iid)))
        *output = static_cast<IPersistStream*>(this);
    if (!*output) return static_cast<HRESULT>(0x80004002u);
    try { AddRef(); return 0; }
    catch (...) { *output = nullptr; return static_cast<HRESULT>(0x80004005u); }
}

ULONG YRPP_STDCALL LocomotionClass::AddRef() {
    ++std::atomic_ref<LONG>(Game::COMReferenceCount);
    return static_cast<ULONG>(++std::atomic_ref<int>(RefCount));
}

ULONG YRPP_STDCALL LocomotionClass::Release() {
    --std::atomic_ref<LONG>(Game::COMReferenceCount);
    const int remaining = --std::atomic_ref<int>(RefCount);
    // COM instances use the original allocator, as do the other original classes.
    if (!remaining) GameDelete(this);
    return static_cast<ULONG>(remaining);
}

HRESULT YRPP_STDCALL LocomotionClass::GetSizeMax(ULARGE_INTEGER* size) {
    if (!size) return static_cast<HRESULT>(0x80004003u);
    try { size->QuadPart = static_cast<DWORD>(Size()) + DWORD{4}; return 0; }
    catch (...) { return static_cast<HRESULT>(0x80004005u); }
}
