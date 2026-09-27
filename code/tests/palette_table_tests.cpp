#include "support/test_support.hpp"
// Links base core only: palette generation must not require Surface/Blitters.
#include "yrpp/ConvertClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/ColorScheme.h"
#include <fstream>
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

TEST(PaletteTable, Contracts) {
    BytePalette palette;
    for (auto& color : palette.Entries) color.R=color.G=color.B=128;
    std::array<BYTE,256> indexes{}; indexes.fill(1); indexes[2]=0;
    std::vector<WORD> output(53*256,0xa55a);
    EXPECT_TRUE((!ConvertClass::BuildColorTable(palette,output.data(),255,1,2))) << "reject insufficient base output";
    EXPECT_TRUE((!LightConvertClass::BuildColorTable(palette,output.data(),output.size(),13,1000,1000,1000,indexes.data(),256,2,false))) << "reject invalid light shade count";
    EXPECT_TRUE((!LightConvertClass::BuildColorTable(palette,output.data(),output.size(),53,1000,1000,1000,indexes.data(),255,2,false))) << "reject short index mask";
    EXPECT_TRUE((std::all_of(output.begin(),output.end(),[](WORD v) { return v==0xa55a; }))) << "invalid calls preserve output";
    const int mode_before=static_cast<int>(Drawing::ColorMode);
    constexpr WORD normal[]{0x4210,0x8420,0x8410,0x8210};
    constexpr WORD lit[]{0x3def,0x7bdf,0x7bef,0x7def};
    for (int mode=0;mode<4;++mode) {
        EXPECT_TRUE((ConvertClass::BuildColorTable(palette,output.data(),output.size(),1,mode))) << "base palette without renderer";
        EXPECT_TRUE((output[0]==normal[mode] && output[1]==normal[mode])) << "base includes palette index zero";
        for (bool mmx : {false,true}) {
            EXPECT_TRUE((LightConvertClass::BuildColorTable(palette,output.data(),output.size(),53,1000,1000,1000,indexes.data(),256,mode,mmx))) << "light table without renderer";
            EXPECT_TRUE((output[0]==0 && output[1]==0 && output[2]==0)) << "darkest row and transparent index";
            EXPECT_TRUE((output[26*256]==0 && output[26*256+1]==lit[mode] && output[26*256+2]==normal[mode])) << "original 65535 factor preserves the one-step midpoint difference and ignored indexes";
            EXPECT_TRUE((output[52*256+1]==(mode ? 0xffff : 0x7fff))) << "bright row uses original saturation";
        }
    }
    EXPECT_TRUE((static_cast<int>(Drawing::ColorMode)==mode_before)) << "explicit format never mutates host color state";
    EXPECT_TRUE((LightConvertClass::BuildColorTable(palette,output.data(),output.size(),0,1000,1000,1000,nullptr,0,2,false)
        && output[1]==0x7bef)) << "zero shades normalizes to one and a null mask lights all colors";
    DWORD intensity=0; int terrain=1000,red=3,green=3,blue=3;
    LightConvertClass::NormalizeCellLight(intensity,terrain,red,green,blue);
    EXPECT_TRUE((intensity==196 && terrain==2 && red==1000 && green==999 && blue==999)) << "red-dominant branch retains the original extended denominator";
    terrain=1000; red=2; green=3; blue=3;
    LightConvertClass::NormalizeCellLight(intensity,terrain,red,green,blue);
    EXPECT_TRUE((intensity==196 && terrain==2 && red==668 && green==1000 && blue==1003)) << "green-dominant branch reloads the truncated integer denominator";
}
}


TEST(PaletteTable, ColorSchemeUnlitRangeAndFailure) {
    BytePalette palette{};for(auto& color:palette.Entries)color={128,128,128};
    std::vector<WORD> table(53*256,0xA55A);
    EXPECT_FALSE(ColorScheme::BuildColorTable(palette,table.data(),table.size()-1,53,2,false));
    EXPECT_TRUE(std::all_of(table.begin(),table.end(),[](WORD v){return v==0xA55A;}));
    for(bool mmx:{false,true}){
        ASSERT_TRUE(ColorScheme::BuildColorTable(palette,table.data(),table.size(),53,2,mmx));
        for(int index=1;index<256;++index){
            EXPECT_EQ(table[26*256+index],(index>=240&&index<255)?0x8410:0x7BEF)<<index;
            EXPECT_EQ(table[52*256+index],(index>=240&&index<255)?0x8410:0xFFFF)<<index;
        }
        EXPECT_EQ(table[26*256],0);EXPECT_EQ(table[52*256],0);
    }
}
