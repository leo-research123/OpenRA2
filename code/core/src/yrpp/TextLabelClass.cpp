// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 code/txtlabel.cpp, EA/OpenTS; third_party/opents/LICENSE.md.
// YR 0x0072A440 adds animation/scroll fields to the existing Gadget label.
#include "yrpp/TextLabelClass.h"
#if !defined(RA2_YRPP_GAME)
TextLabelClass::TextLabelClass(wchar_t* text,int x,int y,int color,TextPrintType style) noexcept
    :GadgetClass(x,y,1,1,GadgetFlag(0),false),UserData1(nullptr),UserData2(nullptr),
     Style(DWORD(style)),Text(text),ColorSchemeIndex(color),PixWidth(0xFFFFFFFF),
     anim_dword3C(0),SkipDraw(false),Animate(false),AnimPos(0),AnimTiming(0) {}
#endif
