#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BeaconClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "scenario_runtime.hpp"
#include <cstddef>
static_assert(offsetof(TechnoClass,IsAlive)==0x90);
static_assert(offsetof(HouseClass,IsInPlayerControl)==0x1ED);
static_assert(offsetof(TacticalClass,SelectableCount)==0xDB0);
static_assert(offsetof(BeaconClass,Bitfield)==0xC);
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=*reinterpret_cast<DynamicVectorClass<ObjectClass*>*>(0xA8ECB8);
DynamicVectorClass<TechnoClass*>& TechnoClass::Array=*reinterpret_cast<DynamicVectorClass<TechnoClass*>*>(0xA8EC78);
TacticalClass*& TacticalClass::Instance=*reinterpret_cast<TacticalClass**>(0x887324);
BeaconClass* (&BeaconClass::Array)[8][3]=*reinterpret_cast<BeaconClass*(*)[8][3]>(0x89C3B0);
int& BeaconClass::Count=*reinterpret_cast<int*>(0x89C410);
int& Game::SelectionCommandMode=*reinterpret_cast<int*>(0xB0FE54);
bool& Game::AttackMoveMode=*reinterpret_cast<bool*>(0xB0FE58);
bool& Game::TypeSelectionIncludesMap=*reinterpret_cast<bool*>(0xB0FE64);
bool& Game::TypeSelectionActive=*reinterpret_cast<bool*>(0xB0FE65);
bool& Unsorted::MoveFeedback=*reinterpret_cast<bool*>(0x822CF2);
// Link-only dependencies of other methods in TacticalClassSelection.cpp.
// Every callable stub traps; none is part of the selection comparison.
int& Unsorted::CurrentFrame=*reinterpret_cast<int*>(0xA8ED84);
CDTimerClass& TechnoClass::ActionLineTimer=*reinterpret_cast<CDTimerClass*>(0xB0EA80);
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x87F7E8);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
RectangleStruct& TacticalClass::ViewBounds=*reinterpret_cast<RectangleStruct*>(0x88733C);
DynamicVectorClass<BuildingClass*>& BuildingClass::Array=*reinterpret_cast<DynamicVectorClass<BuildingClass*>*>(0xA8EB40);
Point2D TacticalClass::CoordsToScreen(const CoordStruct&) noexcept{__debugbreak();return {};}
CoordStruct AbstractClass::GetCoords() const{__debugbreak();return {};}
CellClass* MapClass::GetCellAt(const CoordStruct&) const{__debugbreak();return nullptr;}
CellClass* MapClass::GetCellAt(const CellStruct&) const{__debugbreak();return nullptr;}
bool TacticalClass::PickTerrainCell(const Point2D&,const RectangleStruct&,CellStruct&) noexcept{__debugbreak();return false;}
bool HouseClass::IsControlledByCurrentPlayer() const {
    return reinterpret_cast<bool(__thiscall*)(const HouseClass*)>(0x50B6F0)(this);
}
namespace game {
const ScenarioRuntimeServices& scenario_runtime() {
    static const ScenarioRuntimeServices runtime=[] {
        ScenarioRuntimeServices value{};
        value.session_mode=[](void*) noexcept {return *reinterpret_cast<int*>(0xA8B238);};
        return value;
    }();
    return runtime;
}
}
bool BuildingTypeClass::IsVehicle() const {
    return reinterpret_cast<bool(__thiscall*)(const BuildingTypeClass*)>(0x465D40)(this);
}
extern "C" {
int _fltused=0;
__declspec(dllexport) TacticalSelectableStruct* __cdecl SelectionArray(){return TacticalClass::SelectableObjects;}
__declspec(dllexport) void __fastcall SelectRectangle(TacticalClass* tactical,void*,const RectangleStruct& rect,TacticalClass::SelectionCallback callback){tactical->SelectThese(rect,callback);}
__declspec(dllexport) void __fastcall ClearSelection(){MapClass::UnselectAll();}
__declspec(dllexport) void __fastcall SelectType(const char* name){Game::UICommands_TypeSelect_7327D0(name);}
__declspec(dllexport) void __fastcall DeselectType(const char* name){Game::UICommands_TypeDeselect(name);}
__declspec(dllexport) void __fastcall ResetSelectionMode(int value){Game::SetSelectionCommandMode(value);}
__declspec(dllexport) bool __cdecl GetTypeSelection(){return Game::IsTypeSelecting();}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __cdecl __std_exception_copy(const __std_exception_data*,__std_exception_data*){__debugbreak();}
void __cdecl __std_exception_destroy(__std_exception_data*){__debugbreak();}
void __stdcall _CxxThrowException(void*,void*){__debugbreak();}
void* __cdecl memcpy(void* output,const void* input,std::size_t count){
    auto* to=static_cast<volatile unsigned char*>(output);const auto* from=static_cast<const volatile unsigned char*>(input);
    for(std::size_t i=0;i<count;++i)to[i]=from[i];return output;
}
void* __cdecl memset(void* output,int value,std::size_t count){
    auto* to=static_cast<volatile unsigned char*>(output);
    for(std::size_t i=0;i<count;++i)to[i]=static_cast<unsigned char>(value);return output;
}
int __cdecl SelectionCompare(const char* left,const char* right){return reinterpret_cast<int(__cdecl*)(const char*,const char*)>(0x7C8D20)(left,right);}
auto SelectionStrcmpi=&SelectionCompare;
void __cdecl SelectionRuntimeTrap(){__debugbreak();}
auto SelectionWatson=&SelectionRuntimeTrap;
void* SelectionTypeInfo[2]={nullptr,nullptr};
}
void* __cdecl operator new(std::size_t bytes){return reinterpret_cast<void*(__cdecl*)(std::size_t)>(0x7C8E17)(bytes);}
void __cdecl operator delete(void* p) noexcept{reinterpret_cast<void(__cdecl*)(void*)>(0x7C8B3D)(p);}
void __cdecl operator delete(void* p,std::size_t) noexcept{::operator delete(p);}
#pragma comment(linker,"/alternatename:__imp___invoke_watson=_SelectionWatson")
#pragma comment(linker,"/alternatename:__imp___strcmpi=_SelectionStrcmpi")
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_SelectionTypeInfo")
