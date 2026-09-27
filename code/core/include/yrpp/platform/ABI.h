// Portable scalar types and calling conventions for YRpp declarations.
#pragma once
#if defined(RA2_FILES_GAME) || defined(RA2_IMAGE_GAME) || defined(RA2_YRPP_GAME)
// Required by game-facing builds, not by the portable core. A calling-convention
// attribute or -fms-extensions is NOT a substitute for the Microsoft C++ ABI.
#if !defined(_WIN32) || !defined(_MSC_VER) || !defined(_M_IX86) || defined(__MINGW32__)
#error Original-game C++ objects require Windows x86 Microsoft ABI. Use clang-cl --target=i686-pc-windows-msvc or cl / Win32.
#endif
static_assert(sizeof(void*) == 4 && sizeof(long) == 4 && sizeof(wchar_t) == 2,
    "Unexpected original-game scalar ABI");
#endif
#include <cstdint>
// YRpp also uses the lowercase RPC spelling without including RPC headers.
using byte = std::uint8_t;
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// Keep the YRpp method names independent of the Win32 A/W macros.
#undef CreateFile
#undef DeleteFile
#else
using BYTE = std::uint8_t;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using COLORREF = DWORD;
using BOOL = std::int32_t;
using UINT = std::uint32_t;
using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
using LRESULT = std::intptr_t;
inline constexpr int MAX_PATH = 260;
struct HWND__;
using HWND = HWND__*;
using HANDLE = void*;
// Win32 message ABI used by the original ToolTipManager. Native hosts deliver
// equivalent device/timer messages through the internal platform adapter.
struct tagMSG {
    HWND hwnd; UINT message; std::uintptr_t wParam; std::intptr_t lParam;
    DWORD time; struct { std::int32_t x, y; } pt;
};
using MSG = tagMSG;
struct HINSTANCE__;
using HINSTANCE = HINSTANCE__*;
struct HIMC__;
using HIMC = HIMC__*;
struct tagRECT;
using LPRECT = tagRECT*;
using RECT = tagRECT;
struct tagDRAWITEMSTRUCT;
using DRAWITEMSTRUCT = tagDRAWITEMSTRUCT;
union LARGE_INTEGER {
    struct { std::uint32_t LowPart; std::int32_t HighPart; };
    std::int64_t QuadPart;
};
#endif
#if defined(_MSC_VER) && defined(_M_IX86)
#define YRPP_FASTCALL __fastcall
#define YRPP_CDECL __cdecl
#define YRPP_STDCALL __stdcall
#define YRPP_THISCALL __thiscall
#elif defined(__i386__)
#define YRPP_FASTCALL __attribute__((fastcall))
#define YRPP_CDECL __attribute__((cdecl))
#define YRPP_STDCALL __attribute__((stdcall))
#define YRPP_THISCALL __attribute__((thiscall))
#else
#define YRPP_FASTCALL
#define YRPP_CDECL
#define YRPP_STDCALL
#define YRPP_THISCALL
#endif
#ifndef _WIN32
#define CALLBACK YRPP_STDCALL
using WNDPROC = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
#endif
static_assert(sizeof(DWORD) == 4 && sizeof(BOOL) == 4);

// Existing YRpp construction tag, shared without pulling in Syringe/assembly.
struct noinit_t final {};
