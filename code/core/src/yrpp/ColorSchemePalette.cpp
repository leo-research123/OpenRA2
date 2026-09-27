// Adapted from OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9,
// code/scheme.cpp Build_Light_Converter and code/hsv.cpp HSVClass::operator RGBClass.
// Copyright Electronic Arts. GPL-3.0 with EA additional terms; see
// code/third_party/opents/LICENSE.md and source.json.
// YR 1.001 calibration: 0x68C3B0 uses the quantized Math sine/cosine table;
// 0x517440 uses integer HSV division; 0x68C710 passes the mask at 0x83E1AC.
#include "yrpp/ColorScheme.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/YRMath.h"

ColorStruct ColorScheme::HSVToRGB(const ColorStruct& hsv) noexcept {
    const unsigned hue=unsigned(hsv.R)*6, saturation=hsv.G, value=hsv.B;
    const unsigned f=hue%255;
    const unsigned values[7]={0,value,value,
        value*(255-saturation*f/255)/255,
        value*(255-saturation)/255,value*(255-saturation)/255,
        value*(255-saturation*(255-f)/255)/255};
    unsigned i=hue/255;
    i=i>4?i-4:i+2;const BYTE red=BYTE(values[i]);
    i=i>4?i-4:i+2;const BYTE blue=BYTE(values[i]);
    i=i>4?i-4:i+2;const BYTE green=BYTE(values[i]);
    return {red,green,blue};
}
void ColorScheme::BuildPalette(const ColorStruct& hsv,const BytePalette& source,
        BytePalette& output) noexcept {
    output=source;
    for(int i=0;i<16;++i){
        const double cosine=i?0.3490658503988659+i*0.08144869842640204:0.1963495408493621;
        const double sine=0.8726646259971648+i*0.04654211338651545;
        output.Entries[16+i]=HSVToRGB({hsv.R,
            BYTE(int(Math::sin(sine)*hsv.G)),BYTE(int(Math::cos(cosine)*hsv.B))});
    }
}
bool ColorScheme::BuildColorTable(const BytePalette& palette,WORD* output,
        std::size_t capacity,int shades,int mode,bool mmx) noexcept {
    BYTE indexes[256];
    for(int i=0;i<256;++i)indexes[i]=BYTE(i<240||i==255);
    return LightConvertClass::BuildColorTable(palette,output,capacity,shades,
        1000,1000,1000,indexes,256,mode,mmx);
}
