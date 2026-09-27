#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/Unsorted.h"
#include <cstddef>
static_assert(offsetof(DisplayClass,CurrentFoundation_CenterCell)==0x1174);
static_assert(offsetof(DisplayClass,CurrentFoundation_TopLeftOffset)==0x1178);
static_assert(offsetof(DisplayClass,CurrentFoundation_Data)==0x117C);
static_assert(offsetof(DisplayClass,CurrentFoundationCopy_CenterCell)==0x1182);
static_assert(offsetof(DisplayClass,CurrentFoundationCopy_TopLeftOffset)==0x1186);
static_assert(offsetof(DisplayClass,CurrentFoundationCopy_Data)==0x118C);
static_assert(offsetof(CellClass,AltFlags)==0x12C);
static_assert(offsetof(BuildingClass,Type)==0x520);
static_assert(offsetof(BuildingClass,UpgradeLevel)==0x702);
static_assert(offsetof(BuildingTypeClass,PowersUpBuilding)==0xE88);
static_assert(offsetof(BuildingTypeClass,Upgrades)==0x14E0);
static_assert(offsetof(BuildingTypeClass,PowersUpToLevel)==0x16FC);
TechnoClass* (&Game::SidebarTabObjects)[2]=*reinterpret_cast<TechnoClass*(*)[2]>(0xB0FE5C);
bool MapClass::CoordinatesLegal(const CellStruct& cell) const {
    return reinterpret_cast<bool(__thiscall*)(const MapClass*,const CellStruct&)>(0x568300)(this,cell);
}
CellClass* MapClass::GetCellAt(const CellStruct& cell) const {
    return reinterpret_cast<CellClass*(__thiscall*)(const MapClass*,const CellStruct&)>(0x5657A0)(this,cell);
}
CellStruct* DisplayClass::FoundationBoundsSize(CellStruct& out,const CellStruct* cells) const {
    return reinterpret_cast<CellStruct*(__thiscall*)(const DisplayClass*,CellStruct&,const CellStruct*)>(0x4A94F0)(this,out,cells);
}
CellStruct DisplayClass::FoundationBoundsSize(const CellStruct* cells) const {
    CellStruct out;return *FoundationBoundsSize(out,cells);
}
extern "C" {
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
__declspec(dllexport) void __fastcall SetActive(DisplayClass* display,void*,const CellStruct* cells){display->SetActiveFoundation(cells);}
__declspec(dllexport) void __fastcall SetPending(DisplayClass* display,void*,const CellStruct* cells){display->SetActiveFoundationCopy(cells);}
__declspec(dllexport) void __fastcall MarkActive(DisplayClass* display,void*,CellStruct* at,bool mark){display->MarkFoundation(at,mark);}
__declspec(dllexport) void __fastcall MarkPending(DisplayClass* display,void*,CellStruct* at,bool mark){display->MarkFoundationCopy(at,mark);}
__declspec(dllexport) CellStruct* __fastcall Buffer(int pending){return pending?DisplayClass::PendingFoundationBuffer:DisplayClass::ActiveFoundationBuffer;}
__declspec(dllexport) bool __fastcall CanUpgrade(BuildingClass* building,void*,const BuildingTypeClass* upgrade,const HouseClass* owner){return building->CanUpgrade(upgrade,owner);}
__declspec(dllexport) int __fastcall ClearSidebar(const TechnoClass* object){return Game::ClearSidebarTabObject(object);}
void* __cdecl memcpy(void* output,const void* input,std::size_t count){
    auto* to=static_cast<volatile unsigned char*>(output);const auto* from=static_cast<const volatile unsigned char*>(input);
    for(std::size_t i=0;i<count;++i)to[i]=from[i];return output;
}
int __cdecl PlacementCompare(const char* left,const char* right){return reinterpret_cast<int(__cdecl*)(const char*,const char*)>(0x7C8D20)(left,right);}
auto PlacementStrcmpi=&PlacementCompare;
}
#pragma comment(linker,"/alternatename:__imp___strcmpi=_PlacementStrcmpi")
