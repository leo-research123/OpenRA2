// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 scheme.cpp named constructor; YR 0x0068C710 / 0x0068C3B0.
// Electronic Arts / OpenTS; terms: third_party/opents/LICENSE.md.
// The software component supplies the original converter-owning constructor.
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include <array>
#if !defined(RA2_YRPP_GAME)
ColorScheme::ColorScheme(const char* id,const ColorStruct& baseColor,
    const BytePalette& palette1,const BytePalette& palette2,int shades,bool add)
    : ColorScheme(id,baseColor,palette1,shades,false) {
    // LightConvert retains this pointer (original global at 0x0083E1AC).
    static std::array<BYTE,256> indexes=[] {
        std::array<BYTE,256> value{};
        for(int i=0;i<256;++i)value[i]=BYTE(i<240 || i==255);
        return value;
    }();
    LightConvert=new LightConvertClass(&Colors,const_cast<BytePalette*>(&palette2),
        2,1000,1000,1000,false,indexes.data(),shades);
    if(add){Array.AddItem(this);ArrayIndex=Array.Count;}
}
#endif
