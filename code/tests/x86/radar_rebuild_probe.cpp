// Production Reset/PostLoad sequence. Existing allocation, range, image,
// and mode entries are explicit boundaries; no substitute Radar object.
#include "yrpp/RadarClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/FPSCounter.h"
#include <cstddef>
static_assert(sizeof(RadarClass)==0x150C);
static_assert(offsetof(TechnoClass,IsRadarTracked)==0x423);
RadarClass& RadarClass::Instance=*reinterpret_cast<RadarClass*>(0x87F7E8);
DynamicVectorClass<TechnoClass*>& TechnoClass::Array=*reinterpret_cast<DynamicVectorClass<TechnoClass*>*>(0xA8EC78);
RulesClass*& RulesClass::Instance=*reinterpret_cast<RulesClass**>(0x8871E0);
unsigned& Detail::MinFrameRate=*reinterpret_cast<unsigned*>(0x829FF4);
void Detail::SetMinFrameRate(unsigned v) noexcept {MinFrameRate=v;}
void RadarClass::InitRadar() noexcept {reinterpret_cast<void(__thiscall*)(RadarClass*)>(0x6558D0)(this);}
void RadarClass::ComputeRadarImage() noexcept {reinterpret_cast<void(__thiscall*)(RadarClass*)>(0x654650)(this);}
void RadarClass::SetRadarMode(int mode,bool sound) noexcept {reinterpret_cast<void(__thiscall*)(RadarClass*,int,bool)>(0x656CB0)(this,mode,sound);}
extern "C" {
__declspec(dllexport) void __fastcall RadarReset(RadarClass* self,void*){self->ResetRadar();}
__declspec(dllexport) void __fastcall RadarPostLoad(RadarClass* self,void*){self->PostLoadRadarFixup();}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __cdecl __std_exception_copy(const __std_exception_data*,__std_exception_data*){__debugbreak();}
void __cdecl __std_exception_destroy(__std_exception_data*){__debugbreak();}
void __stdcall _CxxThrowException(void*,void*){__debugbreak();}
void __cdecl RadarRebuildUnexpected(){__debugbreak();}
void (*RadarRebuildWatson)()=RadarRebuildUnexpected;
void* RadarRebuildTypeInfo[2]={nullptr,nullptr};
volatile void* RadarRebuildFreed;
__declspec(dllexport) __declspec(noinline) void __cdecl RadarRebuildFree(void* p){RadarRebuildFreed=p;__debugbreak();}
void* __cdecl memset(void* p,int v,std::size_t n){auto* b=static_cast<volatile byte*>(p);while(n--)*b++=byte(v);return p;}
}
void* __cdecl operator new(std::size_t){__debugbreak();return nullptr;}
void __cdecl operator delete(void* p,std::size_t) noexcept {RadarRebuildFree(p);}
void __cdecl operator delete[](void* p) noexcept {RadarRebuildFree(p);}
void __cdecl operator delete[](void* p,std::size_t) noexcept {RadarRebuildFree(p);}
#pragma comment(linker,"/alternatename:__imp___invoke_watson=_RadarRebuildWatson")
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_RadarRebuildTypeInfo")
