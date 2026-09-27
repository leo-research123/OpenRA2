// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 scheme.cpp lifecycle.
// Electronic Arts / OpenTS; additional terms: third_party/opents/LICENSE.md.
// YR 0x0068C710 / 0x0068C8D0; shared palette arithmetic is calibrated separately.
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include <cstdlib>
#include <cstring>
#include <new>

#if !defined(RA2_YRPP_GAME)
namespace {
DynamicVectorClass<ColorScheme*> color_schemes;
}
DynamicVectorClass<ColorScheme*>& ColorScheme::Array=color_schemes;
ColorScheme::ColorScheme(const char* id,const ColorStruct& baseColor,
    const BytePalette& palette,int shadeCount,bool addToArray)
    : Colors{},ID(nullptr),BaseColor(baseColor),LightConvert(nullptr),ShadeCount(shadeCount) {
    const int first[]{16,15,25,24,22,16,19};
    const int last[]{16,21};
    std::memcpy(unknown_314,first,sizeof(first));
    MainShadeIndex=16;
    std::memcpy(unknown_334,last,sizeof(last));
    const auto length=std::strlen(id)+1;
    ID=static_cast<char*>(std::malloc(length));
    if(!ID)throw std::bad_alloc();
    std::memcpy(ID,id,length);
    try {
        BuildPalette(BaseColor,palette,Colors);
        if(addToArray){Array.AddItem(this);ArrayIndex=Array.Count;}
    } catch(...) {delete LightConvert;std::free(ID);throw;}
}
ColorScheme::~ColorScheme() {
    std::free(ID);ID=nullptr;
    delete LightConvert;LightConvert=nullptr;
    Array.Remove(this);
}
#endif
