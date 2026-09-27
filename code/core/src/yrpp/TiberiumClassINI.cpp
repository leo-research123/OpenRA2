// Original resource INI and identity: 0x00721A50 / 0x005FDD20.
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/CCINIClass.h"
#include <algorithm>
bool TiberiumClass::LoadFromINI(CCINIClass*ini){if(!ini||!AbstractTypeClass::LoadFromINI(ini))return false;
 Spread=ini->ReadInteger(ID,"Spread",Spread);Growth=ini->ReadInteger(ID,"Growth",Growth);SpreadPercentage=ini->ReadDouble(ID,"SpreadPercentage",SpreadPercentage);GrowthPercentage=ini->ReadDouble(ID,"GrowthPercentage",GrowthPercentage);
 Value=std::max(0,ini->ReadInteger(ID,"Value",Value));Power=ini->ReadInteger(ID,"Power",Power);int image=ini->ReadInteger(ID,"Image",1);int base=image==2?27:image==3?127:image==4?147:102;Image=OverlayTypeClass::Array.GetItemOrDefault(base);NumFrames=12;NumImages=12;NumSlopes=image==2?0:8;return true;
}
