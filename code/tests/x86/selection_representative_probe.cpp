// Actual production selection helper, sharing original sqrt and controlled
// virtual object queries. No host selection or command model is substituted.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/YRMath.h"
#include <cstddef>
static_assert(offsetof(TechnoClass,Berzerk)==0x298);
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=*reinterpret_cast<DynamicVectorClass<ObjectClass*>*>(0xA8ECB8);
int TechnoClass::CombatDamage(int weapon) const {
    return reinterpret_cast<int(__thiscall*)(const TechnoClass*,int)>(0x6F3970)(this,weapon);
}
double Math::sqrt(double value){return reinterpret_cast<double(__cdecl*)(double)>(0x4CAC40)(value);}
extern "C" {
__declspec(dllexport) ObjectClass* __fastcall SelectionRepresentative(const CellStruct* cell,ObjectClass* target){return Unsorted::BestSelectedObject(cell,target);}
__declspec(dllexport) int __cdecl SelectionSetRound(int){__debugbreak();return 0;}
int (__cdecl* SelectionRoundImport)(int)=SelectionSetRound;
int _fltused=0;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __cdecl SelectionUnexpected(){__debugbreak();}
auto SelectionUnexpectedImport=&SelectionUnexpected;
}
#pragma comment(linker,"/alternatename:__imp__fesetround=_SelectionRoundImport")
#pragma comment(linker,"/alternatename:__imp____stdio_common_vsprintf=_SelectionUnexpectedImport")
#pragma comment(linker,"/alternatename:__imp____stdio_common_vswprintf=_SelectionUnexpectedImport")
