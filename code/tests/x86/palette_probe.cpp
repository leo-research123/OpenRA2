// Public base-core palette ABI, independent of object/vtable addresses.
#include "yrpp/ConvertClass.h"
#include <cstdint>
extern "C" __declspec(dllexport) int __stdcall PaletteProbe_Convert(const BytePalette* palette,
    WORD* output,unsigned capacity,int shades,int mode) {
    return palette && ConvertClass::BuildColorTable(*palette,output,capacity,shades,mode);
}
extern "C" __declspec(dllexport) int __stdcall PaletteProbe_Light(const BytePalette* palette,
    WORD* output,unsigned capacity,int shades,int red,int green,int blue,const BYTE* indexes,int mode,int mmx) {
    return palette && LightConvertClass::BuildColorTable(*palette,output,capacity,shades,
        red,green,blue,indexes,indexes ? 256 : 0,mode,mmx!=0);
}
extern "C" __declspec(dllexport) int __stdcall PaletteProbe_Normalize(DWORD* values) {
    return LightConvertClass::NormalizeCellLight(values[0],reinterpret_cast<int*>(values)[1],
        reinterpret_cast<int*>(values)[2],reinterpret_cast<int*>(values)[3],reinterpret_cast<int*>(values)[4]);
}
