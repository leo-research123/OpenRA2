// Execute the production cursor conversion body with original-layout objects.
#include "yrpp/DisplayClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Drawing.h"
#include "yrpp/Unsorted.h"
#include "yrpp/InputManagerClass.h"
#include "yrpp/WWMouseClass.h"
#include <cstddef>
static_assert(offsetof(DisplayClass,WaypointColorRed)==0x11CC);
static_assert(offsetof(BuildingTypeClass,InvisibleInGame)==0x1701);
static_assert(offsetof(TechnoTypeClass,MoveToShroud)==0xC8D);
static_assert(offsetof(ConvertClass,PaletteData)==0x174);
static_assert(offsetof(HouseClass,SelectedPathIndex)==0x20C);
static_assert(offsetof(MouseClass,MouseCursorIsMini)==0x555C);
static_assert(offsetof(MouseClass,MouseCursorIndex)==0x5560);
static_assert(sizeof(MouseClass)==0x556C);
MouseCursor (&MouseCursor::Cursors)[86]=*reinterpret_cast<MouseCursor(*)[86]>(0x82D028);
SHPStruct*& MouseClass::CursorShape=*reinterpret_cast<SHPStruct**>(0xABF294);
SysTimerClass& MouseClass::CursorTimer=*reinterpret_cast<SysTimerClass*>(0xABF2A0);
bool& MouseClass::CursorInitialized=*reinterpret_cast<bool*>(0xABF2DD);
WWMouseClass*& WWMouseClass::Instance=*reinterpret_cast<WWMouseClass**>(0x887640);
DWORD SystemTimer::GetTime() {return reinterpret_cast<DWORD(__cdecl*)()>(0x6C8C40)();}
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x87F7E8);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=*reinterpret_cast<DynamicVectorClass<ObjectClass*>*>(0xA8ECB8);
byte& Unsorted::ArmageddonMode=*reinterpret_cast<byte*>(0xA8ED6B);
bool& Game::AttackMoveMode=*reinterpret_cast<bool*>(0xB0FE58);
InputManagerClass*& InputManagerClass::Instance=*reinterpret_cast<InputManagerClass**>(0x87F770);
GameOptionsClass& GameOptionsClass::Instance=*reinterpret_cast<GameOptionsClass*>(0xA8EB60);
ConvertClass*& FileSystem::MOUSE_PAL=*reinterpret_cast<ConvertClass**>(0x87F6C8);
BytePalette& FileSystem::WAYPOINT_PAL=*reinterpret_cast<BytePalette*>(0x885180);
int& Drawing::RedShiftLeft=*reinterpret_cast<int*>(0x8A0DD0);
int& Drawing::RedShiftRight=*reinterpret_cast<int*>(0x8A0DD4);
int& Drawing::BlueShiftLeft=*reinterpret_cast<int*>(0x8A0DD8);
int& Drawing::BlueShiftRight=*reinterpret_cast<int*>(0x8A0DDC);
int& Drawing::GreenShiftLeft=*reinterpret_cast<int*>(0x8A0DE0);
int& Drawing::GreenShiftRight=*reinterpret_cast<int*>(0x8A0DE4);
CellClass* MapClass::GetCellAt(const CellStruct& cell) const {
    return reinterpret_cast<CellClass*(__thiscall*)(const MapClass*,const CellStruct*)>(0x5657A0)(this,&cell);
}
int MapClass::GetCellFloorHeight(const CoordStruct& coord) const {
    return reinterpret_cast<int(__thiscall*)(const MapClass*,const CoordStruct*)>(0x578080)(this,&coord);
}
bool MapClass::IsWithinUsableArea(const CellStruct& cell,bool height) const {
    return reinterpret_cast<bool(__thiscall*)(const MapClass*,const CellStruct*,bool)>(0x578460)(this,&cell,height);
}
WaypointClass* HouseClass::GetPlanningWaypointAt(CellStruct* cell) {
    return reinterpret_cast<WaypointClass*(__thiscall*)(HouseClass*,CellStruct*)>(0x5023B0)(this,cell);
}
bool HouseClass::GetPlanningWaypointProperties(WaypointClass* point,int& path,BYTE& index) {
    return reinterpret_cast<bool(__thiscall*)(HouseClass*,WaypointClass*,int*,BYTE*)>(0x502460)(this,point,&path,&index);
}
bool InputManagerClass::IsKeyPressed(int key) const {
    return reinterpret_cast<bool(__thiscall*)(const InputManagerClass*,int)>(0x54F5C0)(this,key);
}
extern "C" {
__declspec(dllexport) bool __fastcall ConvertRadarAction(DisplayClass* self,void*,const CellStruct& cell,bool fog,ObjectClass* object,Action action,bool mini) {
    return self->DisplayClass::ConvertAction(cell,fog,object,action,mini);
}
__declspec(dllexport) bool __cdecl RadarAttackMove(){return Game::IsAttackMoveMode();}
__declspec(dllexport) bool __fastcall MouseSet(MouseClass* self,void*,MouseCursorType index,bool mini){return self->MouseClass::SetCursor(index,mini);}
__declspec(dllexport) bool __fastcall MouseOverride(MouseClass* self,void*,MouseCursorType index,bool mini){return self->MouseClass::UpdateCursor(index,mini);}
__declspec(dllexport) bool __fastcall MouseRestore(MouseClass* self,void*){return self->MouseClass::RestoreCursor();}
__declspec(dllexport) void __fastcall MouseMini(MouseClass* self,void*,bool mini){self->MouseClass::UpdateCursorMinimapState(mini);}
__declspec(dllexport) MouseCursorType __fastcall MouseLast(MouseClass* self,void*){return self->MouseClass::GetLastMouseCursor();}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
