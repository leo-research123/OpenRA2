// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors
// Based on OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 code/swizzle.cpp.
// Modified for RedAlert2Open: restore YR COM, vectors and pointer-ID contracts.
// See third_party/opents/LICENSE.md for additional terms and warranty disclaimers.
#include "yrpp/SwizzleManagerClass.h"
#include <bit>
#include <cstring>
#include <limits>
#include <utility>

SwizzleManagerClass::SwizzleManagerClass() noexcept {
    // The target sets growth, but does not allocate/reserve at construction.
    Swizzles_Old.CapacityIncrement = Swizzles_New.CapacityIncrement = 1000;
}
SwizzleManagerClass::~SwizzleManagerClass() { SwizzleManagerClass::Reset(); }
ULONG YRPP_STDCALL SwizzleManagerClass::AddRef() { return 1; }
ULONG YRPP_STDCALL SwizzleManagerClass::Release() { return 1; }
HRESULT YRPP_STDCALL SwizzleManagerClass::QueryInterface(REFIID iid, void** output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    constexpr GUID unknown{0,0,0,{0xC0,0,0,0,0,0,0,0x46}};
    constexpr GUID swizzle{0x5FF0CA70,0x8B12,0x11D1,{0xB7,0x08,0,0xA0,0x24,0xDD,0xAF,0xD1}};
    // Unlike AbstractClass, an unsupported IID leaves *output unchanged.
    if (std::memcmp(&iid,&unknown,sizeof(iid)) && std::memcmp(&iid,&swizzle,sizeof(iid)))
        return static_cast<HRESULT>(0x80004002u);
    *output = static_cast<ISwizzle*>(this);
    try { AddRef(); return 0; }
    catch (...) { return static_cast<HRESULT>(0x80004005u); }
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Fetch_Swizzle_ID(void* pointer, LONG* id) const {
    if (!pointer || !id) return static_cast<HRESULT>(0x80004003u);
    const auto value = reinterpret_cast<std::uintptr_t>(pointer);
    // A 64-bit native address is not a 32-bit original save identity.
    if (value > std::numeric_limits<DWORD>::max()) return static_cast<HRESULT>(0x80070057u);
    *id = std::bit_cast<LONG>(static_cast<DWORD>(value));
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Swizzle(void** pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (!*pointer) return 0;
    LONG id = 0;
    HRESULT hr;
    try { hr = Fetch_Swizzle_ID(*pointer,&id); }
    catch (...) { return static_cast<HRESULT>(0x80004005u); }
    if (hr < 0) return hr;
    SwizzlePointerClass entry; entry.unknown_0 = id; entry.pAnything = pointer;
    // 0x6CF240 clears the slot and returns S_OK even if vector growth fails.
    try { Swizzles_Old.AddItem(entry); }
    catch (...) { Swizzles_Old.IsInitialized = true; }
    *pointer = nullptr;
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Here_I_Am(LONG id, void* pointer) {
    SwizzlePointerClass entry; entry.unknown_0 = id; entry.pAnything = pointer;
    try { Swizzles_New.AddItem(entry); }
    catch (...) { Swizzles_New.IsInitialized = true; }
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Reset() {
    if (!Swizzles_Old.Count) return 0; // Original retains announcements in this case.
    // Match the target CRT's equal-key ordering, as calibrated for IndexClass.
    auto sort = [](auto&& sort, SwizzlePointerClass* items, int low, int high) -> void {
        while (low < high) {
            if (high-low+1 <= 8) {
                for (int end=high; end>low; --end) {
                    int maximum=low;
                    for (int i=low+1; i<=end; ++i)
                        if (items[maximum].unknown_0 < items[i].unknown_0) maximum=i;
                    std::swap(items[maximum],items[end]);
                }
                return;
            }
            std::swap(items[low+(high-low+1)/2],items[low]);
            int left=low, right=high+1;
            for (;;) {
                do { ++left; } while (left<=high && items[low].unknown_0>=items[left].unknown_0);
                do { --right; } while (right>low && items[right].unknown_0>=items[low].unknown_0);
                if (right<left) break;
                std::swap(items[left],items[right]);
            }
            std::swap(items[low],items[right]);
            if (right-low >= high-left+1) {
                if (left<high) sort(sort,items,left,high);
                high=right-1;
            } else {
                if (low+1<right) sort(sort,items,low,right-1);
                low=left;
            }
        }
    };
    sort(sort,Swizzles_New.Items,0,Swizzles_New.Count-1);
    sort(sort,Swizzles_Old.Items,0,Swizzles_Old.Count-1);
    // The original deliberately divides by zero on a mismatched ID and may read
    // beyond announcements. Diagnose without writing a
    // partial pointer graph; preserve requests so a caller can announce and retry.
    int announced=0;
    for (const auto& request : Swizzles_Old) {
        while (announced<Swizzles_New.Count && Swizzles_New[announced].unknown_0<request.unknown_0) ++announced;
        if (announced==Swizzles_New.Count || Swizzles_New[announced].unknown_0!=request.unknown_0 ||
            !request.pAnything) return static_cast<HRESULT>(0x80004005u);
    }
    announced=0;
    for (const auto& request : Swizzles_Old) {
        while (Swizzles_New[announced].unknown_0<request.unknown_0) ++announced;
        *static_cast<void**>(request.pAnything) = Swizzles_New[announced].pAnything;
    }
    for (auto* table : {&Swizzles_Old,&Swizzles_New}) {
        if (table->IsAllocated) table->Clear();
        else { table->Count=0; table->Capacity=0; } // Borrowed Items survive original Clear.
    }
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Save_Interface(IStream*, IUnknown*) {
    return static_cast<HRESULT>(0x80004001u);
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Load_Interface(IStream*, GUID*, void**) {
    return static_cast<HRESULT>(0x80004001u);
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Get_Save_Size(int* size) const {
    if (!size) return static_cast<HRESULT>(0x80004003u);
    *size = 4;
    return 0;
}
