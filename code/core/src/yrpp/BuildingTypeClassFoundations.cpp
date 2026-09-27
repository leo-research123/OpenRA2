// Original occupied-cell tables reconstructed from the saved EXE analysis.
// Navigation exterior tables are intentionally not fabricated.
#include "map_world.hpp"
#include "yrpp/BuildingTypeClass.h"
namespace {
// Original SetActiveFoundation copies 120 cells including trailing storage.
// Keep every canonical source padded so that native callers honor that span.
CellStruct b0[120]={{0,0},{32767,32767}};
CellStruct b1[120]={{0,0},{1,0},{32767,32767}};
CellStruct b2[120]={{0,0},{0,1},{32767,32767}};
CellStruct b3[120]={{0,0},{1,0},{0,1},{1,1},{32767,32767}};
CellStruct b4[120]={{0,0},{1,0},{0,1},{1,1},{0,2},{1,2},{32767,32767}};
CellStruct b5[120]={{0,0},{1,0},{2,0},{0,1},{1,1},{2,1},{32767,32767}};
CellStruct b6[120]={{0,0},{1,0},{2,0},{0,1},{1,1},{2,1},{0,2},{1,2},{2,2},{32767,32767}};
CellStruct b7[120]={{0,0},{1,0},{2,0},{0,1},{1,1},{2,1},{0,2},{1,2},{2,2},{0,3},{1,3},{2,3},{0,4},{1,4},{2,4},{32767,32767}};
CellStruct b8[120]={{0,0},{1,0},{2,0},{3,0},{0,1},{1,1},{2,1},{3,1},{32767,32767}};
CellStruct b9[120]={{0,0},{1,0},{2,0},{0,1},{1,1},{0,2},{1,2},{2,2},{32767,32767}};
CellStruct b10[120]={{0,0},{0,1},{0,2},{32767,32767}};
CellStruct b11[120]={{0,0},{1,0},{2,0},{32767,32767}};
CellStruct b12[120]={{0,0},{1,0},{2,0},{3,0},{0,1},{1,1},{2,1},{3,1},{0,2},{1,2},{2,2},{3,2},{32767,32767}};
CellStruct b13[120]={{0,0},{0,1},{0,2},{0,3},{32767,32767}};
CellStruct b14[120]={{0,0},{0,1},{0,2},{0,3},{0,4},{32767,32767}};
CellStruct b15[120]={{0,0},{1,0},{0,1},{1,1},{0,2},{1,2},{0,3},{1,3},{0,4},{1,4},{0,5},{1,5},{32767,32767}};
CellStruct b16[120]={{0,0},{1,0},{0,1},{1,1},{0,2},{1,2},{0,3},{1,3},{0,4},{1,4},{32767,32767}};
CellStruct b17[120]={{0,0},{1,0},{2,0},{3,0},{4,0},{0,1},{1,1},{2,1},{3,1},{4,1},{0,2},{1,2},{2,2},{3,2},{4,2},{32767,32767}};
CellStruct b18[120]={{0,0},{1,0},{2,0},{3,0},{0,1},{1,1},{2,1},{3,1},{0,2},{1,2},{2,2},{3,2},{0,3},{1,3},{2,3},{3,3},{32767,32767}};
CellStruct b19[120]={{0,0},{1,0},{2,0},{0,1},{1,1},{2,1},{0,2},{1,2},{2,2},{0,3},{1,3},{2,3},{32767,32767}};
CellStruct b20[120]={{0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{0,1},{1,1},{2,1},{3,1},{4,1},{5,1},{0,2},{1,2},{2,2},{3,2},{4,2},{5,2},{0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{32767,32767}};
CellStruct b21[120]={{32767,32767}};
CellStruct* buildings[]={b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15,b16,b17,b18,b19,b20,b21};
CellStruct* terrain[]={b0,b1,b2,b3,b4,b5,b6,b9};
constexpr short widths[]={1,2,1,2,2,3,3,3,4,3,1,3,4,1,1,2,2,5,4,3,6,0};
constexpr short heights[]={1,1,2,2,3,2,3,5,2,3,3,1,3,4,5,6,5,3,4,4,4,0};
}
namespace game {
bool native_foundation(void*,int index,CellStruct*&out) noexcept{out=index>=0&&index<8?terrain[index]:nullptr;return out!=nullptr;}
CellStruct* native_building_foundation(int index) noexcept{return index>=0&&index<22?buildings[index]:nullptr;}
}
short BuildingTypeClass::GetFoundationWidth() const{int i=static_cast<int>(Foundation);return i>=0&&i<22?widths[i]:0;}
// OpenTS builtype.cpp::Lepton_Dimensions, calibrated at YR 0x00464AF0.
// Native uses the original initialized level height (0x0089DDB8 = 104).
CoordStruct* BuildingTypeClass::Dimension2(CoordStruct* output) {
 if(output)*output={GetFoundationWidth()*256,GetFoundationHeight(false)*256,Height*Unsorted::LevelHeight};
 return output;
}
// OpenTS 44fac744 builtype.cpp::Occupy_List; YR 0x45EC20.
// GPL-3.0-or-later; EA Section 7 terms: third_party/opents/LICENSE.md.
CellStruct* BuildingTypeClass::GetFoundationData(bool) const {
 static CellStruct empty[120]{{0x7FFF,0x7FFF}};
 return FoundationData?FoundationData:empty;
}
bool BuildingTypeClass::IsVehicle() const {return UndeploysInto && GetFoundationWidth()==1 && GetFoundationHeight(false)==1;}
short BuildingTypeClass::GetFoundationHeight(bool bib) const{int i=static_cast<int>(Foundation);return i>=0&&i<22?short(heights[i]+(bib&&Bib?1:0)):0;}
