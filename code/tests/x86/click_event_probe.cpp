#include "yrpp/EventClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include <cstddef>
#include <new>
static_assert(sizeof(EventClass)==0x6F);
static_assert(offsetof(EventClass,DataBuffer)==0x7);
static_assert(offsetof(SuperWeaponTypeClass,Action)==0xBC);
int& Unsorted::CurrentFrame=*reinterpret_cast<int*>(0xA8ED84);
DynamicVectorClass<SuperWeaponTypeClass*>& SuperWeaponTypeClass::Array=*reinterpret_cast<DynamicVectorClass<SuperWeaponTypeClass*>*>(0xA8E330);
extern "C" {
__declspec(dllexport) EventClass* __fastcall TargetEvent(void* self,void*,int house,EventType type,int id,int rtti){return ::new(self) EventClass(house,type,id,rtti);}
__declspec(dllexport) EventClass* __fastcall CellEvent(void* self,void*,int house,EventType type,const CellStruct& cell){return ::new(self) EventClass(house,type,cell);}
__declspec(dllexport) EventClass* __fastcall PlaceEvent(void* self,void*,int house,EventType type,AbstractType rtti,int heap,int naval,const CellStruct& cell){return ::new(self) EventClass(house,type,rtti,heap,naval,cell);}
__declspec(dllexport) EventClass* __fastcall SimplePlaceEvent(void* self,void*,int house,EventType type,AbstractType rtti,const CellStruct& cell){return ::new(self) EventClass(house,type,rtti,cell);}
__declspec(dllexport) EventClass* __fastcall GroundPlaceEvent(void* self,void*,int house,EventType type,AbstractType rtti,int heap,const CellStruct& cell){return ::new(self) EventClass(house,type,rtti,heap,cell);}
__declspec(dllexport) EventClass* __fastcall SpecialEvent(void* self,void*,int house,EventType type,int id,const CellStruct& cell){return ::new(self) EventClass(house,type,id,cell);}
__declspec(dllexport) SuperWeaponTypeClass* __fastcall FindAction(Action action){return SuperWeaponTypeClass::FindFirstOfAction(action);}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
