// The production release body and Event constructors execute in this probe.
// Other original callees remain explicit trace boundaries; notably Vox and
// Beacon::Place are not implemented on native hosts yet.
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/VoxClass.h"
#include <cstddef>
void YRPP_FASTCALL VoxClass::Play(const char* name,int control,int priority) noexcept {
    reinterpret_cast<void(__fastcall*)(const char*,int,int)>(0x752700)(name,control,priority);
}
static_assert(offsetof(DisplayClass,CurrentBuilding)==0x11A4);
static_assert(offsetof(DisplayClass,CurrentBuildingType)==0x11A8);
static_assert(offsetof(DisplayClass,unknown_1180)==0x1180);
static_assert(offsetof(DisplayClass,LeftPressAndDraggingRectangle)==0x11CF);
static_assert(offsetof(TacticalClass,Redrawing)==0xD7D);
static_assert(offsetof(BuildingClass,HasPower)==0x660);
static_assert(offsetof(BuildingClass,Translucency)==0x6ED);
static_assert(offsetof(TechnoTypeClass,Naval)==0xCCE);
DisplayClass& DisplayClass::Instance=*reinterpret_cast<DisplayClass*>(0x87F7E8);
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x87F7E8);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
TacticalClass*& TacticalClass::Instance=*reinterpret_cast<TacticalClass**>(0x887324);
InputManagerClass*& InputManagerClass::Instance=*reinterpret_cast<InputManagerClass**>(0x87F770);
BeaconManagerClass& BeaconManagerClass::Instance=*reinterpret_cast<BeaconManagerClass*>(0x89C3B0);
QueueClass<EventClass,128>& EventClass::OutList=*reinterpret_cast<QueueClass<EventClass,128>*>(0xA802C8);
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=*reinterpret_cast<DynamicVectorClass<ObjectClass*>*>(0xA8ECB8);
bool& Unsorted::ScenarioStarted=*reinterpret_cast<bool*>(0xA8ED5C);
byte& Unsorted::ArmageddonMode=*reinterpret_cast<byte*>(0xA8ED6B);
bool& Unsorted::DragSelectAborted=*reinterpret_cast<bool*>(0xA8ED9D);
int& Unsorted::MuteSWLaunches=*reinterpret_cast<int*>(0xA8B538);
int& Unsorted::CurrentFrame=*reinterpret_cast<int*>(0xA8ED84);
bool& Game::AttackMoveMode=*reinterpret_cast<bool*>(0xB0FE58);
bool& Game::IsActive=*reinterpret_cast<bool*>(0xA8E9A0);
DWORD SystemTimer::GetMilliseconds() noexcept {return reinterpret_cast<DWORD(__cdecl*)()>(0x01030100)();}
bool DisplayClass::PassesProximityCheck(ObjectTypeClass* t,int house,CellStruct* f,CellStruct* at){return reinterpret_cast<bool(__thiscall*)(DisplayClass*,ObjectTypeClass*,int,CellStruct*,CellStruct*)>(0x4A8EB0)(this,t,house,f,at);}
bool BuildingClass::CanUpgrade(const BuildingTypeClass* t,const HouseClass* h) const noexcept{return reinterpret_cast<bool(__thiscall*)(const BuildingClass*,const BuildingTypeClass*,const HouseClass*)>(0x452670)(this,t,h);}
int YRPP_FASTCALL Game::ClearSidebarTabObject(const TechnoClass* t) noexcept{return reinterpret_cast<int(__fastcall*)(const TechnoClass*)>(0x734270)(t);}
void DisplayClass::SetActiveFoundation(const CellStruct* c) noexcept{reinterpret_cast<void(__thiscall*)(DisplayClass*,const CellStruct*)>(0x4A8BF0)(this,c);}
void DisplayClass::SetActiveFoundationCopy(const CellStruct* c) noexcept{reinterpret_cast<void(__thiscall*)(DisplayClass*,const CellStruct*)>(0x4A8D50)(this,c);}
bool InputManagerClass::IsKeyPressed(int k) const{return reinterpret_cast<bool(__thiscall*)(const InputManagerClass*,int)>(0x54F5C0)(this,k);}
bool TacticalClass::HasBandObjects() const{return reinterpret_cast<bool(__thiscall*)(const TacticalClass*)>(0x6DA080)(this);}
void MapClass::UnselectAll() noexcept{reinterpret_cast<void(__cdecl*)()>(0x48DC90)();}
void TacticalClass::SelectRubberBand(SelectionCallback cb){reinterpret_cast<void(__thiscall*)(TacticalClass*,SelectionCallback)>(0x6D9FF0)(this,cb);}
void TacticalClass::StartDrawActionLineTimer(){reinterpret_cast<void(__cdecl*)()>(0x70D150)();}
void YRPP_FASTCALL DisplayClass::BandboxSelectionCallback(ObjectClass*) noexcept{__debugbreak();}
bool HouseClass::IsControlledByCurrentPlayer() const{return reinterpret_cast<bool(__thiscall*)(const HouseClass*)>(0x50B6F0)(this);}
bool Game::IsTypeSelecting() noexcept{return reinterpret_cast<bool(__cdecl*)()>(0x732D00)();}
void YRPP_FASTCALL Game::UICommands_TypeDeselect(const char* name) noexcept{reinterpret_cast<void(__fastcall*)(const char*)>(0x732600)(name);}
void YRPP_FASTCALL Game::UICommands_TypeSelect_7327D0(const char* name) noexcept{reinterpret_cast<void(__fastcall*)(const char*)>(0x7327D0)(name);}
CellClass* MapClass::GetCellAt(const CellStruct& at) const{return reinterpret_cast<CellClass*(__thiscall*)(const MapClass*,const CellStruct&)>(0x5657A0)(this,at);}
bool BeaconManagerClass::SelectBeacon(int x,int y,int z) noexcept{return reinterpret_cast<bool(__thiscall*)(BeaconManagerClass*,int,int,int)>(0x430F70)(this,x,y,z);}
bool Game::PlanningManager_CheckSelection() noexcept{return reinterpret_cast<bool(__cdecl*)()>(0x639040)();}
bool Game::PlanningManager_CheckCapacity() noexcept{return reinterpret_cast<bool(__cdecl*)()>(0x639130)();}
void DisplayClass::ActiveClick(ObjectClass* o,CellStruct c,Action a){reinterpret_cast<void(__thiscall*)(DisplayClass*,ObjectClass*,CellStruct,Action)>(0x4AE750)(this,o,c,a);}
TargetClass::TargetClass(AbstractClass* o) noexcept{reinterpret_cast<void(__thiscall*)(TargetClass*,AbstractClass*)>(0x6E6AB0)(this,o);}
void DisplayClass::SetBeaconMode(int mode) noexcept{reinterpret_cast<void(__thiscall*)(DisplayClass*,int)>(0x4AC960)(this,mode);}
SuperWeaponTypeClass* YRPP_FASTCALL SuperWeaponTypeClass::FindFirstOfAction(::Action a) noexcept{return reinterpret_cast<SuperWeaponTypeClass*(__fastcall*)(::Action)>(0x6CEEB0)(a);}
extern "C" {
__declspec(dllexport) void __fastcall LeftRelease(DisplayClass* d,void*,const CoordStruct& xyz,const CellStruct& c,ObjectClass* o,Action a,DWORD mini){d->DisplayClass::LeftMouseButtonUp(xyz,c,o,a,mini);}
// CRT dynamic-cast is a shared boundary with original RTTI descriptors checked
// by the harness. No alternate class predicate is compiled into production.
void* __cdecl __RTDynamicCast(void* p,long offset,void* from,void* to,int ref){return reinterpret_cast<void*(__cdecl*)(void*,long,void*,void*,int)>(0x7CAAE4)(p,offset,from,to,ref);}
void* __cdecl memset(void* p,int v,std::size_t n){auto* b=static_cast<volatile unsigned char*>(p);for(std::size_t i=0;i<n;++i)b[i]=static_cast<unsigned char>(v);return p;}
void* __cdecl memcpy(void* p,const void* s,std::size_t n){auto* b=static_cast<volatile unsigned char*>(p);auto* a=static_cast<const volatile unsigned char*>(s);for(std::size_t i=0;i<n;++i)b[i]=a[i];return p;}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
__declspec(noreturn) void __cdecl abort(){__debugbreak();for(;;){}}
void* type_info_vtable[1]{};
}
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_type_info_vtable")
