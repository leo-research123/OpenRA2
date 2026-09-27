#pragma once

#include "yrpp/platform/ABI.h"

#include <windows.h>
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/Unsorted.h"

class UI {
public:
    typedef BOOL (CALLBACK *Callback)(HWND, UINT, WPARAM, LPARAM);

    /// VA: 0x004A3B40.
    static HGLOBAL YRPP_FASTCALL GetResource(LPCTSTR lpName, LPCTSTR lpType) JMP_STD(0x4A3B40)
    /// VA: 0x0060D450.
    static void RegisterComboDropAndNewEditClasses() JMP_STD(0x60D450)
    /// VA: 0x00622B50.
    static BOOL YRPP_FASTCALL StandardWndProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) JMP_STD(0x622B50)
    /// VA: 0x00622650.
    static HWND YRPP_FASTCALL BeginDialog(LPCTSTR lpName, Callback windProc, DWORD dwUnk) JMP_STD(0x622650)
    /// VA: 0x00622720.
    static void YRPP_FASTCALL EndDialog(HWND hDlg) JMP_STD(0x622720)
    /// VA: 0x00623120.
    static bool Updated() JMP_STD(0x623120)
    /// VA: 0x00622800.
    static void YRPP_FASTCALL FocusOnWindow(HWND hWnd) JMP_STD(0x622800)
    /// VA: 0x005D4ED0.
    static void YRPP_FASTCALL RemoveModelessDialog(HWND hWnd) JMP_STD(0x5D4ED0)
    /// VA: 0x00777060.
    static void YRPP_FASTCALL CenterWindow(HWND hWnd) JMP_STD(0x777060)
    /// VA: 0x00622820.
    static void YRPP_FASTCALL RegisterWindow(HWND hWnd, LPARAM msg) JMP_STD(0x622820)
    /// VA: 0x0061EF70.
    static void YRPP_FASTCALL GetKeyboardKeyString(unsigned short key, wchar_t* buffer) JMP_STD(0x61EF70)
    // Fills lpRect with a dialog control's rectangle in display (surface)
    // coordinates, accounting for the full-screen owner-draw scaling. This is
    // how the engine's reconnect dialog positions its per-player sync bars.
    /// VA: 0x00775690.
    static void YRPP_FASTCALL GetDisplayRect(HWND hWnd, LPRECT lpRect) JMP_STD(0x775690)
};
