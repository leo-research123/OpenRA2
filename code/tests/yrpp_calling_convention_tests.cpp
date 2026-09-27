#include "support/test_support.hpp"
// Check ABI macro spelling against independent compiler-native types on x86.
#include "yrpp/platform/ABI.h"
#include "yrpp/platform/ABI.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include "yrpp/Memory.h"
#include <type_traits>

namespace {
int YRPP_STDCALL stdcall_probe(int a, int b) { return a + b; }
int YRPP_FASTCALL fastcall_probe(int a, int b) { return a - b; }
int YRPP_CDECL cdecl_probe(int a, int b) { return a * b; }
struct MemberProbe {
    int value;
    int YRPP_THISCALL add(int other) const { return value + other; }
};

#if defined(_MSC_VER) && defined(_M_IX86)
using NativeStdcall = int (__stdcall*)(int, int);
using NativeFastcall = int (__fastcall*)(int, int);
using NativeCdecl = int (__cdecl*)(int, int);
using NativeThiscall = int (__thiscall MemberProbe::*)(int) const;
#elif defined(__i386__)
using NativeStdcall = int (__attribute__((stdcall)) *)(int, int);
using NativeFastcall = int (__attribute__((fastcall)) *)(int, int);
using NativeCdecl = int (__attribute__((cdecl)) *)(int, int);
using NativeThiscall = int (__attribute__((thiscall)) MemberProbe::*)(int) const;
#else
using NativeStdcall = int (*)(int, int);
using NativeFastcall = int (*)(int, int);
using NativeCdecl = int (*)(int, int);
using NativeThiscall = int (MemberProbe::*)(int) const;
#endif

static_assert(std::is_same_v<decltype(&stdcall_probe), NativeStdcall>);
static_assert(std::is_same_v<decltype(&fastcall_probe), NativeFastcall>);
static_assert(std::is_same_v<decltype(&cdecl_probe), NativeCdecl>);
static_assert(std::is_same_v<decltype(&MemberProbe::add), NativeThiscall>);
using AlphaAcquire = AlphaLightingRemapClass* (YRPP_STDCALL*)(int);
using AlphaRelease = void (YRPP_STDCALL*)(AlphaLightingRemapClass*);
using Allocate = void* (YRPP_CDECL*)(std::size_t);
static_assert(std::is_same_v<decltype(&AlphaLightingRemapClass::FindOrAllocate), AlphaAcquire>);
static_assert(std::is_same_v<decltype(&AlphaLightingRemapClass::Release), AlphaRelease>);
static_assert(std::is_same_v<decltype(&YRMemory::Allocate), Allocate>);
}


TEST(YrppCallingConvention, NativeFunctionPointers) {
    const NativeStdcall stdcall_call = &stdcall_probe;
    const NativeFastcall fastcall_call = &fastcall_probe;
    const NativeCdecl cdecl_call = &cdecl_probe;
    const NativeThiscall member_call = &MemberProbe::add;
    const MemberProbe member{7};
    for (int i = 0; i != 1024; ++i) {
        EXPECT_FALSE((stdcall_call(i, 3) != i + 3 || fastcall_call(i, 3) != i - 3 ||
            cdecl_call(i, 3) != i * 3 || (member.*member_call)(i) != i + 7));
    }
}
