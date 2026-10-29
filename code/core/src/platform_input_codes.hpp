#pragma once
#include "yrpp/platform/ABI.h"

namespace game {
// Win32 wire codes retained by the original input consumers. Native hosts
// translate their device events to these values; they are not host key codes.
inline constexpr int Win32KeyEscape = 0x1B;
inline constexpr int Win32KeyTab = 0x09;
inline constexpr int Win32KeyLeft = 0x25;
inline constexpr int Win32KeyRight = 0x27;
inline constexpr int Win32KeyUp = 0x26;
inline constexpr int Win32KeyDown = 0x28;

inline constexpr UINT Win32MessageLeftButtonDown = 0x0201;
inline constexpr UINT Win32MessageLeftButtonUp = 0x0202;
inline constexpr UINT Win32MessageRightButtonDown = 0x0204;
inline constexpr UINT Win32MessageRightButtonUp = 0x0205;
inline constexpr UINT Win32MessageMiddleButtonDown = 0x0207;
inline constexpr UINT Win32MessageMiddleButtonUp = 0x0208;
inline constexpr UINT Win32MessageMouseMove = 0x0200;
inline constexpr UINT Win32MessageTimer = 0x0113;

#if defined(_WIN32)
static_assert(Win32KeyEscape == VK_ESCAPE && Win32KeyTab == VK_TAB && Win32KeyLeft == VK_LEFT &&
              Win32KeyRight == VK_RIGHT && Win32KeyUp == VK_UP && Win32KeyDown == VK_DOWN);
static_assert(Win32MessageLeftButtonDown == WM_LBUTTONDOWN && Win32MessageLeftButtonUp == WM_LBUTTONUP &&
              Win32MessageRightButtonDown == WM_RBUTTONDOWN && Win32MessageRightButtonUp == WM_RBUTTONUP &&
              Win32MessageMiddleButtonDown == WM_MBUTTONDOWN && Win32MessageMiddleButtonUp == WM_MBUTTONUP &&
              Win32MessageMouseMove == WM_MOUSEMOVE && Win32MessageTimer == WM_TIMER);
#endif
}
