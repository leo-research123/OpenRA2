#pragma once
#include "support/test_support.hpp"
// Shared synthetic resources; these are format-valid fixtures, not original game assets.
#include "api/filesystem.hpp"
#include "api/map_objects.hpp"
#include "map_view.hpp"
#include "map_world.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace map_fixture {

void binary(const std::filesystem::path& p,const std::vector<unsigned char>& v) {
    std::ofstream f(p,std::ios::binary); f.write(reinterpret_cast<const char*>(v.data()),v.size());
}
void shp(const std::filesystem::path& path,int frames,int width=32,int height=32,int color=-1) {
    const int start=sizeof(SHPStruct)+frames*sizeof(SHPFrame);
    std::vector<unsigned char> v(start+frames*width*height);
    SHPStruct header; header.Width=width;header.Height=height; header.Frames=frames;
    std::memcpy(v.data(),&header,sizeof(header));
    for (int n=0;n<frames;++n) {
        SHPFrame f{}; f.Width=width;f.Height=height; f.Flags=0; f.Offset=start+n*width*height;
        std::memcpy(v.data()+sizeof(SHPStruct)+n*sizeof(SHPFrame),&f,sizeof(f));
        if(color>=0){std::fill(v.begin()+f.Offset,v.begin()+f.Offset+width*height,color);continue;}
        for (int y=2;y<height-2;++y) for (int x=2;x<width-2;++x) v[f.Offset+y*width+x]=42+n;
    }
    binary(path,v);
}
void fixtures(const std::filesystem::path& root) {
    std::ofstream(root/"TEMPERATMD.INI") << "[General]\nClearTile=0\n[TileSet0000]\nTilesInSet=1\nFileName=T\nAllowTiberium=yes\n";
    std::vector<unsigned char> tile(20+52+900);
    auto put=[&](int offset,int value) { std::memcpy(tile.data()+offset,&value,4); };
    put(0,1);put(4,1);put(8,60);put(12,30);put(16,20);
    std::fill(tile.begin()+72,tile.end(),42); binary(root/"T01.TEM",tile);
    std::vector<unsigned char> palette(768); for(std::size_t i=0;i<palette.size();++i)palette[i]=i%64;
    for(auto name:{"ISOTEM.PAL","UNITTEM.PAL","ANIM.PAL"})binary(root/name,palette);
    binary(root/"ANIM.PAL",std::vector<unsigned char>(768,7));
    binary(root/"TEMPERAT.PAL",std::vector<unsigned char>(768,11));
    std::ofstream(root/"KEYBOARDMD.INI") << "[Hotkey]\nDeployObject=68\n";
    std::ofstream rules(root/"RULESMD.INI");
    rules << "[Clear]\nBuildable=yes\n[BuildingTypes]\n0=BLDG\n1=CATIME\n[BLDG]\nName=Native Building\nStrength=100\nSelectable=yes\n"
        "[TerrainTypes]\n0=DRILL\n[DRILL]\nName=Ore Drill\nStrength=200\nTheater=no\nIsAnimated=yes\nAnimationRate=1\nAnimationProbability=1\nSpawnsTiberium=yes\n"
        "[Tiberiums]\n0=ORETYPE\n[ORETYPE]\nImage=1\nValue=25\nGrowth=1\nGrowthPercentage=1\nSpread=0\n[OverlayTypes]\n";
    for(int i=0;i<114;++i)rules<<i<<"=ORE"<<i<<"\n";
    for(int i=102;i<114;++i){rules<<"[ORE"<<i<<"]\nTiberium=yes\nImage=ORE"<<i<<"\nTheater=no\n";shp(root/("ORE"+std::to_string(i)+".SHP"),12);}
    std::ofstream(root/"ARTMD.INI") << "[BLDG]\nFoundation=2x2\nNewTheater=no\nNormalZAdjust=-20\nExtraLight=-10\nZShapePointMove=2,-3\nAnimIdle=0,1,1\nActiveAnim=SPIN\nActiveAnimX=1\nActiveAnimZAdjust=-20\nActiveAnimYSort=11\nActiveAnimDamaged=SPINDAM\n"
        "[DRILL]\nFoundation=1x1\n[SPIN]\nRate=1\nStart=0\nEnd=2\nLoopEnd=2\nShadow=yes\nUseNormalLight=yes\nTranslucency=50\nYDrawOffset=7\nZAdjust=99\nYSortAdjust=99\n"
        "[SPINDAM]\nRate=1\nStart=0\nEnd=2\nLoopEnd=2\nShadow=no\n";
    shp(root/"BLDG.SHP",2);shp(root/"DRILL.SHP",4);shp(root/"SPIN.SHP",4);shp(root/"SPINDAM.SHP",2);shp(root/"ORE.SHP",12);
    shp(root/"BUILDNGZ.SHA",1,396,477);
    // Format-valid neutral lighting assets for the real background pass.
    shp(root/"SHROUD.SHP",48,60,30,127);shp(root/"FOG.SHP",48,60,30,127);
    std::ofstream(root/"world.map") << "[Map]\nSize=0,0,8,12\nLocalSize=0,0,8,12\nLevel=0\nTheater=TEMPERATE\n[Basic]\nNewINIFormat=4\n"
        "[Structures]\n0=Neutral,BLDG,128,8,3,0,None,1,0,1,0,0,None,None,None,0,0\n[Terrain]\n7008=DRILL\n"
        // ALL01UMD retains this unused Rules registry entry with only map overrides.
        "[CATIME]\nClickRepairable=no\nCanBeOccupied=no\nLeaveRubble=yes\n";
}
}
