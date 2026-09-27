// Production display dispatch, map clipping and Foot path assignment.
// House path queries and unit commands are trace boundaries in this probe;
// native integration exercises their actual class implementations separately.
#include "yrpp/DisplayClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/WaypointPathClass.h"
#include <cstddef>
static_assert(offsetof(TechnoClass,unknown_bool_430)==0x430);
static_assert(offsetof(FootClass,PlanningPathIdx)==0x520);
static_assert(offsetof(FootClass,WaypointNearbyAccessibleCellDelta)==0x524);
static_assert(offsetof(FootClass,WaypointCell)==0x528);
static_assert(offsetof(FootClass,WaypointIndex)==0x686);
static_assert(offsetof(HouseClass,PlanningPaths)==0x210);
static_assert(sizeof(WaypointClass)==0x0C);
static_assert(sizeof(WaypointPathClass)==0x40);
static_assert(offsetof(WaypointPathClass,Waypoints)==0x28);
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x87F7E8);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=*reinterpret_cast<DynamicVectorClass<ObjectClass*>*>(0xA8ECB8);
byte& Unsorted::ArmageddonMode=*reinterpret_cast<byte*>(0xA8ED6B);
bool& Unsorted::MoveFeedback=*reinterpret_cast<bool*>(0x822CF2);
CellClass* MapClass::GetCellAt(const CellStruct& cell) const {
    return reinterpret_cast<CellClass*(__thiscall*)(const MapClass*,const CellStruct*)>(0x5657A0)(this,&cell);
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
extern "C" {
__declspec(dllexport) void __fastcall RadarActiveClick(DisplayClass* self,void*,ObjectClass* object,CellStruct cell,Action action){self->ActiveClick(object,cell,action);}
__declspec(dllexport) CellStruct* __fastcall RadarClip(MapClass* self,void*,CellStruct* out,const CellStruct* cell){return self->ClipToMap(out,*cell);}
__declspec(dllexport) void __fastcall RadarAssignPath(FootClass* self,void*,int path,signed char index){self->FootClass::AssignPlanningPath(path,index);}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
