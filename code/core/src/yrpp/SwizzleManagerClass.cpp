// Original pointer relocation service, YR 0x6CF180..0x6CF4E0.
#include "yrpp/SwizzleManagerClass.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>

SwizzleManagerClass::SwizzleManagerClass() noexcept {
    Swizzles_Old.CapacityIncrement = Swizzles_New.CapacityIncrement = 1000;
}
SwizzleManagerClass::~SwizzleManagerClass() = default;
ULONG YRPP_STDCALL SwizzleManagerClass::AddRef() { return 1; }
ULONG YRPP_STDCALL SwizzleManagerClass::Release() { return 1; }
HRESULT YRPP_STDCALL SwizzleManagerClass::QueryInterface(REFIID iid, void** output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    constexpr GUID unknown{0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    constexpr GUID swizzle{0x5FF0CA70, 0x8B12, 0x11D1, {0xB7, 0x08, 0, 0xA0, 0x24, 0xDD, 0xAF, 0xD1}};
    if (std::memcmp(&iid, &unknown, sizeof(iid)) && std::memcmp(&iid, &swizzle, sizeof(iid)))
        return static_cast<HRESULT>(0x80004002u);
    *output = this;
    AddRef();
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Fetch_Swizzle_ID(void* pointer, LONG* id) const {
    if (!pointer || !id) return static_cast<HRESULT>(0x80004003u);
    *id = static_cast<LONG>(reinterpret_cast<std::uintptr_t>(pointer));
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Swizzle(void** pointer) {
    if (!pointer) return static_cast<HRESULT>(0x80004003u);
    if (!*pointer) return 0;
    SwizzlePointerClass entry;
    entry.unknown_0 = static_cast<LONG>(reinterpret_cast<std::uintptr_t>(*pointer));
    entry.pAnything = pointer;
    try {
        Swizzles_Old.AddItem(entry);
    } catch (const std::bad_alloc&) {
        // Original vector growth failure still clears the source pointer.
    } catch (...) {
        return static_cast<HRESULT>(0x80004005u);
    }
    *pointer = nullptr;
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Here_I_Am(LONG id, void* pointer) {
    SwizzlePointerClass entry;
    entry.unknown_0 = id;
    entry.pAnything = pointer;
    try {
        Swizzles_New.AddItem(entry);
    } catch (const std::bad_alloc&) {
    } catch (...) {
        return static_cast<HRESULT>(0x80004005u);
    }
    return 0;
}
HRESULT YRPP_STDCALL SwizzleManagerClass::Reset() {
    if (!Swizzles_Old.Count) return 0;
    const auto order = [](const SwizzlePointerClass& a, const SwizzlePointerClass& b) {
        return a.unknown_0 < b.unknown_0;
    };
    if (Swizzles_New.Count > 1) std::sort(Swizzles_New.Items, Swizzles_New.Items + Swizzles_New.Count, order);
    if (Swizzles_Old.Count > 1) std::sort(Swizzles_Old.Items, Swizzles_Old.Items + Swizzles_Old.Count, order);
    int index = 0;
    for (const auto& request : Swizzles_Old) {
        while (index < Swizzles_New.Count && Swizzles_New[index].unknown_0 < request.unknown_0) ++index;
        // Malformed native streams must not make the original unbounded search
        // overrun its relocation vector. Retain the requests on failure.
        if (index == Swizzles_New.Count || Swizzles_New[index].unknown_0 != request.unknown_0)
            return static_cast<HRESULT>(0x80004005u);
        *static_cast<void**>(request.pAnything) = Swizzles_New[index].pAnything;
    }
    Swizzles_Old.Clear();
    Swizzles_New.Clear();
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
