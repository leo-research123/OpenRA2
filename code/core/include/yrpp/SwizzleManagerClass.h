#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/YRPPCore.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/Interfaces.h"

#include "yrpp/Helpers/CompileTime.h"

class SwizzlePointerClass
{
    friend class SwizzleManagerClass;
    LONG unknown_0 = 0; // Original saved pointer identity, signed 32-bit sort key.
    void* pAnything = nullptr; // Request destination or announced new address.

public:
    bool operator==(const SwizzlePointerClass& tOther) const
    {
        return unknown_0 == tOther.unknown_0;
    }
};

class NOVTABLE SwizzleManagerClass : public ISwizzle
{
public:
    /// Global VA: 0x00B0C110.
    static SwizzleManagerClass& Instance;

    // IUnknown
    /// VA: 0x006CF430
    HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) override;

    /// VA: 0x006CF4B0
    ULONG YRPP_STDCALL AddRef() override;
    /// VA: 0x006CF4C0
    ULONG YRPP_STDCALL Release() override;

    // ISwizzle
    /// VA: 0x006CF230
    HRESULT YRPP_STDCALL Reset() override;

    /// VA: 0x006CF240
    HRESULT YRPP_STDCALL Swizzle(void** pointer) override;

    /// VA: 0x006CF490
    HRESULT YRPP_STDCALL Fetch_Swizzle_ID(void* pointer, LONG* id) const override;

    /// VA: 0x006CF2C0
    HRESULT YRPP_STDCALL Here_I_Am(LONG id, void* pointer) override;

    /// VA: 0x006CF4D0
    HRESULT YRPP_STDCALL Save_Interface(IStream* stream, IUnknown* pointer) override;
    /// VA: 0x006CF4E0
    HRESULT YRPP_STDCALL Load_Interface(IStream* stream, GUID* riid, void** pointer) override;

    /// VA: 0x006CF410
    HRESULT YRPP_STDCALL Get_Save_Size(int* psize) const override;

    // DTOR
    // Not virtual: the original table ends at ISwizzle::Get_Save_Size.
    /// VA: 0x006CF1D0
    ~SwizzleManagerClass();

    // CTOR
    /// VA: 0x006CF180
    SwizzleManagerClass() noexcept;

protected:
    explicit __forceinline SwizzleManagerClass(noinit_t)
    { }

public:

    DynamicVectorClass<SwizzlePointerClass> Swizzles_Old;
    DynamicVectorClass<SwizzlePointerClass> Swizzles_New;

};
